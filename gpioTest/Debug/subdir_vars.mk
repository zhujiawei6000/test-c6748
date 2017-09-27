################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Add inputs and outputs from these tool invocations to the build variables 
CFG_SRCS += \
../app.cfg 

C_SRCS += \
../GPIO_board.c \
../main_led_blink.c 

GEN_CMDS += \
./configPkg/linker.cmd 

GEN_FILES += \
./configPkg/linker.cmd \
./configPkg/compiler.opt 

GEN_MISC_DIRS += \
./configPkg/ 

C_DEPS += \
./GPIO_board.d \
./main_led_blink.d 

GEN_OPTS += \
./configPkg/compiler.opt 

OBJS += \
./GPIO_board.obj \
./main_led_blink.obj 

GEN_MISC_DIRS__QUOTED += \
"configPkg\" 

OBJS__QUOTED += \
"GPIO_board.obj" \
"main_led_blink.obj" 

C_DEPS__QUOTED += \
"GPIO_board.d" \
"main_led_blink.d" 

GEN_FILES__QUOTED += \
"configPkg\linker.cmd" \
"configPkg\compiler.opt" 

C_SRCS__QUOTED += \
"../GPIO_board.c" \
"../main_led_blink.c" 


