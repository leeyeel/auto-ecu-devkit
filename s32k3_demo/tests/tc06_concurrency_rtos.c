/* tc06_concurrency_rtos.c */
#include <stdint.h>
#include <stdbool.h>

static bool g_spi_done;          /* ISR 与 Task 共享 */
static uint8_t g_spi_status;

void Spi_IsrDone(uint8_t st)     /* 假设在中断上下文调用 */
{
    g_spi_status = st;
    g_spi_done = true;          /* 无内存屏障/无原子/无 volatile */
}

bool Tc06_WaitDone(uint32_t timeout)
{
    while ((g_spi_done == false) && (timeout > 0U)) {
        timeout--;
    }
    g_spi_done = false;
    return (g_spi_status == 0U);
}

