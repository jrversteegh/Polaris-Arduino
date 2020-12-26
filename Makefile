SRC := Tracker
MAIN := Tracker.ino
ARDUINO := ${HOME}/Applications/arduino
LOCAL_ARDUINO := ${HOME}/.arduino15
BUILD := .
BOARD := fortebit_openiot:stm32:polaris_1:hwrev=V10,modem=UG96,upload_method=STM32CubeDFU,usb=CDC,opt=o2std,rtlib=nanofps
VID_PID := 0483_5740

BUILDER := ${ARDUINO}/arduino-builder
TOOLS_BUILDER := ${ARDUINO}/tools-builder
HARDWARE := ${ARDUINO}/hardware
LOCAL_PACKAGES := ${LOCAL_ARDUINO}/packages
STM32_TOOLS := ${LOCAL_PACKAGES}/STM32/tools
STM32_TOOLS_TOOLS := ${STM32_TOOLS}/STM32Tools/1.0.2
STM32_GCC_PATH := ${STM32_TOOLS}/arm-none-eabi-gcc/6.-2017-q2-update
CMSIS := ${STM32_TOOLS}/CMSIS/5.3.0
LIBRARIES := ${ARDUINO}/libraries
BUILDER_FLAGS := -logger=machine -tools ${TOOLS_BUILDER} -tools ${LOCAL_PACKAGES} -hardware ${HARDWARE} -hardware ${LOCAL_PACKAGES} -fqbn=${BOARD} -vid-pid=${VID_PID} -warnings=default -build-path ${BUILD} -build-cache ${BUILD}/cache -prefs=build.warn_data_percentage=75 -prefs=runtime.tools.CMSIS.path=${CMSIS} -prefs=runtime.tools.CMSIS-5.3.0.path=${CMSIS} -prefs=runtime.tools.STM32Tools.path=${STM32_TOOLS_TOOLS} -prefs=runtime.tools.STM32Tools-1.0.2.path=${STM32_TOOLS_TOOLS} -prefs=runtime.tools.arm-none-eabi-gcc.path=${STM32_GCC_PATH} -prefs=runtime.tools.arm-none-eabi-gcc-6-2017-q2-update.path=${STM32_GCC_PATH} -verbose -libraries ${LIBRARIES} -built-in-libraries ${LIBRARIES}


MAKEFILE_DIR := $(dir $(realpath $(firstword $(MAKEFILE_LIST))))
SRC_DIR := ${MAKEFILE_DIR}/${SRC}
FULL_MAIN := ${SRC_DIR}/${MAIN}

all:
	${BUILDER} ${BUILDER_FLAGS} ${FULL_MAIN}
