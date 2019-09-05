
/******************************************/
/*           广州旭东阪田电子有限公司     */
/*Project:      FCT测试主板               */
/*Guest:                                  */
/*Name:             12864_lcd.c           */
/*Mcu chip:         Atmega64              */
/*Main clock:       外部晶体11.0592MHz    */
/*Rtc clock:                              */
/*Author:           Jack.Fu               */
/*Create date:      2008.11.20            */
/*Design date:                            */
/*Complete date:                          */
/******************************************/

/********************************************************************/
/********************************************************************/
#include <iom64v.h>
#include <macros.h>
#include <delay.h>
#include <port.h>
#include <default.h>
#include <12864_LCD.h>

/*---------------------------------------------*/
/***************LCD初始化函数 start ************/
void init_lcd(void)
{
    clr_bit(PORTE,LCD_EN);
    set_bit(PORTE,LCD_RS);
    delay_nms(2);
    write_command(0x32); //选择基本指令集
    delay_nms(2);
    write_command(0x32);
    delay_nms(2);
    write_command(0x08); //关显示，不显示光标
    delay_nms(2);
    write_command(0x01); //清除显示
    delay_nms(2);
    write_command(0x0c); //开显示
    delay_nms(2);
    write_command(0x02); //AC=0
    delay_nms(200);
    write_command(0x06); //AC自动加1，禁止滚动
}
/***************LCD初始化函数 end **************/
/*---------------------------------------------*/

/*---------------------------------------------*/
/***************写控制字函数 start**************/
void write_command(unsigned char x)
{
    clr_bit(PORTE,LCD_RS);
    LCD_data=x;
    clr_bit(PORTE,LCD_EN);
    delay_nms(2);
    set_bit(PORTE,LCD_EN);
    delay_nms(2);
}
/***************写控制字函数 end ***************/
/*---------------------------------------------*/

/*---------------------------------------------*/
/***************写数据函数 start****************/
void write_data(unsigned char x)
{
    set_bit(PORTE,LCD_RS);
    LCD_data=x;
    clr_bit(PORTE,LCD_EN);
    delay_nms(2);
    set_bit(PORTE,LCD_EN);
    delay_nms(2);
}
/***************写数据函数 end *****************/
/*---------------------------------------------*/

/*---------------------------------------------*/
/********显示一行字符串数据函数 start***********/
//函数名：DisplayLine
//输入：line显示指定的行，DBpointer显示的数据指针
//输出：无
//功能：显示一行字符串数据
void displayline(uchar line,uchar *DBpointer)
{
    uchar i,Len;
    write_command(line);
	Len = 16;
    //Len =  strlen(DBpointer);
    for (i=0;i<Len;i++)
    {
        write_data(*(DBpointer+i));
    }
}
/********显示一行字符串数据函数 end*************/
/*---------------------------------------------*/

/*---------------------------------------------*/
/*************显示欢迎屏函数 start**************/
//函数名：DispWelcome
//输入：无
//输出：无
//功能：显示欢迎屏
void DispWelcome(void)
{
    displayline(LINE1,"    欢迎使用    ");
    displayline(LINE2,"方向盘MID 检测  ");
    displayline(LINE3,"    Ver:1.0     ");
    displayline(LINE4,"  PKS-旭东阪田  ");
}
/*************显示欢迎屏函数 end****************/
/*---------------------------------------------*/

/*---------------------------------------------*/
/*************显示请按键函数 start**************/
//函数名：Disp_Autokey
//输入：无
//输出：无
//功能：显示请按键
void Disp_Autokey(void)
{
    displayline(LINE1,"方向盘MID 检测  ");
    displayline(LINE2,"                ");
    displayline(LINE3,"                ");
    displayline(LINE4,"请按加键开始测试");
}
/*************显示请按键函数 end****************/
/*---------------------------------------------*/