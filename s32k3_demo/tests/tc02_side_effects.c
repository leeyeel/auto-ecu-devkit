/* tc02_side_effects.c */
#include <stdint.h>

static uint8_t g_idx;
static uint8_t g_buf[16];

void Tc02_Write(uint8_t v)
{
    /* 典型：副作用 + 多次求值风险 */
    g_buf[g_idx++] = (uint8_t)(v + g_idx++);
}

