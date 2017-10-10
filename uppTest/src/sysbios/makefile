
XOPTS = -I"D:/ti/xdctools_3_32_02_25_core/packages/" -Dxdc_target_types__=D:/ti/bios_6_46_05_55/packages/ti/targets/elf/std.h -Dxdc_target_name__=C674

vpath % D:/ti/bios_6_46_05_55/packages/ti/sysbios/
vpath %.c D:/ti/xdctools_3_32_02_25_core/packages/

CCOPTS    = -mv6740 --abi=eabi -q -mi10 -mo -pdr -pden -pds=238 -pds=880 -pds1110  --program_level_compile -o3 -g --optimize_with_debug  -Dti_sysbios_knl_Task_minimizeLatency__D=FALSE -Dti_sysbios_family_c64p_Exception_enableExternalMPC__D=FALSE -Dti_sysbios_family_c64p_Exception_enablePrint__D=TRUE -Dti_sysbios_knl_Clock_stopCheckNext__D=FALSE

XDC_ROOT  = D:/ti/xdctools_3_32_02_25_core/packages/

BIOS_ROOT = D:/ti/bios_6_46_05_55/packages/ti/sysbios/

BIOS_DEFS =  -Dti_sysbios_BIOS_swiEnabled__D=TRUE -Dti_sysbios_BIOS_taskEnabled__D=TRUE -Dti_sysbios_BIOS_clockEnabled__D=TRUE -Dti_sysbios_BIOS_runtimeCreatesEnabled__D=TRUE -Dti_sysbios_knl_Task_moduleStateCheckFlag__D=FALSE -Dti_sysbios_knl_Task_objectCheckFlag__D=FALSE -Dti_sysbios_hal_Hwi_DISABLE_ALL_HOOKS -Dti_sysbios_knl_Swi_DISABLE_ALL_HOOKS -Dti_sysbios_BIOS_smpEnabled__D=FALSE -Dti_sysbios_Build_useHwiMacros -Dti_sysbios_knl_Swi_numPriorities__D=16 -Dti_sysbios_knl_Task_deleteTerminatedTasks__D=FALSE -Dti_sysbios_knl_Task_numPriorities__D=16 -Dti_sysbios_knl_Task_checkStackFlag__D=TRUE -Dti_sysbios_knl_Task_initStackFlag__D=TRUE -Dti_sysbios_knl_Task_DISABLE_ALL_HOOKS -Dti_sysbios_knl_Clock_TICK_SOURCE=ti_sysbios_knl_Clock_TickSource_TIMER -Dti_sysbios_knl_Clock_TICK_MODE=ti_sysbios_knl_Clock_TickMode_PERIODIC -Dti_sysbios_hal_Core_delegate_getId=ti_sysbios_hal_CoreNull_getId__E -Dti_sysbios_hal_Core_delegate_interruptCore=ti_sysbios_hal_CoreNull_interruptCore__E -Dti_sysbios_hal_Core_delegate_lock=ti_sysbios_hal_CoreNull_lock__E -Dti_sysbios_hal_Core_delegate_unlock=ti_sysbios_hal_CoreNull_unlock__E -Dti_sysbios_hal_Core_numCores__D=1 -Dti_sysbios_hal_CoreNull_numCores__D=1 -Dti_sysbios_utils_Load_taskEnabled__D=TRUE -Dti_sysbios_utils_Load_swiEnabled__D=FALSE -Dti_sysbios_utils_Load_hwiEnabled__D=FALSE -Dti_sysbios_family_c64p_Hwi_dispatcherSwiSupport__D=TRUE -Dti_sysbios_family_c64p_Hwi_dispatcherTaskSupport__D=TRUE -Dti_sysbios_family_c64p_Hwi_dispatcherAutoNestingSupport__D=TRUE -Dti_sysbios_family_c64p_Hwi_dispatcherIrpTrackingSupport__D=TRUE -Dti_sysbios_knl_Semaphore_supportsEvents__D=FALSE -Dti_sysbios_knl_Semaphore_supportsPriority__D=TRUE 

BIOS_INC  = -I"D:/ti/bios_6_46_05_55/packages/"

INCS =  $(BIOS_INC) -I"D:/ti/bios_6_46_05_55/packages/" -I"D:/ti/pdk_omapl138_1_0_1/packages/" -I"D:/ti/ccsv7/ccs_base/" -I"D:/ti/xdctools_3_32_02_25_core/packages/" -I"../" -I"/packages//" 

CC = D:/ti/ccsv7/tools/compiler/ti-cgt-c6000_8.2.1/bin/cl6x -c $(CCOPTS) -I D:/ti/ccsv7/tools/compiler/ti-cgt-c6000_8.2.1/include
ASM = D:/ti/ccsv7/tools/compiler/ti-cgt-c6000_8.2.1/bin/cl6x -c $(CCOPTS) -I D:/ti/ccsv7/tools/compiler/ti-cgt-c6000_8.2.1/include
AR = D:/ti/ccsv7/tools/compiler/ti-cgt-c6000_8.2.1/bin/ar6x rq

DEL = D:/ti/xdctools_3_32_02_25_core/packages/../bin/rm -f
CP = D:/ti/xdctools_3_32_02_25_core/packages/../bin/cp -f

