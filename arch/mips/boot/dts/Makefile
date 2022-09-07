dtb-$(CONFIG_DT_EASY50712)		+= easy50712.dtb
dtb-$(CONFIG_DT_EASY98000NOR)		+= lantiq_falcon_easy98000-nor.dtb
dtb-$(CONFIG_DT_EASY98000SFLASH)	+= lantiq_falcon_easy98000-sflash.dtb
dtb-$(CONFIG_DT_FALCONSFP)		+= lantiq_falcon-sfp.dtb

obj-y		+= $(patsubst %.dtb, %.dtb.o, $(dtb-y))

targets		+= dtbs
targets		+= $(dtb-y)

dtbs: $(addprefix $(obj)/, $(dtb-y))

clean-files	+= *.dtb *.dtb.S
