   在 `.vscode/tasks.json` 中把 `make.exe`、`bash.exe` 和 `STM32CubeCLT` 的路径替换为电脑上的实际安装路径。

   例如：

   "C:\Program Files\Git\bin\bash.exe"
   ```
   PS C:\WINDOWS\system32> where.exe make
   C:\Users\Administrator\AppData\Local\UniGetUI\Chocolatey\bin\make.exe
   ```

   ```
   C:\Users\Administrator>where arm-none-eabi-gcc
   C:\ST\STM32CubeCLT_1.18.0\GNU-tools-for-STM32\bin\arm-none-eabi-gcc.exe
   D:\EmbeddedToolchain\Arm_GNU_Toolchain\bin\arm-none-eabi-gcc.exe
   ```

   `Terminal > Integrated > Default Profile: Windows` 用来修改 Windows 系统下 VS Code 默认终端的设置。下拉菜单选 **“Git Bash”**。（需要先安装Git）

   ## 在 VSCode 中编译 BMS_MASTER 完整配置步骤

   ### 前置信息（从项目中提取）

   | 项目     | 值                                                   |
   | -------- | ---------------------------------------------------- |
   | MCU      | STM32F105RBTx（Cortex-M3）                           |
   | 外设库   | STM32F10x SPL（标准外设库）                          |
   | 启动文件 | `Startup/startup_stm32f105rbtx.s`（GCC格式，已就绪） |
   | 链接脚本 | `STM32F105RBTX_FLASH.ld`（已就绪）                   |
   | 编译器   | arm-none-eabi-gcc                                    |
   | 构建工具 | GNU Make                                             |

   ## 第一步：安装工具链

   ### 1.1 安装 arm-none-eabi-gcc

   **Windows：**
   - 下载 [Arm GNU Toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads)，选择 `arm-none-eabi` 的 Windows installer，例如：`arm-gnu-toolchain-15.2.rel1-mingw-w64-x86_64-arm-none-eabi.msi`
   - 安装时**勾选"Add to PATH"**，没有就手动添加。

   **macOS：**
   ```bash
   brew install --cask gcc-arm-embedded
   ```

   **Linux（Ubuntu/Debian）：**
   ```bash
   sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi
   ```

   ### 1.2 安装 Make

   **Windows：**
   - 安装 [Chocolatey](https://chocolatey.org/)，然后：
   ```bash
     choco # 如何显示已安装则跳过下面这行安装
   ```

   ```bash
     Set-ExecutionPolicy Bypass -Scope Process -Force; [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072; iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))
   ```

   ```bash
     choco install make -y
   ```

   **macOS：**
   ```bash
   brew install make
   ```

   **Linux：**
   ```bash
   sudo apt install make
   ```

   ### 1.3 验证安装
   ```bash
   arm-none-eabi-gcc --version
   make --version
   ```


   ## 第二步：安装 VSCode 插件

   在 VSCode 扩展市场中安装：

   | 插件                     | 用途                       |
   | ------------------------ | -------------------------- |
   | **C/C++**（Microsoft）   | 代码补全、语法高亮、跳转   |
   | **C/C++ Extension Pack** | 包含上面及更多工具         |
   | **Cortex-Debug**（可选） | 调试 STM32（如需烧录调试） |


   ## 第三步：创建 Makefile

   在项目**根目录**创建 `Makefile` 文件，内容完整如下：

   ```makefile
   ##########################################################
   # BMS_MASTER Makefile
   # Target: STM32F105RBTx, SPL, arm-none-eabi-gcc
   # Build output: Debug/
   # Shell: Git Bash (needs: find, mkdir -p, rm -rf)
   ##########################################################
   TARGET    := BMS_Master
   BUILD_DIR := Debug
   # Toolchain (will be found via PATH set by VSCode task)
   PREFIX  := arm-none-eabi-
   CC      := $(PREFIX)gcc
   AS      := $(PREFIX)gcc -x assembler-with-cpp
   OBJCOPY := $(PREFIX)objcopy
   SIZE    := $(PREFIX)size
   CPU := -mcpu=cortex-m3
   MCU := $(CPU) -mthumb
   DEFS := \
     -DDEBUG \
     -DSTM32 \
     -DSTM32F1 \
     -DSTM32F105RBTx \
     -DSTM32F10X_CL \
     -DUSE_STDPERIPH_DRIVER \
     '-D__weak=__attribute__((weak))' \
     '-D__packed=__attribute__((__packed__))'
   INCLUDES := \
     -ICORE \
     -IStartup \
     -IUSER \
     -IUSMART \
     -ISYSTEM/delay \
     -ISYSTEM/sys \
     -ISYSTEM/usart \
     -IHARDWARE/ADC \
     -IHARDWARE/ADS7841 \
     -IHARDWARE/CAN \
     -IHARDWARE/DATA \
     -IHARDWARE/DS18B20 \
     -IHARDWARE/EXTI \
     -IHARDWARE/KEY \
     -IHARDWARE/LCD \
     -IHARDWARE/LED \
     -IHARDWARE/RTC \
     -IHARDWARE/SPI \
     -IHARDWARE/TIMER \
     -IHARDWARE/W25QXX \
     -IHARDWARE/WDG \
     -ISTM32F10x_FWLib/inc
   # Only one startup file to avoid duplicate vectors/Reset_Handler
   ASM_SOURCES := Startup/startup_stm32f105rbtx.s
   # Linker script (your actual filename)
   LDSCRIPT := STM32F105RBTX_FLASH.ld
   CFLAGS  := $(MCU) $(DEFS) $(INCLUDES)
   CFLAGS  += -Wall -Wextra
   CFLAGS  += -fdata-sections -ffunction-sections
   CFLAGS  += -g3 -O0
   CFLAGS  += -std=gnu11
   ASFLAGS := $(MCU) $(DEFS) $(INCLUDES) -g3
   LDFLAGS := $(MCU)
   LDFLAGS += -T$(LDSCRIPT)
   LDFLAGS += -Wl,--gc-sections
   LDFLAGS += -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref
   LDFLAGS += -specs=nano.specs
   LDFLAGS += -lc -lm -lnosys
   LDFLAGS += -u _printf_float
   LDFLAGS += -u _scanf_float
   # Recursively find all .c sources (Git Bash provides find)
   C_SOURCES := $(shell find USER USMART SYSTEM HARDWARE CORE STM32F10x_FWLib/src -name "*.c" 2>/dev/null)
   # Keep paths to avoid same-name collisions
   OBJECTS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SOURCES))
   OBJECTS += $(patsubst %.s,$(BUILD_DIR)/%.o,$(ASM_SOURCES))
   all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin
   $(BUILD_DIR)/$(TARGET).elf: $(OBJECTS)
     $(CC) $(OBJECTS) $(LDFLAGS) -o $@
     $(SIZE) $@
   $(BUILD_DIR)/%.o: %.c
     @mkdir -p $(dir $@)
     $(CC) -c $(CFLAGS) -o $@ $<
   $(BUILD_DIR)/%.o: %.s
     @mkdir -p $(dir $@)
     $(AS) -c $(ASFLAGS) -o $@ $<
   $(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf
     $(OBJCOPY) -O ihex $< $@
   $(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
     $(OBJCOPY) -O binary -S $< $@
   clean:
     rm -rf $(BUILD_DIR)
   .PHONY: all clean
   ```

   ---

   ## 第四步：配置 VSCode

   ### 4.1 配置 C/C++ 智能提示（`.vscode/c_cpp_properties.json`）

   在项目根目录创建 `.vscode/` 文件夹，新建 `c_cpp_properties.json`：

   ```json
   {
     "configurations": [
       {
         "name": "STM32F105 (CubeCLT)",
         "includePath": [
           "${workspaceFolder}/CORE",
           "${workspaceFolder}/Startup",
           "${workspaceFolder}/USER",
           "${workspaceFolder}/USMART",
           "${workspaceFolder}/SYSTEM/**",
           "${workspaceFolder}/HARDWARE/**",
           "${workspaceFolder}/STM32F10x_FWLib/inc"
         ],
         "defines": [
           "DEBUG",
           "STM32",
           "STM32F1",
           "STM32F105RBTx",
           "STM32F10X_CL",
           "USE_STDPERIPH_DRIVER",
           "__weak=__attribute__((weak))",
           "__packed=__attribute__((__packed__))"
         ],
         "compilerPath": "C:/ST/STM32CubeCLT_1.18.0/GNU-tools-for-STM32/bin/arm-none-eabi-gcc.exe",
         "cStandard": "gnu11",
         "cppStandard": "gnu++17",
         "intelliSenseMode": "gcc-arm",
         "browse": {
           "path": [
             "${workspaceFolder}/CORE",
             "${workspaceFolder}/Startup",
             "${workspaceFolder}/USER",
             "${workspaceFolder}/USMART",
             "${workspaceFolder}/SYSTEM",
             "${workspaceFolder}/HARDWARE",
             "${workspaceFolder}/STM32F10x_FWLib/inc"
           ],
           "limitSymbolsToIncludedHeaders": true
         }
       }
     ],
     "version": 4
   }
   ```

   ### 4.2 配置构建任务（`.vscode/tasks.json`）

   ```json
   {
     "version": "2.0.0",
     "tasks": [
       {
         "label": "Build Debug",
         "type": "shell",
         "command": "C:\\Users\\Administrator\\AppData\\Local\\UniGetUI\\Chocolatey\\bin\\make.exe",
         "args": [
           "-j4"
         ],
         "options": {
           "cwd": "${workspaceFolder}",
           "env": {
             "PATH": "C:\\ST\\STM32CubeCLT_1.18.0\\GNU-tools-for-STM32\\bin;${env:PATH}"
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
         "command": "C:\\Users\\Administrator\\AppData\\Local\\UniGetUI\\Chocolatey\\bin\\make.exe",
         "args": [
           "clean"
         ],
         "options": {
           "cwd": "${workspaceFolder}",
           "env": {
             "PATH": "C:\\ST\\STM32CubeCLT_1.18.0\\GNU-tools-for-STM32\\bin;${env:PATH}"
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
           "${workspaceFolder}/Debug/BMS_Master.elf",
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


   ## 第五步：编译

   ### 方法一：VSCode 快捷键
   按 `Ctrl+Shift+B`（macOS: `Cmd+Shift+B`）直接触发默认构建任务。

   ### 方法二：终端
   ```bash
   # 在项目根目录打开终端
   make -j4
   ```

   编译成功后在 `Debug/` 目录下生成：
   - `BMS_Master.elf` — 调试用
   - `BMS_Master.hex` — 烧录用
   - `BMS_Master.bin` — 烧录用
   - `BMS_Master.map` — 内存分布图


   ## 烧录

   按 `Ctrl + Shift + P` -> `Tasks: Run Task` -> `Flash Download` 即可完成烧录

   ## 调试（可选）

   要实现像 IDE 那样的断点调试，需要安装 VS Code 插件 **Cortex-Debug**。
   安装后，在项目根目录创建 `.vscode/launch.json`，并填入以下配置：

   ```JSON
   {
     "version": "0.2.0",
     "configurations": [
       {
         "name": "STM32 Debug (ST-Link)",
         "cwd": "${workspaceFolder}",
         "executable": "${workspaceFolder}/Debug/BMS_Master.elf",
         "request": "launch",
         "type": "cortex-debug",
         "servertype": "stlink",
         "device": "STM32F105RB",
         "interface": "swd",
         "runToEntryPoint": "main",
         "svdFile": "STM32F105xx.svd", // 可选，用于查看寄存器
         "preLaunchTask": "Build Debug" // 调试前自动运行编译任务
       }
     ]
   }
   ```

   - **核心点**：
       - `servertype`：设置为 `stlink`（VS Code 会自动寻找 ST-Link GDB Server）。
       - `preLaunchTask`：这能确保按 **F5** 时，它先把代码编译好，再烧录并进入调试状态。
       **F5 一键调试**：使用 **Cortex-Debug** 插件和 ST-Link 调试器进行调试，按 F5 自动编译、烧录并进入调试状态。

   首次按下 F5 启动调试前，请确保 Cortex-Debug 插件能够在系统路径中找到 `st-util`（ST-Link GDB Server）。由于我们使用的是 STM32CubeCLT，可能需要在 VS Code 设置 (`settings.json`) 中显式指定 Server 路径，或者将 `C:\ST\STM32CubeCLT_1.18.0\STLink-gdb-server\bin` 添加到 Windows 环境变量中。
   ## .bashrc（可选）

   在 Git Bash 中输入：`code ~/.bashrc`

   ```Bash
   # 1. 基础操作优化
   # 让 ls 命令显示颜色，区分文件夹和文件
   alias ls='ls --color=auto'
   # ll 显示详细列表，la 显示包含隐藏文件的详细列表
   alias ll='ls -l'
   alias la='ls -la'
   # 快速清屏 (习惯 Linux 的人常用)
   alias c='clear'
   
   # 2. 目录导航 (Windows 特供)
   # 输入 '..' 返回上一级，'...' 返回上两级
   alias ..='cd ..'
   alias ...='cd ../..'
   
   # Git Bash 中直接输 explorer . 有时会卡住或失效，用这个函数更稳
   function open() {
       if [ -z "$1" ]; then
           explorer.exe .
       else
           explorer.exe $(cygpath -w "$1")
       fi
   }
   # 用法：输入 `open` 即可弹窗打开当前文件夹
   
   # 3. Git 常用简写 
   alias gs='git status'
   alias ga='git add .'
   alias gc='git commit -m'
   alias gp='git push'
   alias gl='git log --oneline --graph --decorate'
   
   # 4. Node.js 开发常用
   alias ni='npm install'
   alias ns='npm start'
   alias nd='npm run dev'
   # 删除 node_modules (Windows下删这个很慢，用 rimraf 更快，前提是你装了 rimraf)
   # 或者利用 bash 的 rm -rf 也是极快的
   alias nuke='rm -rf node_modules && npm install'
   ```

   保存并运行 `source ~/.bashrc` 使其生效。

   ## 常见问题排查

   ### ❶ `__weak` / `__packed` 宏冲突报错
   `.cproject` 已通过 `-D'__weak=...'` 的方式处理，Makefile 中已照搬，应无问题。若还报错，在相关头文件顶部确认没有重复定义。

   ### ❷ `undefined reference to SystemInit`
   `SystemInit` 在 `SYSTEM/sys/` 目录的源文件中定义，检查该目录是否被 `wildcard` 正确扫描到，必要时手动添加：
   ```makefile
   C_SOURCES += SYSTEM/sys/sys.c
   ```

   ### ❸ Windows 路径问题
   Makefile 中的路径使用 `/` 而非 `\`，在 Windows 上需要用 Git Bash、MSYS2 或 WSL 来执行 make，**不要用 cmd.exe**。

   ### ❹ `Startup` 目录启动文件
   项目已有 GCC 格式的 `Startup/startup_stm32f105rbtx.s`（由 STM32CubeIDE 自动生成），**不要**使用 `CORE/` 目录下 Keil 格式的 `.s` 文件（Makefile 中已排除它们）。