define RM
	$(if $(wildcard $1),$(DEL) $1,:)
endef

define ASSEMBLE
	@echo asme674 $< ...
	@$(ASM) $(BIOS_DEFS) $(XOPTS) $(INCS) $<
endef

all: sysbios.ae674

c64p_Exception_asm.obj: family/c64p/Exception_asm.s64P makefile
	@-$(call RM, $@)
	$(ASSEMBLE) --output_file=c64p_Exception_asm.obj

c64p_Hwi_asm.obj: family/c64p/Hwi_asm.s62 makefile
	@-$(call RM, $@)
	$(ASSEMBLE) --output_file=c64p_Hwi_asm.obj

c64p_Hwi_asm_switch.obj: family/c64p/Hwi_asm_switch.s62 makefile
	@-$(call RM, $@)
	$(ASSEMBLE) --output_file=c64p_Hwi_asm_switch.obj

c64p_Hwi_disp_always.obj: family/c64p/Hwi_disp_always.s64P makefile
	@-$(call RM, $@)
	$(ASSEMBLE) --output_file=c64p_Hwi_disp_always.obj

ti_tls_get_tp.obj: rts/ti/tls_get_tp.asm makefile
	@-$(call RM, $@)
	$(ASSEMBLE) --output_file=ti_tls_get_tp.obj

c62_TaskSupport_asm.obj: family/c62/TaskSupport_asm.s62 makefile
	@-$(call RM, $@)
	$(ASSEMBLE) --output_file=c62_TaskSupport_asm.obj

timer64_Timer_asm.obj: timers/timer64/Timer_asm.s64P makefile
	@-$(call RM, $@)
	$(ASSEMBLE) --output_file=timer64_Timer_asm.obj


BIOS.obj: BIOS.c family/c64p/EventCombiner.c family/c64p/Exception.c family/c64p/Hwi.c family/c64p/Hwi_startup.c family/c64p/TimestampProvider.c family/c64p/Cache.c knl/Clock.c knl/Idle.c knl/Intrinsics.c knl/Queue.c knl/Semaphore.c knl/Swi.c knl/Swi_andn.c knl/Task.c hal/Hwi.c hal/Hwi_stack.c hal/Hwi_startup.c io/DEV.c io/GIO.c io/IOM_stub.c rts/ti/ThreadLocalStorage.c family/c62/TaskSupport.c family/c62/IntrinsicsSupport.c gates/GateHwi.c gates/GateMutex.c xdcruntime/SemThreadSupport.c heaps/HeapMem.c timers/timer64/Timer.c family/c64p/primus/TimerSupport.c makefile
	@-$(call RM, $@)
	@echo cle674 $< ...
	@$(CC) $(BIOS_DEFS) $(XOPTS) $(INCS) \
	$(BIOS_ROOT)BIOS.c \
	$(BIOS_ROOT)family/c64p/EventCombiner.c \
	$(BIOS_ROOT)family/c64p/Exception.c \
	$(BIOS_ROOT)family/c64p/Hwi.c \
	$(BIOS_ROOT)family/c64p/Hwi_startup.c \
	$(BIOS_ROOT)family/c64p/TimestampProvider.c \
	$(BIOS_ROOT)family/c64p/Cache.c \
	$(BIOS_ROOT)knl/Clock.c \
	$(BIOS_ROOT)knl/Idle.c \
	$(BIOS_ROOT)knl/Intrinsics.c \
	$(BIOS_ROOT)knl/Queue.c \
	$(BIOS_ROOT)knl/Semaphore.c \
	$(BIOS_ROOT)knl/Swi.c \
	$(BIOS_ROOT)knl/Swi_andn.c \
	$(BIOS_ROOT)knl/Task.c \
	$(BIOS_ROOT)hal/Hwi.c \
	$(BIOS_ROOT)hal/Hwi_stack.c \
	$(BIOS_ROOT)hal/Hwi_startup.c \
	$(BIOS_ROOT)io/DEV.c \
	$(BIOS_ROOT)io/GIO.c \
	$(BIOS_ROOT)io/IOM_stub.c \
	$(BIOS_ROOT)rts/ti/ThreadLocalStorage.c \
	$(BIOS_ROOT)family/c62/TaskSupport.c \
	$(BIOS_ROOT)family/c62/IntrinsicsSupport.c \
	$(BIOS_ROOT)gates/GateHwi.c \
	$(BIOS_ROOT)gates/GateMutex.c \
	$(BIOS_ROOT)xdcruntime/SemThreadSupport.c \
	$(BIOS_ROOT)heaps/HeapMem.c \
	$(BIOS_ROOT)timers/timer64/Timer.c \
	$(BIOS_ROOT)family/c64p/primus/TimerSupport.c \

sysbios.ae674: BIOS.obj c64p_Exception_asm.obj c64p_Hwi_asm.obj c64p_Hwi_asm_switch.obj c64p_Hwi_disp_always.obj ti_tls_get_tp.obj c62_TaskSupport_asm.obj timer64_Timer_asm.obj
	@-$(call RM, $@)
	@echo are674 $^ ...
	@$(AR) $@ $^


clean:
	@$(DEL) ..\makefile.libs
	@-$(call RM, *)
