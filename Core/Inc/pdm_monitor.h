#ifndef PDM_MONITOR_H
#define PDM_MONITOR_H

#include <stdint.h>

typedef struct {
    float voltage_mV;
    float current_mA;
    float power_mW;
    float energy_mWh;
} pdm_channel_t;

extern volatile uint8_t g_alert1_flag;
extern volatile uint8_t g_alert2_flag;

void PDM_Monitor_Init(void);
void PDM_Monitor_Update(void);

#endif /* PDM_MONITOR_H */
