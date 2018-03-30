/*
 * Hitachi44780U.c
 *
 *  Created on: Feb 23, 2018
 *      Author: stefanth
 */


#include "main.h"
#include "stm32l4xx_hal.h"
#include <stdbool.h>
#include <string.h>
#include "Hitachi44780U.h"

struct
{
    uint32_t x;
    uint32_t y;
    uint32_t g;
} cursorPos = {0, 0, 0};

#define maxY (100)
uint32_t windowStart = 0;
uint8_t screenBuf[maxY * 20] = {0};

HD44780U_SignalMap_t signalMap[HD44780U_MAX] = {
        {Disp_RS_GPIO_Port, Disp_RS_Pin},
        {Disp_RW_GPIO_Port, Disp_RW_Pin},
        {Disp_E_GPIO_Port, Disp_E_Pin},
        {Disp_D4_GPIO_Port, Disp_D4_Pin},
        {Disp_D5_GPIO_Port, Disp_D5_Pin},
        {Disp_D6_GPIO_Port, Disp_D6_Pin},
        {Disp_D7_GPIO_Port, Disp_D7_Pin}
};

static void SlideWindow(int dir);
static void RefreshWindow(uint32_t pos);

void RotaryAB_Callback(int dir)
{
    static volatile uint32_t lastTick = 0;
    if((lastTick < HAL_GetTick()))
    {
        lastTick = (uint32_t)0x8ffffff; // Stop from re-entering
        SlideWindow(dir);
        lastTick = HAL_GetTick() + 10; // Set a 5ms cooldown
    }
}

static void WriteBit(HD44780U_signal_t sig, bool set)
{
    if((sig == HD44780U_D4) || (sig == HD44780U_D5) || (sig == HD44780U_D6) || (sig == HD44780U_D7))
    {
        GPIO_InitTypeDef GPIO_InitStruct;
        GPIO_InitStruct.Pin = signalMap[sig].pin;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
        HAL_GPIO_Init(signalMap[sig].port, &GPIO_InitStruct);
    }
    HAL_GPIO_WritePin(signalMap[sig].port, signalMap[sig].pin, (set ? GPIO_PIN_SET : GPIO_PIN_RESET));
}

static void ResetBit(HD44780U_signal_t sig)
{
    if((sig == HD44780U_D4) || (sig == HD44780U_D5) || (sig == HD44780U_D6) || (sig == HD44780U_D7))
    {
        GPIO_InitTypeDef GPIO_InitStruct;
        GPIO_InitStruct.Pin = signalMap[sig].pin;
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(signalMap[sig].port, &GPIO_InitStruct);
    }
}

static bool ReadBit(HD44780U_signal_t sig)
{
    if((sig == HD44780U_D4) || (sig == HD44780U_D5) || (sig == HD44780U_D6) || (sig == HD44780U_D7))
    {
        return (HAL_GPIO_ReadPin(signalMap[sig].port, signalMap[sig].pin) == GPIO_PIN_SET);
    }
    return false;
}

static void Write4Bit(bool rs, uint8_t dataIn)
{
    WriteBit(HD44780U_RS, rs);
    WriteBit(HD44780U_RW, false);
    WriteBit(HD44780U_E, true);
    WriteBit(HD44780U_D7, (dataIn & 0x08));
    WriteBit(HD44780U_D6, (dataIn & 0x04));
    WriteBit(HD44780U_D5, (dataIn & 0x02));
    WriteBit(HD44780U_D4, (dataIn & 0x01));
    WriteBit(HD44780U_E, false);
}

static uint8_t HD44780U_Read(bool rs)
{
    uint8_t data = 0;
    WriteBit(HD44780U_RS, rs);
    WriteBit(HD44780U_RW, true);
    WriteBit(HD44780U_E, true);
    data |= ReadBit(HD44780U_D7) ? 0x80 : 0;
    data |= ReadBit(HD44780U_D6) ? 0x40 : 0;
    data |= ReadBit(HD44780U_D5) ? 0x20 : 0;
    data |= ReadBit(HD44780U_D4) ? 0x10 : 0;
    WriteBit(HD44780U_E, false);
    WriteBit(HD44780U_E, false); // Make sure the signal actually drops
    WriteBit(HD44780U_E, true);
    data |= ReadBit(HD44780U_D7) ? 0x08 : 0;
    data |= ReadBit(HD44780U_D6) ? 0x04 : 0;
    data |= ReadBit(HD44780U_D5) ? 0x02 : 0;
    data |= ReadBit(HD44780U_D4) ? 0x01 : 0;
    WriteBit(HD44780U_E, false);
    return data;
}

