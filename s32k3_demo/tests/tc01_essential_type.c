/* tc01_essential_type.c */
#include <stdint.h>

static uint16_t g_adc_raw;

uint16_t Tc01_GetScaled(void)
{
    int8_t offset = -1;
    /* signed/unsigned + 窄化风险： */
    g_adc_raw = (uint16_t)(g_adc_raw + offset);   /* 期望触发 essential_type */
    return (uint16_t)(g_adc_raw * 3 / 2);         /* 整数提升/截断风险 */
}

