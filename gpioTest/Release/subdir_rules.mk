################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
GPIO_board.obj: ../GPIO_board.c $(GEN_OPTS) | $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: C6000 Compiler'
	"D:/ti/ccsv7/tools/compiler/ti-cgt-c6000_8.2.1/bin/cl6x" -mv6740 -O2 --include_path="D:/Users/zhuji/Projects/CCD/SY18-201708/DSP/gpioTest" --include_path="D:/ti/ccsv7/tools/compiler/ti-cgt-c6000_8.2.1/include" --define=c6748 --diag_warning=225 --diag_wrap=off --display_error_number --preproc_with_compile --preproc_dependency="GPIO_board.d" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

build-1637256623:
	@$(MAKE) -Onone -f subdir_rules.mk build-1637256623-inproc

build-1637256623-inproc: ../app.cfg
	@echo 'Building file: $<'
	@echo 'Invoking: XDCtools'
	"D:/ti/xdctools_3_32_02_25_core/xs" --xdcpath="D:/ti/bios_6_46_05_55/packages;D:/ti/ccsv7/ccs_base;" xdc.tools.configuro -o configPkg -t ti.targets.elf.C674 -p ti.platforms.evm6748 -r release -c "D:/ti/ccsv7/tools/compiler/ti-cgt-c6000_8.2.1" --compileOptions "-g --optimize_with_debug" "$<"
	@echo 'Finished building: $<'
	@echo ' '

configPkg/linker.cmd: build-1637256623 ../app.cfg
configPkg/compiler.opt: build-1637256623
configPkg/: build-1637256623

main_led_blink.obj: ../main_led_blink.c $(GEN_OPTS) | $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: C6000 Compiler'
	"D:/ti/ccsv7/tools/compiler/ti-cgt-c6000_8.2.1/bin/cl6x" -mv6740 -O2 --include_path="D:/Users/zhuji/Projects/CCD/SY18-201708/DSP/gpioTest" --include_path="D:/ti/ccsv7/tools/compiler/ti-cgt-c6000_8.2.1/include" --define=c6748 --diag_warning=225 --diag_wrap=off --display_error_number --preproc_with_compile --preproc_dependency="main_led_blink.d" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '


