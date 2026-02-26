#include "pdm_monitor.h"
#include "driver_ina226.h"
#include "driver_ina226_interface.h"
#include "stm32f1xx_hal.h"
#include "can.h"
#include "main.h"
#include <stdio.h>
#include <string.h>

/* CAN IDs */
#define CAN_ID_BUS    0x300
#define CAN_ID_BAT    0x301

/* Timing intervals (ms) */
#define INTERVAL_READ   50
#define INTERVAL_CAN    500
#define INTERVAL_UART   1000

/* Two INA226 handles */
static ina226_handle_t g_ina226_bus;
static ina226_handle_t g_ina226_bat;

/* Channel data */
static pdm_channel_t g_ch_bus;
static pdm_channel_t g_ch_bat;

/* Timing */
static uint32_t g_last_read_tick;
static uint32_t g_last_can_tick;
static uint32_t g_last_uart_tick;

/* Alert flags (set in EXTI callback) */
volatile uint8_t g_alert1_flag = 0;
volatile uint8_t g_alert2_flag = 0;

/* --- Helper: link interface functions to a handle --- */
static void link_handle(ina226_handle_t *h)
{
    DRIVER_INA226_LINK_INIT(h, ina226_handle_t);
    DRIVER_INA226_LINK_IIC_INIT(h, ina226_interface_iic_init);
    DRIVER_INA226_LINK_IIC_DEINIT(h, ina226_interface_iic_deinit);
    DRIVER_INA226_LINK_IIC_READ(h, ina226_interface_iic_read);
    DRIVER_INA226_LINK_IIC_WRITE(h, ina226_interface_iic_write);
    DRIVER_INA226_LINK_DELAY_MS(h, ina226_interface_delay_ms);
    DRIVER_INA226_LINK_DEBUG_PRINT(h, ina226_interface_debug_print);
    DRIVER_INA226_LINK_RECEIVE_CALLBACK(h, ina226_interface_receive_callback);
}

/* --- Init one INA226 --- */
static uint8_t init_one(ina226_handle_t *h, ina226_address_t addr)
{
    uint8_t res;
    uint16_t cal;

    ina226_set_addr_pin(h, addr);
    ina226_set_resistance(h, 0.004);

    res = ina226_init(h);
    if (res != 0) return res;

    res = ina226_set_average_mode(h, INA226_AVG_16);
    if (res != 0) return res;

    res = ina226_set_bus_voltage_conversion_time(h, INA226_CONVERSION_TIME_1P1_MS);
    if (res != 0) return res;

    res = ina226_set_shunt_voltage_conversion_time(h, INA226_CONVERSION_TIME_1P1_MS);
    if (res != 0) return res;

    res = ina226_calculate_calibration(h, &cal);
    if (res != 0) return res;

    res = ina226_set_calibration(h, cal);
    if (res != 0) return res;

    res = ina226_set_mode(h, INA226_MODE_SHUNT_BUS_VOLTAGE_CONTINUOUS);
    return res;
}

/* --- Read one channel --- */
static void read_channel(ina226_handle_t *h, pdm_channel_t *ch, float dt_h)
{
    uint16_t raw_v, raw_p;
    int16_t raw_i;
    float v, i, p;

    if (ina226_read_bus_voltage(h, &raw_v, &v) == 0)
        ch->voltage_mV = v;

    if (ina226_read_current(h, &raw_i, &i) == 0)
        ch->current_mA = i;

    if (ina226_read_power(h, &raw_p, &p) == 0)
    {
        ch->power_mW = p;
        ch->energy_mWh += ch->power_mW * dt_h;
    }
}

