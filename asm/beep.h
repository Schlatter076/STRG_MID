
/******************************************/
/*           广州旭东阪田电子有限公司     */
/*Project:      FCT测试主板               */
/*Guest:                                  */
/*Name:             beep.h                */
/*Mcu chip:         Atmega64              */
/*Main clock:       内部晶体8MHz          */
/*Rtc clock:                              */
/*Author:           Jack.Fu               */
/*Create date:      2008.11.20            */
/*Design date:                            */
/*Complete date:                          */
/******************************************/

#ifndef _BEEP_H
#define _BEEP_H

void beep1s(void);
void beep3s(void);
void beep500ms(void);
void beeperr3(void);
void rstBEEP(void);
void setBEEP(void);
#endif