#ifndef __GPIO_H__
#define __GPIO_H__

#include <em_device.h>
#include "cmu.h"
#include "systick.h"
#include "utils.h"
#include "nvic.h"

// LED MACROS
#define LED_HIGH()          PERI_REG_BIT_SET(&(GPIO->P[0].DOUT)) = BIT(15)
#define LED_LOW()           PERI_REG_BIT_CLEAR(&(GPIO->P[0].DOUT)) = BIT(15)
#define LED_TOGGLE()        GPIO->P[0].DOUTTGL = BIT(15);

// TXPLL MACROS
#define TXPLL_ENABLE()      PERI_REG_BIT_SET(&(GPIO->P[5].DOUT)) = BIT(7)
#define TXPLL_DISABLE()     PERI_REG_BIT_CLEAR(&(GPIO->P[5].DOUT)) = BIT(7)
#define TXPLL_LATCH()       PERI_REG_BIT_SET(&(GPIO->P[3].DOUT)) = BIT(12)
#define TXPLL_UNLATCH()     PERI_REG_BIT_CLEAR(&(GPIO->P[3].DOUT)) = BIT(12)
#define TXPLL_UNMUTE()      PERI_REG_BIT_SET(&(GPIO->P[5].DOUT)) = BIT(8)
#define TXPLL_MUTE()        PERI_REG_BIT_CLEAR(&(GPIO->P[5].DOUT)) = BIT(8)
#define TXPLL_LOCKED()      PERI_REG_BIT(&(GPIO->P[5].DIN), 9)

void gpio_init();

#endif  // __GPIO_H__
