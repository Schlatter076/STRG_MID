
/******************************************/
/*           广州旭东阪田电子有限公司     */
/*Project:      FCT测试主板               */
/*Guest:                                  */
/*Name:             12864_LCD.h           */
/*Mcu chip:         Atmega64              */
/*Main clock:       内部晶体8MHz          */
/*Rtc clock:                              */
/*Author:           Jack.Fu               */
/*Create date:      2017.09.18            */
/*Design date:                            */
/*Complete date:                          */
/******************************************/

#ifndef _12864_LCD_H
#define _12864_LCD_H

#define LINE1 0x80
#define LINE2 0x90
#define LINE3 0x88
#define LINE4 0x98

void write_command(unsigned char x);
void write_data(unsigned char x);
void init_lcd(void);
void displayline(unsigned char line,unsigned char *DBpointer);
void DispWelcome(void);
void Disp_Autokey(void);
void Disp_dachong(void);
void Disp_xiaochong(void);
void Disp_longtou(void);

#endif