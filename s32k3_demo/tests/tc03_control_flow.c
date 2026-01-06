/* tc03_control_flow.c */
#include <stdint.h>

typedef enum {
    ST_IDLE = 0,
    ST_BUSY = 1,
    ST_ERR  = 2
} State_t;

static State_t g_state;

void Tc03_Handle(State_t s)
{
    switch (s) {
    case ST_IDLE:
        g_state = ST_BUSY;
        /* missing break / fall-through */
    case ST_BUSY:
        g_state = ST_IDLE;
        break;
    case ST_ERR:
        g_state = ST_ERR;
        break;
    }
}

