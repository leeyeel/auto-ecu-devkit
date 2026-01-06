/* tc04_pointers_init.c */
#include <stdint.h>

typedef struct {
    uint16_t a;
    uint16_t b;
} Frame_t;

static Frame_t g_frame;

void Tc04_Copy(Frame_t *dst, const Frame_t *src)
{
    /* 故意不做 NULL 检查，且使用未初始化字段 */
    dst->a = src->a;
    dst->b = g_frame.b;   /* g_frame.b 未初始化时风险 */
}

