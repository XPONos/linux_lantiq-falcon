/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 * Copyright (C) 2010 Thomas Langer, Lantiq Deutschland
 */

#ifndef __FALCON_SYSCTRL_H
#define __FALCON_SYSCTRL_H

#include <falcon/lantiq_soc.h>

#ifdef CONFIG_FALCON_COMPAT_SYSCTRL
static inline void sys1_hw_activate(u32 mask)
{ ltq_sysctl_activate(SYSCTL_SYS1, mask); }
static inline void sys1_hw_deactivate(u32 mask)
{ ltq_sysctl_deactivate(SYSCTL_SYS1, mask); }
static inline void sys1_hw_clk_enable(u32 mask)
{ ltq_sysctl_clken(SYSCTL_SYS1, mask); }
static inline void sys1_hw_clk_disable(u32 mask)
{ ltq_sysctl_clkdis(SYSCTL_SYS1, mask); }
static inline void sys1_hw_activate_or_reboot(u32 mask)
{ ltq_sysctl_reboot(SYSCTL_SYS1, mask); }

static inline void sys_eth_hw_activate(u32 mask)
{ ltq_sysctl_activate(SYSCTL_SYSETH, mask); }
static inline void sys_eth_hw_deactivate(u32 mask)
{ ltq_sysctl_deactivate(SYSCTL_SYSETH, mask); }
static inline void sys_eth_hw_clk_enable(u32 mask)
{ ltq_sysctl_clken(SYSCTL_SYSETH, mask); }
static inline void sys_eth_hw_clk_disable(u32 mask)
{ ltq_sysctl_clkdis(SYSCTL_SYSETH, mask); }
static inline void sys_eth_hw_activate_or_reboot(u32 mask)
{ ltq_sysctl_reboot(SYSCTL_SYSETH, mask); }

static inline void sys_gpe_hw_activate(u32 mask)
{ ltq_sysctl_activate(SYSCTL_SYSGPE, mask); }
static inline void sys_gpe_hw_deactivate(u32 mask)
{ ltq_sysctl_deactivate(SYSCTL_SYSGPE, mask); }
static inline void sys_gpe_hw_clk_enable(u32 mask)
{ ltq_sysctl_clken(SYSCTL_SYSGPE, mask); }
static inline void sys_gpe_hw_clk_disable(u32 mask)
{ ltq_sysctl_clkdis(SYSCTL_SYSGPE, mask); }
static inline void sys_gpe_hw_activate_or_reboot(u32 mask)
{ ltq_sysctl_reboot(SYSCTL_SYSGPE, mask); }
static inline int sys_gpe_hw_is_activated(u32 mask)
{ return 1; }
#endif

#ifdef CONFIG_FALCON_TIMER
void ltq_sysctl_sleep_enable(int cpu0, int xbar);
#endif

#endif /* __FALCON_SYSCTRL_H */
