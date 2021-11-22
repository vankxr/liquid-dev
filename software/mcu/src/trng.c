#include "trng.h"

void trng_init()
{
    cmu_hfper0_clock_gate(CMU_HFPERCLKEN0_TRNG0, 1);

    TRNG0->CONTROL = TRNG_CONTROL_FORCERUN_RUN | TRNG_CONTROL_SOFTRESET_RESET | TRNG_CONTROL_CONDBYPASS_BYPASS | TRNG_CONTROL_TESTEN_NOISE;
    TRNG0->CONTROL &= ~TRNG_CONTROL_SOFTRESET_RESET;
    TRNG0->CONTROL |= TRNG_CONTROL_ENABLE_ENABLED;

    while(TRNG0->FIFOLEVEL < 4);

    TRNG0->KEY0 = TRNG0->FIFO;
    TRNG0->KEY1 = TRNG0->FIFO;
    TRNG0->KEY2 = TRNG0->FIFO;
    TRNG0->KEY3 = TRNG0->FIFO;
}
uint32_t trng_pop_random()
{
    while(!TRNG0->FIFOLEVEL);

    return TRNG0->FIFO;
}