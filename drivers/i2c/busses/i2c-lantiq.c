
/*
 * Lantiq I2C bus adapter
 *
 * Parts based on i2c-designware.c and other i2c drivers from Linux 2.6.33
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Copyright (C) 2012 Thomas Langer <thomas.langer@lantiq.com>
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/slab.h> /* for kzalloc, kfree */
#include <linux/i2c.h>
#include <linux/errno.h>
#include <linux/completion.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/of_irq.h>
#include <linux/of_i2c.h>

#include <lantiq_soc.h>
#include "i2c-lantiq.h"

/*
 * CURRENT ISSUES:
 * - no high speed support
 * - ten bit mode is not tested (no slave devices)
 */

/* access macros */
#define i2c_r32(reg)	\
	__raw_readl(&(priv->membase)->reg)
#define i2c_w32(val, reg)	\
	__raw_writel(val, &(priv->membase)->reg)
#define i2c_w32_mask(clear, set, reg)	\
	i2c_w32((i2c_r32(reg) & ~(clear)) | (set), reg)

#define DRV_NAME "i2c-lantiq"
#define DRV_VERSION "1.00"

#define LTQ_I2C_BUSY_TIMEOUT		20 /* ms */

#ifdef DEBUG
#define LTQ_I2C_XFER_TIMEOUT		(25*HZ)
#else
#define LTQ_I2C_XFER_TIMEOUT		HZ
#endif

#define LTQ_I2C_IMSC_DEFAULT_MASK	(I2C_IMSC_I2C_P_INT_EN | \
					 I2C_IMSC_I2C_ERR_INT_EN)

#define LTQ_I2C_ARB_LOST		(1 << 0)
#define LTQ_I2C_NACK			(1 << 1)
#define LTQ_I2C_RX_UFL			(1 << 2)
#define LTQ_I2C_RX_OFL			(1 << 3)
#define LTQ_I2C_TX_UFL			(1 << 4)
#define LTQ_I2C_TX_OFL			(1 << 5)

struct ltq_i2c {
	struct mutex mutex;


	/* active clock settings */
	unsigned int input_clock;	/* clock input for i2c hardware block */
	unsigned int i2c_clock;		/* approximated bus clock in kHz */

	struct clk *clk_gate;
	struct clk *clk_input;


	/* resources (memory and interrupts) */
	int irq_lb;				/* last burst irq */

	struct lantiq_reg_i2c __iomem *membase;	/* base of mapped registers */

	struct i2c_adapter adap;
	struct device *dev;

	struct completion cmd_complete;


	/* message transfer data */
	struct i2c_msg *current_msg;	/* current message */
	int msgs_num;		/* number of messages to handle */
	u8 *msg_buf;		/* current buffer */
	u32 msg_buf_len;	/* remaining length of current buffer */
	int msg_err;		/* error status of the current transfer */


	/* master status codes */
	enum {
		STATUS_IDLE,
		STATUS_ADDR,	/* address phase */
		STATUS_WRITE,
		STATUS_READ,
		STATUS_READ_END,
		STATUS_STOP
	} status;
};

static irqreturn_t ltq_i2c_isr(int irq, void *dev_id);

static inline void enable_burst_irq(struct ltq_i2c *priv)
{
	i2c_w32_mask(0, I2C_IMSC_LBREQ_INT_EN | I2C_IMSC_BREQ_INT_EN, imsc);
}
static inline void disable_burst_irq(struct ltq_i2c *priv)
{
	i2c_w32_mask(I2C_IMSC_LBREQ_INT_EN | I2C_IMSC_BREQ_INT_EN, 0, imsc);
}

static void prepare_msg_send_addr(struct ltq_i2c *priv)
{
	struct i2c_msg *msg = priv->current_msg;
	int rd = !!(msg->flags & I2C_M_RD);	/* extends to 0 or 1 */
	u16 addr = msg->addr;

	/* new i2c_msg */
	priv->msg_buf = msg->buf;
	priv->msg_buf_len = msg->len;
	if (rd)
		priv->status = STATUS_READ;
	else
		priv->status = STATUS_WRITE;

	/* send slave address */
	if (msg->flags & I2C_M_TEN) {
		i2c_w32(0xf0 | ((addr & 0x300) >> 7) | rd, txd);
		i2c_w32(addr & 0xff, txd);
	} else {
		i2c_w32((addr & 0x7f) << 1 | rd, txd);
	}
}