static uint8_t HD44780U_Write(bool rs, uint8_t dataIn)
{
    uint8_t pos = 0;
    Write4Bit(rs, dataIn >> 4);
    Write4Bit(rs, dataIn & 0x0f);
    ResetBit(HD44780U_D7);
    ResetBit(HD44780U_D6);
    ResetBit(HD44780U_D5);
    ResetBit(HD44780U_D4);
    do
    {
        pos = HD44780U_Read(false);
    }
    while(HD44780U_READBUSYFLAG(pos));
    WriteBit(HD44780U_RW, false);
    return HD44780U_READADDRESS(pos);
}

static uint8_t HD44780U_FixupCursorPos(uint8_t pos)
{
    // Fixup the cusrsor position
    if(pos == 20)
    {
        HD44780U_Write(false, HD44780U_SETDRAMADDR(64));
        pos = 64;
    }
    else if(pos == 84)
    {
        HD44780U_Write(false, HD44780U_SETDRAMADDR(20));
        pos = 20;
    }
    else if(pos == 64)
    {
        HD44780U_Write(false, HD44780U_SETDRAMADDR(84));
        pos = 84;
    }

//    // Transpose the position
//    if((pos >= 0) && (pos < 20))
//    {
//        cursorPos.y = 0;
//        cursorPos.x = pos;
//    }
//    if((pos >= 20) && (pos < 40))
//    {
//        cursorPos.y = 2;
//        cursorPos.x = pos - 20;
//    }
//    if((pos >= 64) && (pos < 84))
//    {
//        cursorPos.y = 1;
//        cursorPos.x = pos - 64;
//    }
//    if((pos >= 84) && (pos < 104))
//    {
//        cursorPos.y = 3;
//        cursorPos.x = pos - 84;
//    }
//    cursorPos.g = cursorPos.y * 20 + cursorPos.x;

    return pos;
}

static void HD44780U_ScrollUp(void)
{
    memcpy(&screenBuf[0], &screenBuf[20], sizeof(screenBuf) - 20);
    memset(&screenBuf[sizeof(screenBuf) - 20], ' ', 20);
//    HD44780U_Home();
//    for(uint32_t n = 0; n < sizeof(screenBuf); n++)
//    {
//        HD44780U_TransposeCursorPos(HD44780U_READADDRESS(HD44780U_Write(true, screenBuf[n])));
//    }
}

bool HD44780U_SetCursorPos(uint8_t y, uint8_t x)
{
//    uint8_t pos = 0;
    if((y > (maxY + 1)) || (x > 20)) return false;

//    switch(y)
//    {
//        case 0:
//            pos = x;
//            break;
//        case 1:
//            pos = 64 + x;
//            break;
//        case 2:
//            pos = 20 + x;
//            break;
//        case 4:
//            HD44780U_ScrollUp();
//            y = 3;
//        case 3:
//            pos = 84 + x;
//            break;
//    }
    if(y == (maxY + 1))
    {
        HD44780U_ScrollUp();
        y--;
    }
    cursorPos.x = x;
    cursorPos.y = y;
    cursorPos.g = cursorPos.y * 20 + cursorPos.x;
//    HD44780U_Write(false, HD44780U_SETDRAMADDR(pos));
    return true;
}

void HD44780U_Init(bool lines, bool font)
{
    HAL_Delay(15);
    Write4Bit(false, HD44780U_INITIALIZATION);
    HAL_Delay(5);
    Write4Bit(false, HD44780U_INITIALIZATION);
    HAL_Delay(1);
    Write4Bit(false, HD44780U_INITIALIZATION);
    Write4Bit(false, HD44780U_RETURN_HOME);
    HD44780U_Write(false, HD44780U_FUNCTIONSET(HD44780U_FUNCTIONSET_4BIT, lines, font));
    HD44780U_Write(false, HD44780U_DISPLAYCONTROL(HD44780U_DISPLAYCONTROL_ON, HD44780U_DISPLAYCONTROL_NOCURSOR, HD44780U_DISPLAYCONTROL_NOBLINK));
    HD44780U_Write(false, HD44780U_CLEAR_DISPLAY);
    memset(screenBuf, ' ', sizeof(screenBuf));
}

