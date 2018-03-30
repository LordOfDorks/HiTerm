/*
 * Hitachi44780U.h
 *
 *  Created on: Feb 23, 2018
 *      Author: stefanth
 */

#ifndef HITACHI44780U_H_
#define HITACHI44780U_H_

#define HD44780U_CLEAR_DISPLAY (0x01)
#define HD44780U_RETURN_HOME (0x02)
#define HD44780U_INITIALIZATION (0x03)
#define HD44780U_ENTRYMODESET(__Inc, __Shift) (0x04 + (__Inc ? 0x02 : 0) + (__Shift ? 0x01 : 0))
#define HD44780U_ENTRYMODESET_INC (1)
#define HD44780U_ENTRYMODESET_DEC (0)
#define HD44780U_ENTRYMODESET_SHIFT (1)
#define HD44780U_ENTRYMODESET_NOSHIFT (0)
#define HD44780U_DISPLAYCONTROL(__On, __Cursor, __Blink) (0x08 + (__On ? 0x04 : 0) + (__Cursor ? 0x02 : 0) + (__Blink ? 0x01 : 0))
#define HD44780U_DISPLAYCONTROL_ON (1)
#define HD44780U_DISPLAYCONTROL_OFF (0)
#define HD44780U_DISPLAYCONTROL_CURSOR (1)
#define HD44780U_DISPLAYCONTROL_NOCURSOR (0)
#define HD44780U_DISPLAYCONTROL_BLINK (1)
#define HD44780U_DISPLAYCONTROL_NOBLINK (0)
#define HD44780U_CURSORORDISPLAYSHIFT(__Shift, __Dir) (0x10 + (__Shift ? 0x08 : 0) + (__Dir ? 0x04 : 0))
#define HD44780U_CURSORORDISPLAYSHIFT_DISPLAYSHIFT (1)
#define HD44780U_CURSORORDISPLAYSHIFT_CURSORMOVE (0)
#define HD44780U_CURSORORDISPLAYSHIFT_RIGHT (1)
#define HD44780U_CURSORORDISPLAYSHIFT_LEFT (0)
#define HD44780U_FUNCTIONSET(__Datalen, __Lines, __Font) (0x20 + (__Datalen ? 0x10 : 0) + (__Lines ? 0x08 : 0) + (__Font ? 0x04 : 0))
#define HD44780U_FUNCTIONSET_8BIT (1)
#define HD44780U_FUNCTIONSET_4BIT (0)
#define HD44780U_FUNCTIONSET_2LINES (1)
#define HD44780U_FUNCTIONSET_1LINE (0)
#define HD44780U_FUNCTIONSET_5X10DOT (1)
#define HD44780U_FUNCTIONSET_5X8DOT (0)
#define HD44780U_SETCGRAMADDR(__Addr) (0x40 + (__Addr & 0x3f))
#define HD44780U_SETDRAMADDR(__Addr) (0x80 + (__Addr & 0x7f))
#define HD44780U_READBUSYFLAG(__Data) (__Data >> 7)
#define HD44780U_READADDRESS(__Data) (__Data & 0x7f)

typedef enum
{
    HD44780U_RS,
    HD44780U_RW,
    HD44780U_E,
    HD44780U_D4,
    HD44780U_D5,
    HD44780U_D6,
    HD44780U_D7,
    HD44780U_MAX
} HD44780U_signal_t;

typedef struct
{
    GPIO_TypeDef* port;
    uint16_t pin;
} HD44780U_SignalMap_t;

void HD44780U_BackLight(bool on);
void HD44780U_Init(bool lines, bool font);
void HD44780U_ClearScreen(void);
void HD44780U_Home(void);
void HD44780U_DisplayCtrl(bool on);
void HD44780U_CursorCtrl(bool on, bool blink);
void HD44780U_ShiftCtrl(bool display, bool right);
bool HD44780U_SetCursorPos(uint8_t y, uint8_t x);
void HD44780U_GetCursorPos(uint8_t* y, uint8_t* x);
uint8_t HD44780U_WriteChar(unsigned char ch);
uint8_t HD44780U_WriteString(char* str);

#endif /* HITACHI44780U_H_ */
