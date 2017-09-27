#
_XDCBUILDCOUNT = 
ifneq (,$(findstring path,$(_USEXDCENV_)))
override XDCPATH = D:/ti/bios_6_46_05_55/packages;D:/ti/pdk_omapl138_1_0_1/packages;D:/ti/ndk_2_20_06_35/packages;D:/ti/ccsv7/ccs_base
override XDCROOT = D:/ti/xdctools_3_32_02_25_core
override XDCBUILDCFG = ./config.bld
endif
ifneq (,$(findstring args,$(_USEXDCENV_)))
override XDCARGS = 
override XDCTARGETS = 
endif
#
ifeq (0,1)
PKGPATH = D:/ti/bios_6_46_05_55/packages;D:/ti/pdk_omapl138_1_0_1/packages;D:/ti/ndk_2_20_06_35/packages;D:/ti/ccsv7/ccs_base;D:/ti/xdctools_3_32_02_25_core/packages;..
HOSTOS = Windows
endif
