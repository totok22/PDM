# 在 VSCode 中编译 PDM 完整配置步骤

> 本文档用于团队共享，内容已按当前项目 `PDM`（`STM32F103C8Tx + HAL`）实测更新。

## 0. 本机已验证工具路径（Windows）

```powershell
PS C:\WINDOWS\system32> where.exe make
C:\Users\Administrator\AppData\Local\UniGetUI\Chocolatey\bin\make.exe
```

```powershell
PS C:\WINDOWS\system32> where.exe arm-none-eabi-gcc
C:\ST\STM32CubeCLT_1.18.0\GNU-tools-for-STM32\bin\arm-none-eabi-gcc.exe
D:\EmbeddedToolchain\Arm_GNU_Toolchain\bin\arm-none-eabi-gcc.exe
```

```powershell
PS C:\WINDOWS\system32> where.exe STM32_Programmer_CLI
C:\ST\STM32CubeCLT_1.18.0\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe
```

```powershell
PS C:\WINDOWS\system32> where.exe ST-LINK_gdbserver
C:\ST\STM32CubeCLT_1.18.0\STLink-gdb-server\bin\ST-LINK_gdbserver.bat
C:\ST\STM32CubeCLT_1.18.0\STLink-gdb-server\bin\ST-LINK_gdbserver.exe
```

Git Bash 路径：

```text
C:\Program Files\Git\bin\bash.exe
```

## 1. 项目参数（从当前工程提取）

| 项目 | 值 |
| --- | --- |
| 工程名 | `PDM` |
| MCU | `STM32F103C8Tx`（Cortex-M3） |
| 库 | `STM32 HAL` |
| 启动文件 | `STM32CubeIDE/Application/User/Startup/startup_stm32f103c8tx.s` |
| 链接脚本 | `STM32CubeIDE/STM32F103C8TX_FLASH.ld` |
| 构建产物 | `Debug/PDM.elf/.hex/.bin/.map` |

## 2. VSCode 插件

建议安装：

- **C/C++**（Microsoft）
- **C/C++ Extension Pack**
- **Cortex-Debug**（需要调试时）

## 3. 根目录 Makefile

在项目根目录创建 `Makefile`，内容如下：

```makefile
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
```

## 4. `.vscode` 配置

在项目根目录创建 `.vscode/`，并创建以下文件。

### 4.1 `settings.json`（默认终端设置为 Git Bash）

```json
{
  "terminal.integrated.defaultProfile.windows": "Git Bash",
  "terminal.integrated.profiles.windows": {
    "Git Bash": {
      "path": "C:\\Program Files\\Git\\bin\\bash.exe"
    }
  }
}
```

### 4.2 `c_cpp_properties.json`

```json
{
  "configurations": [
    {
      "name": "STM32F103 (CubeCLT)",
      "includePath": [
        "${workspaceFolder}/Core/Inc",
        "${workspaceFolder}/Drivers/STM32F1xx_HAL_Driver/Inc",
        "${workspaceFolder}/Drivers/STM32F1xx_HAL_Driver/Inc/Legacy",
        "${workspaceFolder}/Drivers/CMSIS/Device/ST/STM32F1xx/Include",
        "${workspaceFolder}/Drivers/CMSIS/Include"
      ],
      "defines": [
        "DEBUG",
        "USE_HAL_DRIVER",
        "STM32F103xB"
      ],
      "compilerPath": "C:/ST/STM32CubeCLT_1.18.0/GNU-tools-for-STM32/bin/arm-none-eabi-gcc.exe",
      "cStandard": "gnu11",
      "cppStandard": "gnu++17",
      "intelliSenseMode": "gcc-arm",
      "browse": {
        "path": [
          "${workspaceFolder}/Core/Inc",
          "${workspaceFolder}/Drivers/STM32F1xx_HAL_Driver/Inc",
          "${workspaceFolder}/Drivers/STM32F1xx_HAL_Driver/Inc/Legacy",
          "${workspaceFolder}/Drivers/CMSIS/Device/ST/STM32F1xx/Include",
          "${workspaceFolder}/Drivers/CMSIS/Include"
        ],
        "limitSymbolsToIncludedHeaders": true
      }
    }
  ],
  "version": 4
}
```

### 4.3 `tasks.json`