static void ltq_i2c_set_tx_len(struct ltq_i2c *priv)
{
	struct i2c_msg *msg = priv->current_msg;
	int len = (msg->flags & I2C_M_TEN) ? 2 : 1;

	pr_debug("set_tx_len %cX\n", (msg->flags & I2C_M_RD) ? 'R' : 'T');

	priv->status = STATUS_ADDR;

	if (!(msg->flags & I2C_M_RD))
		len += msg->len;
	else
		/* set maximum received packet size (before rx int!) */
		i2c_w32(msg->len, mrps_ctrl);
	i2c_w32(len, tps_ctrl);
	enable_burst_irq(priv);
}

static int ltq_i2c_hw_set_clock(struct i2c_adapter *adap)
{
	struct ltq_i2c *priv = i2c_get_adapdata(adap);
	unsigned int input_clock = clk_get_rate(priv->clk_input);
	u32 dec, inc = 1;

	/* clock changed? */
	if (priv->input_clock == input_clock)
		return 0;

	/*
	 * this formula is only an approximation, found by the recommended
	 * values in the "I2C Architecture Specification 1.7.1"
	 */
	dec = input_clock / (priv->i2c_clock * 2);
	if (dec <= 6)
		return -ENXIO;

	i2c_w32(0, fdiv_high_cfg);
	i2c_w32((inc << I2C_FDIV_CFG_INC_OFFSET) |
		(dec << I2C_FDIV_CFG_DEC_OFFSET),
		fdiv_cfg);

	dev_info(priv->dev, "setup clocks (in %d kHz, bus %d kHz, dec=%d)\n",
		input_clock, priv->i2c_clock, dec);

	priv->input_clock = input_clock;
	return 0;
}

static int ltq_i2c_hw_init(struct i2c_adapter *adap)
{
	int ret = 0;
	struct ltq_i2c *priv = i2c_get_adapdata(adap);

	/* disable bus */
	i2c_w32_mask(I2C_RUN_CTRL_RUN_EN, 0, run_ctrl);

#ifndef DEBUG
	/* set normal operation clock divider */
	i2c_w32(1 << I2C_CLC_RMC_OFFSET, clc);
#else
	/* for debugging a higher divider value! */
	i2c_w32(0xF0 << I2C_CLC_RMC_OFFSET, clc);
#endif

	/* setup clock */
	ret = ltq_i2c_hw_set_clock(adap);
	if (ret != 0) {
		dev_warn(priv->dev, "invalid clock settings\n");
		return ret;
	}

	/* configure fifo */
	i2c_w32(I2C_FIFO_CFG_TXFC | /* tx fifo as flow controller */
		I2C_FIFO_CFG_RXFC | /* rx fifo as flow controller */
		I2C_FIFO_CFG_TXFA_TXFA2 | /* tx fifo 4-byte aligned */
		I2C_FIFO_CFG_RXFA_RXFA2 | /* rx fifo 4-byte aligned */
		I2C_FIFO_CFG_TXBS_TXBS0 | /* tx fifo burst size is 1 word */
		I2C_FIFO_CFG_RXBS_RXBS0,  /* rx fifo burst size is 1 word */
		fifo_cfg);

	/* configure address */
	i2c_w32(I2C_ADDR_CFG_SOPE_EN |	/* generate stop when no more data in
					   the fifo */
		I2C_ADDR_CFG_SONA_EN |	/* generate stop when NA received */
		I2C_ADDR_CFG_MnS_EN |	/* we are master device */
		0,			/* our slave address (not used!) */
		addr_cfg);

	/* enable bus */
	i2c_w32_mask(0, I2C_RUN_CTRL_RUN_EN, run_ctrl);

	return 0;
}

