##########################################################
# PDM Makefile
# Target: STM32F103C8Tx, HAL, arm-none-eabi-gcc
# Build output: Debug/
##########################################################

TARGET    := PDM
BUILD_DIR := Debug

PREFIX  := arm-none-eabi-
CC      := $(PREFIX)gcc
AS      := $(PREFIX)gcc -x assembler-with-cpp
OBJCOPY := $(PREFIX)objcopy
SIZE    := $(PREFIX)size

CPU := -mcpu=cortex-m3
MCU := $(CPU) -mthumb

DEFS := \
  -DDEBUG \
  -DUSE_HAL_DRIVER \
  -DSTM32F103xB

INCLUDES := \
  -ICore/Inc \
  -IDrivers/STM32F1xx_HAL_Driver/Inc \
  -IDrivers/STM32F1xx_HAL_Driver/Inc/Legacy \
  -IDrivers/CMSIS/Device/ST/STM32F1xx/Include \
  -IDrivers/CMSIS/Include

ASM_SOURCES := STM32CubeIDE/Application/User/Startup/startup_stm32f103c8tx.s
LDSCRIPT := STM32CubeIDE/STM32F103C8TX_FLASH.ld

# Recursively collect .c sources using pure GNU Make (no external find dependency).
rwildcard = $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2) $(filter $(subst *,%,$2),$d))
C_SOURCES := $(call rwildcard,Core/Src/,*.c)
C_SOURCES += $(call rwildcard,Drivers/STM32F1xx_HAL_Driver/Src/,*.c)
C_SOURCES += $(call rwildcard,STM32CubeIDE/Application/User/Core/,*.c)

# Keep source paths in object names to avoid collisions.
OBJECTS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SOURCES))
OBJECTS += $(patsubst %.s,$(BUILD_DIR)/%.o,$(ASM_SOURCES))

CFLAGS  := $(MCU) $(DEFS) $(INCLUDES)
CFLAGS  += -std=gnu11 -Wall -Wextra
CFLAGS  += -ffunction-sections -fdata-sections
CFLAGS  += -g3 -O0
CFLAGS  += -MMD -MP

ASFLAGS := $(MCU) $(DEFS) $(INCLUDES) -g3

LDFLAGS := $(MCU)
LDFLAGS += -T$(LDSCRIPT)
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref
LDFLAGS += -specs=nano.specs -specs=nosys.specs
LDFLAGS += -lc -lm -lnosys

define MKDIR_P
powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$(subst /,\,$1)' | Out-Null"
endef

define RM_RF
powershell -NoProfile -Command "if (Test-Path '$(subst /,\,$1)') { Remove-Item -Recurse -Force '$(subst /,\,$1)' }"
endef

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) ; $(CC) $(OBJECTS) $(LDFLAGS) -o $@ && $(SIZE) $@
$(BUILD_DIR)/%.o: %.c ; @$(call MKDIR_P,$(dir $@)) && $(CC) -c $(CFLAGS) -o $@ $<
$(BUILD_DIR)/%.o: %.s ; @$(call MKDIR_P,$(dir $@)) && $(AS) -c $(ASFLAGS) -o $@ $<
$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf ; $(OBJCOPY) -O ihex $< $@
$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf ; $(OBJCOPY) -O binary -S $< $@

clean: ; @$(call RM_RF,$(BUILD_DIR))

.PHONY: all clean

-include $(OBJECTS:.o=.d)