/* --- Send one CAN frame --- */
static void send_can_frame(uint32_t id, pdm_channel_t *ch)
{
    if (HAL_CAN_GetTxMailboxesFreeLevel(&hcan) == 0)
    {
        ina226_interface_debug_print("CAN TX full, drop 0x%03lX\r\n", id);
        return;
    }

    CAN_TxHeaderTypeDef hdr;
    uint8_t data[8];
    uint32_t mailbox;

    hdr.StdId = id;
    hdr.ExtId = 0;
    hdr.IDE = CAN_ID_STD;
    hdr.RTR = CAN_RTR_DATA;
    hdr.DLC = 8;
    hdr.TransmitGlobalTime = DISABLE;

    /* Saturating casts */
    float v_mv = ch->voltage_mV;
    if (v_mv > 32767.0f) v_mv = 32767.0f;
    if (v_mv < -32768.0f) v_mv = -32768.0f;
    int16_t voltage = (int16_t)v_mv;

    float i_scaled = ch->current_mA / 10.0f;
    if (i_scaled > 32767.0f) i_scaled = 32767.0f;
    if (i_scaled < -32768.0f) i_scaled = -32768.0f;
    int16_t current = (int16_t)i_scaled;

    float p_scaled = ch->power_mW / 100.0f;
    if (p_scaled > 65535.0f) p_scaled = 65535.0f;
    if (p_scaled < 0.0f) p_scaled = 0.0f;
    uint16_t power = (uint16_t)p_scaled;

    float e_scaled = ch->energy_mWh / 10.0f;
    if (e_scaled > 65535.0f) e_scaled = 65535.0f;
    if (e_scaled < 0.0f) e_scaled = 0.0f;
    uint16_t energy = (uint16_t)e_scaled;

    data[0] = (uint8_t)(voltage >> 8);
    data[1] = (uint8_t)(voltage & 0xFF);
    data[2] = (uint8_t)(current >> 8);
    data[3] = (uint8_t)(current & 0xFF);
    data[4] = (uint8_t)(power >> 8);
    data[5] = (uint8_t)(power & 0xFF);
    data[6] = (uint8_t)(energy >> 8);
    data[7] = (uint8_t)(energy & 0xFF);

    if (HAL_CAN_AddTxMessage(&hcan, &hdr, data, &mailbox) != HAL_OK)
    {
        ina226_interface_debug_print("CAN TX err 0x%03lX\r\n", id);
    }
}

/* --- Public API --- */

void PDM_Monitor_Init(void)
{
    memset(&g_ch_bus, 0, sizeof(g_ch_bus));
    memset(&g_ch_bat, 0, sizeof(g_ch_bat));

    link_handle(&g_ina226_bus);
    link_handle(&g_ina226_bat);

    if (init_one(&g_ina226_bus, INA226_ADDRESS_0) != 0)
    {
        ina226_interface_debug_print("INA226 #1 (bus) init FAIL\r\n");
    }
    if (init_one(&g_ina226_bat, INA226_ADDRESS_1) != 0)
    {
        ina226_interface_debug_print("INA226 #2 (bat) init FAIL\r\n");
    }

    uint32_t now = HAL_GetTick();
    g_last_read_tick = now;
    g_last_can_tick  = now;
    g_last_uart_tick = now;

    ina226_interface_debug_print("PDM Monitor initialized\r\n");
}

void PDM_Monitor_Update(void)
{
    uint32_t now = HAL_GetTick();

    /* 50ms: read sensors */
    if (now - g_last_read_tick >= INTERVAL_READ)
    {
        float dt_h = (float)(now - g_last_read_tick) / 3600000.0f;
        g_last_read_tick = now;
        read_channel(&g_ina226_bus, &g_ch_bus, dt_h);
        read_channel(&g_ina226_bat, &g_ch_bat, dt_h);
    }

    /* 500ms: CAN + LED heartbeat */
    if (now - g_last_can_tick >= INTERVAL_CAN)
    {
        g_last_can_tick = now;
        send_can_frame(CAN_ID_BUS, &g_ch_bus);
        send_can_frame(CAN_ID_BAT, &g_ch_bat);
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    }

    /* 1000ms: UART debug */
    if (now - g_last_uart_tick >= INTERVAL_UART)
    {
        g_last_uart_tick = now;
        ina226_interface_debug_print(
            "BUS: %.0fmV %.1fmA %.1fmW %.1fmWh | "
            "BAT: %.0fmV %.1fmA %.1fmW %.1fmWh\r\n",
            g_ch_bus.voltage_mV, g_ch_bus.current_mA,
            g_ch_bus.power_mW, g_ch_bus.energy_mWh,
            g_ch_bat.voltage_mV, g_ch_bat.current_mA,
            g_ch_bat.power_mW, g_ch_bat.energy_mWh);
    }

    /* Alert handling */
    if (g_alert1_flag)
    {
        g_alert1_flag = 0;
        ina226_irq_handler(&g_ina226_bus);
    }
    if (g_alert2_flag)
    {
        g_alert2_flag = 0;
        ina226_irq_handler(&g_ina226_bat);
    }
}
