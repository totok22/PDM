#include "driver_ina226_interface.h"
#include "i2c.h"
#include "usart.h"
#include <stdarg.h>
#include <stdio.h>

uint8_t ina226_interface_iic_init(void)
{
    return 0;
}

uint8_t ina226_interface_iic_deinit(void)
{
    return 0;
}

uint8_t ina226_interface_iic_read(uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len)
{
    if (HAL_I2C_Master_Transmit(&hi2c1, addr, &reg, 1, 100) != HAL_OK)
    {
        return 1;
    }
    if (HAL_I2C_Master_Receive(&hi2c1, addr, buf, len, 100) != HAL_OK)
    {
        return 1;
    }
    return 0;
}

uint8_t ina226_interface_iic_write(uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len)
{
    uint8_t tmp[3];
    tmp[0] = reg;
    if (len > 2)
    {
        return 1;
    }
    for (uint16_t i = 0; i < len; i++)
    {
        tmp[1 + i] = buf[i];
    }
    if (HAL_I2C_Master_Transmit(&hi2c1, addr, tmp, (uint16_t)(1 + len), 100) != HAL_OK)
    {
        return 1;
    }
    return 0;
}

void ina226_interface_delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
}

void ina226_interface_debug_print(const char *const fmt, ...)
{
    char buf[128];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    if (len > 0)
    {
        HAL_UART_Transmit(&huart1, (uint8_t *)buf, (uint16_t)len, 100);
    }
}

void ina226_interface_receive_callback(uint8_t type)
{
    switch (type)
    {
        case INA226_STATUS_SHUNT_VOLTAGE_OVER_VOLTAGE:
            ina226_interface_debug_print("ALERT: shunt OV\r\n");
            break;
        case INA226_STATUS_SHUNT_VOLTAGE_UNDER_VOLTAGE:
            ina226_interface_debug_print("ALERT: shunt UV\r\n");
            break;
        case INA226_STATUS_BUS_VOLTAGE_OVER_VOLTAGE:
            ina226_interface_debug_print("ALERT: bus OV\r\n");
            break;
        case INA226_STATUS_BUS_VOLTAGE_UNDER_VOLTAGE:
            ina226_interface_debug_print("ALERT: bus UV\r\n");
            break;
        case INA226_STATUS_POWER_OVER_LIMIT:
            ina226_interface_debug_print("ALERT: power OL\r\n");
            break;
        default:
            ina226_interface_debug_print("ALERT: unknown %d\r\n", type);
            break;
    }
}