static int ltq_i2c_wait_bus_not_busy(struct ltq_i2c *priv)
{
	unsigned long timeout;

	timeout = jiffies + msecs_to_jiffies(LTQ_I2C_BUSY_TIMEOUT);

	do {
		u32 stat = i2c_r32(bus_stat);

		if ((stat & I2C_BUS_STAT_BS_MASK) == I2C_BUS_STAT_BS_FREE)
			return 0;

		cond_resched();
	} while (!time_after_eq(jiffies, timeout));

	dev_err(priv->dev, "timeout waiting for bus ready\n");
	return -ETIMEDOUT;
}

static void ltq_i2c_tx(struct ltq_i2c *priv, int last)
{
	if (priv->msg_buf_len && priv->msg_buf) {
		i2c_w32(*priv->msg_buf, txd);

		if (--priv->msg_buf_len)
			priv->msg_buf++;
		else
			priv->msg_buf = NULL;
	} else {
		last = 1;
	}

	if (last)
		disable_burst_irq(priv);
}

static void ltq_i2c_rx(struct ltq_i2c *priv, int last)
{
	u32 fifo_stat, timeout;
	if (priv->msg_buf_len && priv->msg_buf) {
		timeout = 5000000;
		do {
			fifo_stat = i2c_r32(ffs_stat);
		} while (!fifo_stat && --timeout);
		if (!timeout) {
			last = 1;
			pr_debug("\nrx timeout\n");
			goto err;
		}
		while (fifo_stat) {
			*priv->msg_buf = i2c_r32(rxd);
			if (--priv->msg_buf_len) {
				priv->msg_buf++;
			} else {
				priv->msg_buf = NULL;
				last = 1;
				break;
			}
			/*
			 * do not read more than burst size, otherwise no "last
			 * burst" is generated and the transaction is blocked!
			 */
			fifo_stat = 0;
		}
	} else {
		last = 1;
	}
err:
	if (last) {
		disable_burst_irq(priv);

		if (priv->status == STATUS_READ_END) {
			/* 
			 * do the STATUS_STOP and complete() here, as sometimes
			 * the tx_end is already seen before this is finished
			 */
			priv->status = STATUS_STOP;
			complete(&priv->cmd_complete);
		} else {
			i2c_w32(I2C_ENDD_CTRL_SETEND, endd_ctrl);
			priv->status = STATUS_READ_END;
		}
	}
}

static void ltq_i2c_xfer_init(struct ltq_i2c *priv)
{
	/* enable interrupts */
	i2c_w32(LTQ_I2C_IMSC_DEFAULT_MASK, imsc);

	/* trigger transfer of first msg */
	ltq_i2c_set_tx_len(priv);
}

static void dump_msgs(struct i2c_msg msgs[], int num, int rx)
{
#if defined(DEBUG)
	int i, j;
	pr_debug("Messages %d %s\n", num, rx ? "out" : "in");
	for (i = 0; i < num; i++) {
		pr_debug("%2d %cX Msg(%d) addr=0x%X: ", i,
			(msgs[i].flags & I2C_M_RD) ? 'R' : 'T',
			msgs[i].len, msgs[i].addr);
		if (!(msgs[i].flags & I2C_M_RD) || rx) {
			for (j = 0; j < msgs[i].len; j++)
				pr_debug("%02X ", msgs[i].buf[j]);
		}
		pr_debug("\n");
	}
#endif
}

static void ltq_i2c_release_bus(struct ltq_i2c *priv)
{
	if ((i2c_r32(bus_stat) & I2C_BUS_STAT_BS_MASK) == I2C_BUS_STAT_BS_BM)
		i2c_w32(I2C_ENDD_CTRL_SETEND, endd_ctrl);
}

