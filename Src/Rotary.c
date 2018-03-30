/*
 * Rotary.c
 *
 *  Created on: Feb 24, 2018
 *      Author: stefanth
 */
#include "main.h"
#include "stm32l4xx_hal.h"

__weak void RotaryAB_Callback(int dir)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(dir);
}

__weak void RotaryP_Callback(int but)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(but);
}

void Rotary_EXTI_Callback(uint16_t GPIO_Pin)
{
    if((GPIO_Pin == RotaryA_Pin) || (GPIO_Pin == RotaryB_Pin))
    {
        static int8_t encValAB = 0;
        static const int8_t encStateAB[] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};
        static uint8_t oldAB = 0x03;

        GPIO_PinState a = HAL_GPIO_ReadPin(RotaryA_GPIO_Port, RotaryA_Pin);
        GPIO_PinState b = HAL_GPIO_ReadPin(RotaryB_GPIO_Port, RotaryB_Pin);

        oldAB = (((oldAB << 1) | ((a == GPIO_PIN_SET) ? 1 : 0)) & 0x0f);
        oldAB = (((oldAB << 1) | ((b == GPIO_PIN_SET) ? 1 : 0)) & 0x0f);
        encValAB += encStateAB[oldAB & 0x0f];
        if(encValAB > 3)
        {
            // four steps forward
            RotaryAB_Callback(1);
            encValAB = 0;
        }
        else if(encValAB < -3)
        {
            // four steps back
            RotaryAB_Callback(-1);
            encValAB = 0;
        }
    }
    else if (GPIO_Pin == RotaryP_Pin)
    {
        static const int8_t encStateP[] = {0, -1, 1, 0};
        static uint8_t oldP = 0x01;

        GPIO_PinState p = HAL_GPIO_ReadPin(RotaryP_GPIO_Port, RotaryP_Pin);

        oldP =  (((oldP << 1) | ((p == GPIO_PIN_SET) ? 1 : 0)) & 0x03);
        RotaryP_Callback(encStateP[oldP & 0x03]);
    }
}

