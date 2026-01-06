/* tc05_macros.c */
#include <stdint.h>

#define SCALE(x)  x*10U  /* 缺括号 */
#define MIN(a,b)  ((a)<(b)?(a):(b))  /* 多次求值 */

uint32_t Tc05_Test(uint32_t v)
{
    uint32_t x = SCALE(v + 1U);     /* 触发宏括号问题 */
    uint32_t y = MIN(v++, 100U);    /* 触发多次求值 + 副作用 */
    return x + y;
}

