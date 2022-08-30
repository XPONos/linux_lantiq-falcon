dtb-$(CONFIG_DT_EASY50712)		+= easy50712.dtb
dtb-$(CONFIG_DT_EASY98000NOR)		+= falcon_lantiq_easy98000-nor.dtb
dtb-$(CONFIG_DT_EASY98000SFLASH)	+= falcon_lantiq_easy98000-sflash.dtb
dtb-$(CONFIG_DT_FALCONSFP)		+= falcon_lantiq_falcon-sfp.dtb

obj-y		+= $(patsubst %.dtb, %.dtb.o, $(dtb-y))

targets		+= dtbs
targets		+= $(dtb-y)

dtbs: $(addprefix $(obj)/, $(dtb-y))

clean-files	+= *.dtb *.dtb.S