static int ltq_i2c_xfer(struct i2c_adapter *adap, struct i2c_msg msgs[],
			   int num)
{
	struct ltq_i2c *priv = i2c_get_adapdata(adap);
	int ret;

	dev_dbg(priv->dev, "xfer %u messages\n", num);
	dump_msgs(msgs, num, 0);

	mutex_lock(&priv->mutex);

	INIT_COMPLETION(priv->cmd_complete);
	priv->current_msg = msgs;
	priv->msgs_num = num;
	priv->msg_err = 0;
	priv->status = STATUS_IDLE;

	/* wait for the bus to become ready */
	ret = ltq_i2c_wait_bus_not_busy(priv);
	if (ret)
		goto done;

	while (priv->msgs_num) {
		/* start the transfers */
		ltq_i2c_xfer_init(priv);

		/* wait for transfers to complete */
		ret = wait_for_completion_interruptible_timeout(
			&priv->cmd_complete, LTQ_I2C_XFER_TIMEOUT);
		if (ret == 0) {
			dev_err(priv->dev, "controller timed out\n");
			ltq_i2c_hw_init(adap);
			ret = -ETIMEDOUT;
			goto done;
		} else if (ret < 0)
			goto done;

		if (priv->msg_err) {
			if (priv->msg_err & LTQ_I2C_NACK)
				ret = -ENXIO;
			else
				ret = -EREMOTEIO;
			goto done;
		}
		if (--priv->msgs_num)
			priv->current_msg++;
	}
	/* no error? */
	ret = num;

done:
	ltq_i2c_release_bus(priv);

	mutex_unlock(&priv->mutex);

	if (ret >= 0)
		dump_msgs(msgs, num, 1);

	pr_debug("XFER ret %d\n", ret);
	return ret;
}

static irqreturn_t ltq_i2c_isr_burst(int irq, void *dev_id)
{
	struct ltq_i2c *priv = dev_id;
	struct i2c_msg *msg = priv->current_msg;
	int last = (irq == priv->irq_lb);

	if (last)
		pr_debug("LB ");
	else
		pr_debug("B ");

	if (msg->flags & I2C_M_RD) {
		switch (priv->status) {
		case STATUS_ADDR:
			pr_debug("X");
			prepare_msg_send_addr(priv);
			disable_burst_irq(priv);
			break;
		case STATUS_READ:
		case STATUS_READ_END:
			pr_debug("R");
			ltq_i2c_rx(priv, last);
			break;
		default:
			disable_burst_irq(priv);
			pr_warn("Status R %d\n", priv->status);
			break;
		}
	} else {
		switch (priv->status) {
		case STATUS_ADDR:
			pr_debug("x");
			prepare_msg_send_addr(priv);
			break;
		case STATUS_WRITE:
			pr_debug("w");
			ltq_i2c_tx(priv, last);
			break;
		default:
			disable_burst_irq(priv);
			pr_warn("Status W %d\n", priv->status);
			break;
		}
	}

	i2c_w32(I2C_ICR_BREQ_INT_CLR | I2C_ICR_LBREQ_INT_CLR, icr);
	return IRQ_HANDLED;
}

static void ltq_i2c_isr_prot(struct ltq_i2c *priv)
{
	u32 i_pro = i2c_r32(p_irqss);

	pr_debug("i2c-p");

	/* not acknowledge */
	if (i_pro & I2C_P_IRQSS_NACK) {
		priv->msg_err |= LTQ_I2C_NACK;
		pr_debug(" nack");
	}

	/* arbitration lost */
	if (i_pro & I2C_P_IRQSS_AL) {
		priv->msg_err |= LTQ_I2C_ARB_LOST;
		pr_debug(" arb-lost");
	}
	/* tx -> rx switch */
	if (i_pro & I2C_P_IRQSS_RX)
		pr_debug(" rx");

	/* tx end */
	if (i_pro & I2C_P_IRQSS_TX_END)
		pr_debug(" txend");
	pr_debug("\n");

	if (!priv->msg_err) {
		/* tx -> rx switch */
		if (i_pro & I2C_P_IRQSS_RX) {
			priv->status = STATUS_READ;
			enable_burst_irq(priv);
		}
		if (i_pro & I2C_P_IRQSS_TX_END) {
			if (priv->status == STATUS_READ)
				priv->status = STATUS_READ_END;
			else {
				disable_burst_irq(priv);
				priv->status = STATUS_STOP;
			}
		}
	}

	i2c_w32(i_pro, p_irqsc);
}