```json
{
  "version": "2.0.0",
  "tasks": [
    {
      "label": "Build Debug",
      "type": "shell",
      "command": "C:/Users/Administrator/AppData/Local/UniGetUI/Chocolatey/bin/make.exe",
      "args": [
        "-j4"
      ],
      "options": {
        "cwd": "${workspaceFolder}",
        "env": {
          "PATH": "C:\\ST\\STM32CubeCLT_1.18.0\\GNU-tools-for-STM32\\bin;C:\\Users\\Administrator\\AppData\\Local\\UniGetUI\\Chocolatey\\bin;${env:PATH}"
        },
        "shell": {
          "executable": "C:\\Program Files\\Git\\bin\\bash.exe",
          "args": [
            "-lc"
          ]
        }
      },
      "group": {
        "kind": "build",
        "isDefault": true
      },
      "problemMatcher": [
        "$gcc"
      ]
    },
    {
      "label": "Clean",
      "type": "shell",
      "command": "C:/Users/Administrator/AppData/Local/UniGetUI/Chocolatey/bin/make.exe",
      "args": [
        "clean"
      ],
      "options": {
        "cwd": "${workspaceFolder}",
        "env": {
          "PATH": "C:\\ST\\STM32CubeCLT_1.18.0\\GNU-tools-for-STM32\\bin;C:\\Users\\Administrator\\AppData\\Local\\UniGetUI\\Chocolatey\\bin;${env:PATH}"
        },
        "shell": {
          "executable": "C:\\Program Files\\Git\\bin\\bash.exe",
          "args": [
            "-lc"
          ]
        }
      },
      "group": "build",
      "problemMatcher": []
    },
    {
      "label": "Flash Download",
      "type": "shell",
      "command": "C:\\ST\\STM32CubeCLT_1.18.0\\STM32CubeProgrammer\\bin\\STM32_Programmer_CLI.exe",
      "args": [
        "-c",
        "port=SWD",
        "freq=4000",
        "-w",
        "${workspaceFolder}/Debug/PDM.elf",
        "-v",
        "-rst"
      ],
      "options": {
        "cwd": "${workspaceFolder}"
      },
      "group": "build",
      "problemMatcher": []
    }
  ]
}
```

### 4.4 `launch.json`（可选调试）

```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "STM32 Debug (ST-Link)",
      "cwd": "${workspaceFolder}",
      "executable": "${workspaceFolder}/Debug/PDM.elf",
      "request": "launch",
      "type": "cortex-debug",
      "servertype": "stlink",
      "serverpath": "C:/ST/STM32CubeCLT_1.18.0/STLink-gdb-server/bin/ST-LINK_gdbserver.exe",
      "device": "STM32F103C8",
      "interface": "swd",
      "armToolchainPath": "C:/ST/STM32CubeCLT_1.18.0/GNU-tools-for-STM32/bin",
      "gdbPath": "C:/ST/STM32CubeCLT_1.18.0/GNU-tools-for-STM32/bin/arm-none-eabi-gdb.exe",
      "runToEntryPoint": "main",
      "preLaunchTask": "Build Debug"
    }
  ]
}
```

## 5. 编译与烧录

### 5.1 编译

- VSCode：按 `Ctrl + Shift + B`，执行默认任务 `Build Debug`
- 终端：在项目根目录执行

```bash
make -j4
```

编译成功后会在 `Debug/` 下生成：

- `PDM.elf`
- `PDM.hex`
- `PDM.bin`
- `PDM.map`

### 5.2 烧录

`Ctrl + Shift + P` -> `Tasks: Run Task` -> `Flash Download`

### 5.3 调试（Cortex-Debug）

安装 Cortex-Debug 后按 `F5`：

- 会先执行 `preLaunchTask: Build Debug`
- 然后通过 ST-Link GDB Server 进入调试

## 6. 团队共享注意事项

- 本文中的 `.exe` 路径是当前机器真实路径，可直接在本机使用。
- 团队成员若安装路径不同，只需修改：
  - `.vscode/tasks.json` 里的 `make.exe / bash.exe / STM32_Programmer_CLI.exe / PATH`
  - `.vscode/c_cpp_properties.json` 的 `compilerPath`
  - `.vscode/launch.json` 的 `serverpath / armToolchainPath / gdbPath`
- `Makefile` 中源码路径、启动文件、链接脚本路径已经按本项目固定，不需要改。

## 7. 常见问题

### 7.1 `No such file or directory`（工具路径）

优先检查 `tasks.json` 里命令路径是否与本机一致。

### 7.2 `undefined reference`（链接错误）

确认启动文件和链接脚本使用的是：

- `STM32CubeIDE/Application/User/Startup/startup_stm32f103c8tx.s`
- `STM32CubeIDE/STM32F103C8TX_FLASH.ld`

### 7.3 VSCode 找不到头文件

检查 `c_cpp_properties.json` 的 `includePath` 与项目目录是否一致，修改后执行一次 `C/C++: Reset IntelliSense Database`。
