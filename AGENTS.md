# PDM - FSAE 低压配电模块

## 项目概述
STM32F103 低压 PDM (Power Distribution Module)，用于 FSAE 赛车低压系统电源监控。

## 硬件配置
- MCU: STM32F103 (72MHz HSE+PLL)
- INA226 #1 (0x40): 低压总线侧，采样电阻 4mΩ
- INA226 #2 (0x41): 电池侧，采样电阻 4mΩ
- CAN 500kbps，上报频率 2Hz
- ALERT1: PA1 (EXTI1)，ALERT2: PA3 (EXTI3)
- LED 心跳: PB5

## 关键文件
- `Core/Src/pdm_monitor.c` / `Core/Inc/pdm_monitor.h` — 双 INA226 管理、能量累计、CAN 发送
- `Core/Src/ina226_interface.c` — HAL I2C 胶水层
- `Core/Src/driver_ina226.c` / `Core/Inc/driver_ina226.h` — LibDriver INA226 驱动（从 Reference 复制）
- `Core/Src/main.c` — CAN 过滤器配置 + PDM_Monitor_Init/Update 调用
- `Core/Src/stm32f1xx_it.c` — EXTI 回调设置 alert 标志

## CAN 报文协议
- 0x300: 低压总线侧，0x301: 电池侧
- 8 字节大端序: 电压(1mV/LSB) | 电流(10mA/LSB) | 功率(100mW/LSB) | 耗电量(10mWh/LSB)

## 编码规范
- 只在 `USER CODE BEGIN/END` 块内修改 CubeMX 生成的文件
- INA226 驱动库不做修改，接口适配在 ina226_interface.c 中完成
- 使用中文注释


## 其他
- Documentation and code comments must be standardized, clear, readable, and straightforward. Avoid using jargon or difficult-to-understand, obscure, or abstract terms. Examples of what to avoid: 门控/语义/消费/四元组/降载/闸/锚点/落盘/脏/原子/临界区/边界/服务化/快照/污染...等等
- Don't try to build or compile yourself. I will do it for you.
- Let me handle the commit and push, you just need to tell me the commit message description.