static irqreturn_t ltq_i2c_isr(int irq, void *dev_id)
{
	u32 i_raw, i_err = 0;
	struct ltq_i2c *priv = dev_id;

	i_raw = i2c_r32(mis);
	pr_debug("i_raw 0x%08X\n", i_raw);

	/* error interrupt */
	if (i_raw & I2C_RIS_I2C_ERR_INT_INTOCC) {
		i_err = i2c_r32(err_irqss);
		pr_debug("i_err 0x%08X bus_stat 0x%04X\n",
			i_err, i2c_r32(bus_stat));

		/* tx fifo overflow (8) */
		if (i_err & I2C_ERR_IRQSS_TXF_OFL)
			priv->msg_err |= LTQ_I2C_TX_OFL;

		/* tx fifo underflow (4) */
		if (i_err & I2C_ERR_IRQSS_TXF_UFL)
			priv->msg_err |= LTQ_I2C_TX_UFL;

		/* rx fifo overflow (2) */
		if (i_err & I2C_ERR_IRQSS_RXF_OFL)
			priv->msg_err |= LTQ_I2C_RX_OFL;

		/* rx fifo underflow (1) */
		if (i_err & I2C_ERR_IRQSS_RXF_UFL)
			priv->msg_err |= LTQ_I2C_RX_UFL;

		i2c_w32(i_err, err_irqsc);
	}

	/* protocol interrupt */
	if (i_raw & I2C_RIS_I2C_P_INT_INTOCC)
		ltq_i2c_isr_prot(priv);

	if ((priv->msg_err) || (priv->status == STATUS_STOP))
		complete(&priv->cmd_complete);

	return IRQ_HANDLED;
}

static u32 ltq_i2c_functionality(struct i2c_adapter *adap)
{
	return	I2C_FUNC_I2C |
		I2C_FUNC_10BIT_ADDR |
		I2C_FUNC_SMBUS_EMUL;
}

static struct i2c_algorithm ltq_i2c_algorithm = {
	.master_xfer	= ltq_i2c_xfer,
	.functionality	= ltq_i2c_functionality,
};