void HD44780U_ClearScreen(void)
{
    HD44780U_Write(false, HD44780U_CLEAR_DISPLAY);
}

void HD44780U_Home(void)
{
    HD44780U_Write(false, HD44780U_RETURN_HOME);
}

void HD44780U_DisplayCtrl(bool on)
{
    HD44780U_Write(false, HD44780U_DISPLAYCONTROL(on, HD44780U_DISPLAYCONTROL_NOCURSOR, HD44780U_DISPLAYCONTROL_NOBLINK));
}

void HD44780U_CursorCtrl(bool on, bool blink)
{
    HD44780U_Write(false, HD44780U_DISPLAYCONTROL(HD44780U_DISPLAYCONTROL_ON, on, blink));
}

void HD44780U_ShiftCtrl(bool display, bool right)
{
    HD44780U_Write(false, HD44780U_CURSORORDISPLAYSHIFT(display, right));
}

uint8_t HD44780U_WriteChar(unsigned char ch)
{
    if(ch == '\r')
    {
        HD44780U_SetCursorPos(cursorPos.y, 0);
    }
    else if (ch == '\n')
    {
        HD44780U_SetCursorPos(cursorPos.y + 1, cursorPos.x);
    }
    else
    {
        screenBuf[cursorPos.g] = ch;
        RefreshWindow(cursorPos.g);
        if(++cursorPos.g > sizeof(screenBuf))
        {
            HD44780U_ScrollUp();
            cursorPos.g -= 20;
        }
        cursorPos.y = cursorPos.g / 20;
        cursorPos.x = cursorPos.g % 20;
//        HD44780U_TransposeCursorPos(HD44780U_READADDRESS(HD44780U_Write(true, ch)));
//        if(cursorPos.g == 0)
//        {
//            HD44780U_SetCursorPos(4, 0);
//        }
    }
    return 1;
}

uint8_t HD44780U_WriteString(char* str)
{
    uint32_t m = 0;
    for(uint32_t n = 0; n < strlen(str); n++)
    {
        m += HD44780U_WriteChar(str[n]);
    }
    return m;
}

void HD44780U_BackLight(bool on)
{
    HAL_GPIO_WritePin(Disp_BL_GPIO_Port, Disp_BL_Pin, (on ? GPIO_PIN_SET : GPIO_PIN_RESET));
}

static void SlideWindow(int dir)
{
    if(((dir < 0) && (windowStart == 0)) ||
       ((dir > 0) && ((windowStart / 20) >= cursorPos.y - 3)))
    {
        return;
    }
    windowStart += (dir * 20);
    HD44780U_Home();
    for(uint32_t n = 0; n < 4; n++)
    {
        for(uint32_t m = 0; m < 20; m++)
        {
            HD44780U_FixupCursorPos(HD44780U_READADDRESS(HD44780U_Write(true, screenBuf[windowStart + n * 20 + m])));
        }
    }
    if((windowStart / 20) > 0)
    {
        HD44780U_Write(false, HD44780U_SETDRAMADDR(19));
        HD44780U_Write(true, 0x7f); // Arrow left
    }
    if((windowStart / 20) + 3 < cursorPos.y)
    {
        HD44780U_Write(false, HD44780U_SETDRAMADDR(103));
        HD44780U_Write(true, 0x7e); // Arrow right
    }
}

static void RefreshWindow(uint32_t pos)
{
    if((pos < windowStart) || (pos > (windowStart + 4 * 20)))
    {
        // Jump the window to the change
        windowStart = (pos / 20) * 20;
        if(windowStart <= (3 * 20))
        {
            windowStart = 0;
        }
        else
        {
            windowStart -= (3 * 20);
        }
        SlideWindow(0);
    }

    // Calculate and set display position
    uint8_t wPosX = (pos - windowStart) % 20;
    uint8_t wPosY = (pos - windowStart) / 20;
    uint8_t dPos = 0;
    switch(wPosY)
    {
        case 0:
            dPos = wPosX;
            break;
        case 1:
            dPos = 64 + wPosX;
            break;
        case 2:
            dPos = 20 + wPosX;
            break;
        case 3:
            dPos = 84 + wPosX;
            break;
    }
    HD44780U_Write(false, HD44780U_SETDRAMADDR(dPos));

    // Write the change
    HD44780U_Write(true, screenBuf[pos]);
}
