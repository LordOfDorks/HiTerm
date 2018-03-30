/*
 * Rotary.h
 *
 *  Created on: Feb 24, 2018
 *      Author: stefanth
 */

#ifndef ROTARY_H_
#define ROTARY_H_

void RotaryAB_Callback(int dir); // Will be called dir = 1/-1 for left/right
void RotaryP_Callback(int but); // Will be called but = 1/-1 for pressed/released
void Rotary_EXTI_Callback(uint16_t GPIO_Pin);

#endif /* ROTARY_H_ */