static int __devinit ltq_i2c_probe(struct platform_device *pdev)
{
	struct device_node *node = pdev->dev.of_node;
	struct ltq_i2c *priv;
	struct i2c_adapter *adap;
	struct resource *mmres, irqres[4];
	int ret = 0;

	dev_dbg(&pdev->dev, "probing\n");

	mmres = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	ret = of_irq_to_resource_table(node, irqres, 4);
	if (!mmres || (ret != 4)) {
		dev_err(&pdev->dev, "no resources\n");
		return -ENODEV;
	}

	/* allocate private data */
	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		dev_err(&pdev->dev, "can't allocate private data\n");
		return -ENOMEM;
	}

	adap = &priv->adap;
	i2c_set_adapdata(adap, priv);
	adap->owner = THIS_MODULE;
	adap->class = I2C_CLASS_HWMON | I2C_CLASS_SPD;
	strlcpy(adap->name, DRV_NAME "-adapter", sizeof(adap->name));
	adap->algo = &ltq_i2c_algorithm;

	if (of_property_read_u32(node, "clock-frequency", &priv->i2c_clock)) {
		dev_warn(&pdev->dev, "No I2C speed selected, using 100kHz\n");
		priv->i2c_clock = 100000;
	}

	init_completion(&priv->cmd_complete);
	mutex_init(&priv->mutex);

	priv->membase = devm_request_and_ioremap(&pdev->dev, mmres);
	if (priv->membase == NULL)
		return -ENOMEM;

	priv->dev = &pdev->dev;
	priv->irq_lb = irqres[0].start;

	ret = devm_request_irq(&pdev->dev, irqres[0].start, ltq_i2c_isr_burst,
		IRQF_DISABLED, "i2c lb", priv);
	if (ret) {
		dev_err(&pdev->dev, "can't get last burst IRQ %d\n",
			irqres[0].start);
		return -ENODEV;
	}

	ret = devm_request_irq(&pdev->dev, irqres[1].start, ltq_i2c_isr_burst,
		IRQF_DISABLED, "i2c b", priv);
	if (ret) {
		dev_err(&pdev->dev, "can't get burst IRQ %d\n",
			irqres[1].start);
		return -ENODEV;
	}

	ret = devm_request_irq(&pdev->dev, irqres[2].start, ltq_i2c_isr,
		IRQF_DISABLED, "i2c err", priv);
	if (ret) {
		dev_err(&pdev->dev, "can't get error IRQ %d\n",
			irqres[2].start);
		return -ENODEV;
	}

	ret = devm_request_irq(&pdev->dev, irqres[3].start, ltq_i2c_isr,
		IRQF_DISABLED, "i2c p", priv);
	if (ret) {
		dev_err(&pdev->dev, "can't get protocol IRQ %d\n",
			irqres[3].start);
		return -ENODEV;
	}

	dev_dbg(&pdev->dev, "mapped io-space to %p\n", priv->membase);
	dev_dbg(&pdev->dev, "use IRQs %d, %d, %d, %d\n", irqres[0].start,
		irqres[1].start, irqres[2].start, irqres[3].start);

	priv->clk_gate = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(priv->clk_gate)) {
		dev_err(&pdev->dev, "failed to get i2c clk\n");
		return -ENOENT;
	}

	/* this is a static clock, which has no refcounting */
	priv->clk_input = clk_get_fpi();
	if (IS_ERR(priv->clk_input)) {
		dev_err(&pdev->dev, "failed to get fpi clk\n");
		return -ENOENT;
	}

	clk_activate(priv->clk_gate);

	/* add our adapter to the i2c stack */
	ret = i2c_add_numbered_adapter(adap);
	if (ret) {
		dev_err(&pdev->dev, "can't register I2C adapter\n");
		goto out;
	}

	platform_set_drvdata(pdev, priv);
	i2c_set_adapdata(adap, priv);

	/* print module version information */
	dev_dbg(&pdev->dev, "module id=%u revision=%u\n",
		(i2c_r32(id) & I2C_ID_ID_MASK) >> I2C_ID_ID_OFFSET,
		(i2c_r32(id) & I2C_ID_REV_MASK) >> I2C_ID_REV_OFFSET);

	/* initialize HW */
	ret = ltq_i2c_hw_init(adap);
	if (ret) {
		dev_err(&pdev->dev, "can't configure adapter\n");
		i2c_del_adapter(adap);
		platform_set_drvdata(pdev, NULL);
	} else {
		dev_info(&pdev->dev, "version %s\n", DRV_VERSION);
	}

	of_i2c_register_devices(adap);

out:
	/* if init failed, we need to deactivate the clock gate */
	if (ret)
		clk_deactivate(priv->clk_gate);

	return ret;
}

static int __devexit ltq_i2c_remove(struct platform_device *pdev)
{
	struct ltq_i2c *priv = platform_get_drvdata(pdev);

	/* disable bus */
	i2c_w32_mask(I2C_RUN_CTRL_RUN_EN, 0, run_ctrl);

	/* power down the core */
	clk_deactivate(priv->clk_gate);

	/* remove driver */
	i2c_del_adapter(&priv->adap);
	kfree(priv);

	dev_dbg(&pdev->dev, "removed\n");
	platform_set_drvdata(pdev, NULL);

	return 0;
}
static const struct of_device_id ltq_i2c_match[] = {
	{ .compatible = "lantiq,lantiq-i2c" },
	{},
};
MODULE_DEVICE_TABLE(of, ltq_i2c_match);

static struct platform_driver ltq_i2c_driver = {
	.probe	= ltq_i2c_probe,
	.remove	= __devexit_p(ltq_i2c_remove),
	.driver	= {
		.name	= DRV_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = ltq_i2c_match,
	},
};

module_platform_driver(ltq_i2c_driver);

MODULE_DESCRIPTION("Lantiq I2C bus adapter");
MODULE_AUTHOR("Thomas Langer <thomas.langer@lantiq.com>");
MODULE_ALIAS("platform:" DRV_NAME);
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);
