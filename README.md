# PDM - FSAE 低压配电模块电源监控

基于 STM32F103C8T6 的 FSAE 低压 PDM 电源监控固件，通过双路 INA226 实时采集电压/电流/功率，累计耗电量，并通过 CAN 总线上报数据。

---

## 硬件概述

| 模块 | 说明 |
|------|------|
| MCU | STM32F103C8T6，8MHz HSE + PLL = 72MHz |
| INA226 #1 (0x40) | 低压总线侧（DCDC + 电池 OR-RING 汇合后），采样电阻 4mΩ |
| INA226 #2 (0x41) | 低压电池侧（7串磷酸铁锂），采样电阻 4mΩ |
| CAN | 500kbps，SN65HVD230 收发器，PA11/PA12 |
| I2C | PB6(SCL)/PB7(SDA)，4.7kΩ 上拉 |
| UART1 | PA9(TX)/PA10(RX)，调试输出 |
| ALERT1 | PA1，INA226 #1 告警中断，10kΩ 上拉 |
| ALERT2 | PA3，INA226 #2 告警中断，10kΩ 上拉 |
| LED | PB5，低电平点亮，1kΩ 限流，心跳指示 |

---

## 软件架构

```
Core/
├── Inc/
│   ├── driver_ina226.h            # LibDriver INA226 驱动头文件
│   ├── driver_ina226_interface.h  # 平台接口声明
│   └── pdm_monitor.h              # 公开 API
└── Src/
    ├── driver_ina226.c            # LibDriver INA226 驱动实现
    ├── ina226_interface.c         # HAL I2C/UART 胶水层
    ├── pdm_monitor.c              # 双通道管理、能量累计、CAN 发送
    └── main.c                     # 初始化入口 + 主循环
```

---

## CAN 报文协议

| CAN ID | 通道 |
|--------|------|
| `0x300` | 低压总线侧 (INA226 #1) |
| `0x301` | 电池侧 (INA226 #2) |

帧格式（8 字节，大端序）：

| 字节 | 字段 | 类型 | 单位 | 比例 |
|------|------|------|------|------|
| 0-1 | 电压 | int16 | mV | 1 mV/LSB |
| 2-3 | 电流 | int16 | mA | 10 mA/LSB |
| 4-5 | 功率 | uint16 | mW | 100 mW/LSB |
| 6-7 | 累计耗电量 | uint16 | mWh | 10 mWh/LSB（滚动，上限 655.35 Wh） |

解码示例（Python）：
```python
import struct
data = bytes([...])  # 8字节 CAN 数据
voltage_mV, current_x10, power_x100, energy_x10 = struct.unpack('>hhHH', data)
current_mA = current_x10 * 10
power_mW   = power_x100 * 100
energy_mWh = energy_x10 * 10
```

---

## 定时任务

| 周期 | 任务 |
|------|------|
| 50ms | 读取两路 INA226（电压/电流/功率），累计能量 |
| 500ms | 发送 CAN 0x300 + 0x301，翻转 LED 心跳 |
| 1000ms | UART 打印调试信息 |
| 异步 | ALERT 中断触发后在主循环调用 `ina226_irq_handler` |

---

## INA226 配置

| 参数 | 值 |
|------|----|
| 平均次数 | 16 次 |
| 总线电压转换时间 | 1.1ms |
| 分流电压转换时间 | 1.1ms |
| 工作模式 | 连续测量（分流 + 总线） |
| 采样电阻 | 4mΩ |

---

## UART 调试输出格式

波特率默认由 CubeMX 配置（通常 115200）。每秒输出一行：

```
BUS: 24000mV 1500.0mA 36000.0mW 18.0mWh | BAT: 22800mV 1480.0mA 33744.0mW 16.9mWh
```

---

## 编译与烧录

1. 用 STM32CubeIDE 打开项目根目录
2. 确认 `Core/Src/` 和 `Core/Inc/` 下包含所有新增文件（CubeIDE 通常自动扫描）
3. Build → Flash

> 注意：CubeMX 重新生成代码时只会覆盖 `USER CODE BEGIN/END` 块之外的内容，所有修改均在 USER CODE 块内，安全。

---

## 注意事项

- INA226 I2C 地址在驱动中已左移一位（`0x40 << 1 = 0x80`），与 HAL 格式一致，无需手动处理
- CAN 发送前检查 TX mailbox 是否空闲，总线无 ACK 时会打印告警而不阻塞
- 耗电量字段在 655.35 Wh 时滚动归零（uint16 自然溢出）
- LED 低电平点亮，`HAL_GPIO_TogglePin` 每 500ms 翻转一次，即 1s 周期闪烁
