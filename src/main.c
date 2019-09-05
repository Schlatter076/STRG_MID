/******************************************/
/*           广州旭东阪田电子有限公司     */
/*Project:     日产方向盘中配开关检测     */
/*Guest:                                  */
/*Name:             main.c                */
/*Mcu chip:         Atmega64              */
/*Main clock:       外部晶体11.0592MHz    */
/*Rtc clock:                              */
/*Author:                                 */
/*Create date:      2019.05.18            */
/*Design date:                            */
/*Complete date:                          */
/******************************************/
#include <iom64v.h>
#include <stdio.h>
#include <macros.h>
#include <port.h>
#include <default.h>
#include <delay.h>
#include <12864_LCD.h>
#include <beep.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <eeprom.h>


#define KEY_COUNTER 5
//Key
uchar key_temp = 0;
uchar key_now = 0;
uchar key_code = 0;
uchar key_old = 0;
uchar key_cnt = 0;

uchar temp_jizhong = 0;
uchar num = 0;
uchar ver_select = 0;
/*============================================================*/

//电流读取指令
const unsigned char read_current[] = {0x88, 0xAE, 0x00, 0x11};

char *cur_str;

/***********USART0接收中断服务函数 start**********************/
//USART接收缓冲区
#define RX_BUFFER_SIZE 8                  //接收缓冲区大小，可根据需要修改。
unsigned char rx_buffer[RX_BUFFER_SIZE];   //定义接收缓冲区
unsigned char rx_counter = 0;              //定义rx_counter为存放在队列中的已接收到字符个数。

//定义一个标志位Usart0_RECVFlag1:=1表示串口0接收到了一个完整的数据包
//在port.h中定义

#pragma interrupt_handler usart0_rxc_isr:19  //接收中断服务程序
void usart0_rxc_isr(void)
{
    uchar status, data;
    status = UCSR0A;
    data = UDR0;
    if((flag1 & (1 << Usart0_RECVFlag1)) == 0) //判断是否允许接收一个新的数据包
    {
        if ((status & (USART0_FRAMING_ERROR | USART0_PARITY_ERROR | USART0_DATA_OVERRUN)) == 0)
        {
            rx_buffer[rx_counter] = data;
            rx_counter++;
            switch (rx_counter)
            {
            case 1:       // 检验起始字符
            {
                if (data != 0xFA) rx_counter = 0;
            }
            break;
            case 2:
            {
                if (data != 0xFB) rx_counter = 0;
            }
            break;
            case 3:
            {
                if (data != 0x00) rx_counter = 0; //设备地址
            }
            break;
            case 8:      // 检验结束字符
            {
                rx_counter = 0;
                //check_byte = (rx_buffer[3] + rx_buffer[4] + rx_buffer[5] + rx_buffer[6]) & 0xff;
                //if (data == check_byte)
                set_bit(flag1, Usart0_RECVFlag1); // Usart0_RecvFlag=1，表示正确接收到一个数据包
            }
            break;
            default:
                break;
            }
        }
    }
}
/***************USART0接收中断服务函数 end**********************/
/*============================================================*/
/*============================================================*/
/***************USART0发送中断服务函数 start********************/
#define TX_BUFFER_SIZE 4
unsigned char tx_buffer[TX_BUFFER_SIZE];
unsigned char tx_wr_index = 0, tx_rd_index = 0, tx_counter = 0;

#pragma interrupt_handler usart0_txc_isr:21  //发送中断服务程序
void usart0_txc_isr(void)
{
    if (tx_counter)//队列不为空
    {
        --tx_counter;//出队列
        UDR0 = tx_buffer[tx_rd_index];
        if (++tx_rd_index == TX_BUFFER_SIZE) tx_rd_index = 0;
    }
}
/***********USART0发送中断服务函数 end**********************/

/*============================================================*/
/***********USART0发送一个字符函数 start**********************/
void USART0_putchar(unsigned char c)
{
    while (tx_counter == TX_BUFFER_SIZE);
    CLI();//#asm("cli")关闭全局中断允许
    if (tx_counter || ((UCSR0A & USART0_DATA_REGISTER_EMPTY) == 0)) //发送缓冲器不为空
    {
        tx_buffer[tx_wr_index] = c; //数据进入队列
        if (++tx_wr_index == TX_BUFFER_SIZE) tx_wr_index = 0; //队列已满
        ++tx_counter;
    }
    else
        UDR0 = c;
    SEI(); //#asm("sei")打开全局中断允许
}
/***********USART0发送服务函数 end**********************/
//封装指令发送函数
void function(void (*fuc)())
{
    uchar func_cnt = 0;
    do
    {
        if(func_cnt++ >= 3)
        {
            func_cnt = 0;
            break; //如果重试次数大于RETYR_TIMES,结束循环
        }
        (*fuc)();    //发送测试指令
        delay_nms(300); //等待被测板数据返回
    }
    while((flag1 & (1 << Usart0_RECVFlag1)) == 0);
}
//发送读取电流指令
void send_current(void)
{
    uchar i;
    for(i = 0; i < TX_BUFFER_SIZE; i++)
    {
        USART0_putchar(read_current[i]);
    }
}

union U
{
    float v;
    unsigned char buf[4];
} cur_U;

//填充电流字节数到指定数组
void fill_cur_vals(void)
{
    cur_U.buf[0] = rx_buffer[6];
    cur_U.buf[1] = rx_buffer[5];
    cur_U.buf[2] = rx_buffer[4];
    cur_U.buf[3] = rx_buffer[3];
}

//定时器0初始化
void init_TIMER0_OVF(void)
{
    TCCR0 = 0x06; //256分频
    TCNT0 = 256 - CRYSTAL / 256 / 2 * 0.5; //0.5s定时
    TIMSK |= (1 << TOIE0); //定时器0中断使能
    SREG = 0x80;
}
#pragma interrupt_handler timer0_isr:17
void timer0_isr(void)
{
    TCNT0 = 256 - CRYSTAL / 256 / 2 * 0.5; //0.5s定时
}
/***************USART01初始化函数 start*************************/
void init_usart0(void)
{
    UCSR0B = 0x00;
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);  //异步，8位数据，无奇偶校验，一个停止位，无倍速
    UBRR0L = BAUD_L;                        //设定波特率
    UBRR0H = BAUD_H;
    UCSR0A = 0x00;
    UCSR0B = (1 << RXCIE0) | (1 << TXCIE0) | (1 << RXEN0) | (1 << TXEN0); //0XD8  接收、发送使能， 开中断使能
}
/***************USART0初始化函数 end***************************/
/***************系统初始化函数 start ***********/
void init_cpu(void)
{
    EIMSK = 0x00; //屏蔽INT0~INT1的所有外部中断
    clr_bit(SFIOR, PUD); //设置输入上拉电阻有效
    DDRA = 0xff; //1是输出，0是输入
    PORTA = 0x00; //LCD的数据口

    DDRB = 0xff; //1是输出，0是输入
    PORTB = 0x00;

    DDRC = 0xff;
    PORTC = 0x00;

    DDRD  = 0x4c; //PIND0/1/4/5/7为输入
    PORTD = 0xb3;  //外接上拉，按键带上拉

    DDRE = 0xfe; //RXD0输入，且上拉有效
    PORTE = 0x01; //TXD0输出

    DDRF = 0x00; //PINF全是输入
    PORTF = 0xff;

    DDRG = 0xff;
    PORTG = 0x00; //PING口全输出

    init_usart0();
    init_TIMER0_OVF();
    SEI();

    flag1 = 0;
    flag2 = 0;
    flag3 = 0;
    flagerr = 0;
}
/***************系统初始化函数 end ***********/
void strg_mid_auto(void)
{
    uchar line_cnt = 0;
    /*
    //测试浮点数转字符串显示用代码
    uchar i;
    displayline(LINE1, "方向盘MID 检测  ");
    displayline(LINE2, "step7 电流测试  ");
    displayline(LINE3, "                ");
    displayline(LINE4, "1.96-2.20mA     ");
    for(i = 0; i < 10; i++)
    {
        cur_data = 3.5462808f + i;
    	cur_str = ftoa(cur_data, 0);
    		write_command(LINE3);
    		for(line_cnt = 0; line_cnt < 6; line_cnt++)
    		{
    		    write_data(cur_str[line_cnt]);
    		}
    		for(line_cnt = 0; line_cnt < 10; line_cnt++)
    		{
    		    write_data(' ');
    		}
    		delay_nms(2000);
    }
    while(1);
    //*/
    if((flag2 & (1 << is_NGFlag2)) == 0)
    {
        //step1
        displayline(LINE1, "方向盘MID 检测  ");
        displayline(LINE2, "step1 CN1-1-BR  ");
        displayline(LINE3, "                ");
        displayline(LINE4, "线束检查        ");
        set_bit(PORTB, PB0);
        set_bit(PORTC, PC0);
        delay_nms(200);
        if((PINF & (1 << PF2)) == 0)
        {
            displayline(LINE3, "OK              ");
        }
        else
        {
            displayline(LINE3, "NG              ");
            set_bit(flag2, is_NGFlag2);
            goto error;
        }
        clr_bit(PORTB, PB0);
        clr_bit(PORTC, PC0);
        //step2
        displayline(LINE2, "step2 CN1-2-V   ");
        displayline(LINE3, "                ");
        set_bit(PORTB, PB1);
        set_bit(PORTC, PC1);
        delay_nms(200);
        if((PINF & (1 << PF3)) == 0)
        {
            displayline(LINE3, "OK              ");
        }
        else
        {
            displayline(LINE3, "NG              ");
            set_bit(flag2, is_NGFlag2);
            goto error;
        }
        clr_bit(PORTB, PB1);
        clr_bit(PORTC, PC1);
        displayline(LINE2, "step3 CN1-3-P   ");
        displayline(LINE3, "                ");
        set_bit(PORTB, PB2);
        set_bit(PORTC, PC2);
        delay_nms(200);
        if((PINF & (1 << PF4)) == 0)
        {
            displayline(LINE3, "OK              ");
        }
        else
        {
            displayline(LINE3, "NG              ");
            set_bit(flag2, is_NGFlag2);
            goto error;
        }
        clr_bit(PORTB, PB2);
        clr_bit(PORTC, PC2);
        displayline(LINE2, "step4 CN1-4-W   ");
        displayline(LINE3, "                ");
        set_bit(PORTB, PB3);
        set_bit(PORTC, PC3);
        delay_nms(200);
        if((PINF & (1 << PF5)) == 0)
        {
            displayline(LINE3, "OK              ");
        }
        else
        {
            displayline(LINE3, "NG              ");
            set_bit(flag2, is_NGFlag2);
            goto error;
        }
        clr_bit(PORTB, PB3);
        clr_bit(PORTC, PC3);
        displayline(LINE2, "step5 CN1-5-Y   ");
        displayline(LINE3, "                ");
        set_bit(PORTB, PB4);
        set_bit(PORTC, PC4);
        delay_nms(200);
        if((PINF & (1 << PF6)) == 0)
        {
            displayline(LINE3, "OK              ");
        }
        else
        {
            displayline(LINE3, "NG              ");
            set_bit(flag2, is_NGFlag2);
            goto error;
        }
        clr_bit(PORTB, PB4);
        clr_bit(PORTC, PC4);
        displayline(LINE2, "step6 CN1-6-L   ");
        displayline(LINE3, "                ");
        set_bit(PORTB, PB5);
        set_bit(PORTC, PC5);
        delay_nms(200);
        if((PINF & (1 << PF7)) == 0)
        {
            displayline(LINE3, "OK              ");
        }
        else
        {
            displayline(LINE3, "NG              ");
            set_bit(flag2, is_NGFlag2);
            goto error;
        }
        clr_bit(PORTB, PB5);
        clr_bit(PORTC, PC5);
        //电流测试
        displayline(LINE1, "方向盘MID 检测  ");
        displayline(LINE2, "step7 电流测试  ");
        displayline(LINE3, "                ");
        displayline(LINE4, "15mA-20mA       ");
        set_bit(PORTB, PB6);
        set_bit(PORTB, PB7);

        delay_nms(800);
        function(send_current);

        if((flag1 & (1 << Usart0_RECVFlag1)) != 0)
        {
            clr_bit(flag1, Usart0_RECVFlag1);

            fill_cur_vals(); //获取电流表返回的数值

            cur_str = ftoa(cur_U.v, 0); //格式化浮点为字符串
            write_command(LINE3);
            for(line_cnt = 0; line_cnt < 6; line_cnt++)
            {
                write_data(cur_str[line_cnt]);
            }
            for(line_cnt = 0; line_cnt < 10; line_cnt++)
            {
                write_data(' ');
            }
            delay_nms(1000);
            if(cur_U.v < 15 || cur_U.v > 20)
            {
                set_bit(flag2, is_NGFlag2);
                goto error;
            }
            else
            {
                displayline(LINE2, "                ");
                displayline(LINE3, "良品            ");
                displayline(LINE4, "请按加键开始测试");
            }
        }
        else
        {
            displayline(LINE3, "通讯错误, 请重试");
            set_bit(flag2, is_NGFlag2);
            goto error;
        }//*/

error:
        PORTB = 0x00;
        PORTC = 0x00;
        if((flag2 & (1 << is_NGFlag2)) != 0)
        {
            set_bit(PORTC, PC6); //不良报警
        }
    }
}
void strg_low_auto(void)
{
    uchar line_cnt = 0;
    if((flag2 & (1 << is_NGFlag2)) == 0)
    {
        //step1
        displayline(LINE2, "step2 CN1-2-BR  ");
        displayline(LINE3, "                ");
        set_bit(PORTB, PB1);
        set_bit(PORTC, PC0);
        delay_nms(200);
        if((PINF & (1 << PF3)) == 0)
        {
            displayline(LINE3, "OK              ");
        }
        else
        {
            displayline(LINE3, "NG              ");
            set_bit(flag2, is_NGFlag2);
            goto error;
        }
        clr_bit(PORTB, PB1);
        clr_bit(PORTC, PC0);
		//step2
        displayline(LINE1, "方向盘LOW 检测  ");
        displayline(LINE2, "step1 CN1-1-V   ");
        displayline(LINE3, "                ");
        displayline(LINE4, "线束检查        ");
        set_bit(PORTB, PB0);
        set_bit(PORTC, PC1);
        delay_nms(200);
        if((PINF & (1 << PF2)) == 0)
        {
            displayline(LINE3, "OK              ");
        }
        else
        {
            displayline(LINE3, "NG              ");
            set_bit(flag2, is_NGFlag2);
            goto error;
        }
        clr_bit(PORTB, PB0);
        clr_bit(PORTC, PC1);
        
        displayline(LINE2, "step3 CN1-3-P   ");
        displayline(LINE3, "                ");
        set_bit(PORTB, PB2);
        set_bit(PORTC, PC2);
        delay_nms(200);
        if((PINF & (1 << PF4)) == 0)
        {
            displayline(LINE3, "OK              ");
        }
        else
        {
            displayline(LINE3, "NG              ");
            set_bit(flag2, is_NGFlag2);
            goto error;
        }
        clr_bit(PORTB, PB2);
        clr_bit(PORTC, PC2);
        
        displayline(LINE2, "step4 CN1-5-Y   ");
        displayline(LINE3, "                ");
        set_bit(PORTB, PB4);
        set_bit(PORTC, PC4);
        delay_nms(200);
        if((PINF & (1 << PF6)) == 0)
        {
            displayline(LINE3, "OK              ");
        }
        else
        {
            displayline(LINE3, "NG              ");
            set_bit(flag2, is_NGFlag2);
            goto error;
        }
        clr_bit(PORTB, PB4);
        clr_bit(PORTC, PC4);
        
        //电流测试
        displayline(LINE1, "方向盘LOW 检测  ");
        displayline(LINE2, "step5 电流测试  ");
        displayline(LINE3, "                ");
        displayline(LINE4, "1.75mA-2.20mA   ");
        set_bit(PORTB, PB6);
        set_bit(PORTB, PB7);

        delay_nms(800);
        function(send_current);

        if((flag1 & (1 << Usart0_RECVFlag1)) != 0)
        {
            clr_bit(flag1, Usart0_RECVFlag1);

            fill_cur_vals(); //获取电流表返回的数值

            cur_str = ftoa(cur_U.v, 0); //格式化浮点为字符串
            write_command(LINE3);
            for(line_cnt = 0; line_cnt < 6; line_cnt++)
            {
                write_data(cur_str[line_cnt]);
            }
            for(line_cnt = 0; line_cnt < 10; line_cnt++)
            {
                write_data(' ');
            }
            delay_nms(1000);
            if(cur_U.v < 1.75 || cur_U.v > 2.20)
            {
                set_bit(flag2, is_NGFlag2);
                goto error;
            }
            else
            {
                displayline(LINE2, "                ");
                displayline(LINE3, "良品            ");
                displayline(LINE4, "请按加键开始测试");
            }
        }
        else
        {
            displayline(LINE3, "通讯错误, 请重试");
            set_bit(flag2, is_NGFlag2);
            goto error;
        }//*/

error:
        PORTB = 0x00;
        PORTC = 0x00;
        if((flag2 & (1 << is_NGFlag2)) != 0)
        {
            set_bit(PORTC, PC6); //不良报警
        }
    }
}
//按键处理函数===============================================
void key_scan(void)
{
    if((flag1 && (1 << keyprq_flag1)) == 0)  //如果没有按键按下
    {
        if((PIND & (1 << key1)) == 0) //启动测试按键
        {
            key_now = 1;
        }
        else if((PIND & (1 << key2)) == 0) //手动
        {
            key_now = 2;
        }//*/
        else
        {
            key_now = 0;
            key_old = 0;
            key_code = 0;
        }
        if(key_now != 0)
        {
            if(key_now != key_code)
            {
                key_code = key_now;
                key_cnt = 0;
            }
            else
            {
                key_cnt++;
                if(key_cnt >= KEY_COUNTER)
                {
                    set_bit(flag1, keyprq_flag1);
                }
            }
        }
    }
}
//按键处理函数===============================================
void key_process(void)
{
    if((flag1 & (1 << keyprq_flag1)) != 0)
    {
        clr_bit(flag1, keyprq_flag1);
        if(key_code == key_old)
        {
            ; //do nothing~
        }
        else
        {
            key_old = key_code;
            set_bit(flag1, keyeff_flag1);  //按键有效
        }
        if((flag1 & (1 << keyeff_flag1)) != 0)
        {
            clr_bit(flag1, keyeff_flag1);
            switch(key_old)
            {
            case 1:  //启动测试按键按下
            {
                switch(ver_select)
                {
                case 1:
                    strg_mid_auto();
                    break;
                case 2:
                    strg_low_auto();
                    break;
                default:
                    break;
                }
            }
            break;
            case 2:
            {
                //men按键
                num++;
                switch(num)
                {
                case 1:
                {
                    ver_select = 1;
                    displayline(LINE1, "方向盘MID 检测  ");
                    displayline(LINE2, "                ");
                }
                break;
                case 2:
                {
                    ver_select = 2;
                    displayline(LINE1, "方向盘LOW 检测  ");
                    displayline(LINE2, "                ");
                }
                break;
                default:
                {
                    num = 0;
                }
                break;
                }
                EEPROMwrite(0x20, ver_select);
            }
            break;//*/
            default:
                break;
            }
        }
    }
}
/***************主函数 start *******************/
void main(void)
{
    init_cpu();    //初始化CPU
    init_lcd();     //初始化LCD
    delay_nms(20);
    DispWelcome();  //显示欢迎界面
    delay_nms(200);
    Disp_Autokey();
    temp_jizhong = EEPROMread(0x20);
    switch(temp_jizhong)
    {
    case 1:
    {
        ver_select = 1;
        displayline(LINE1, "方向盘MID 检测  ");
        displayline(LINE2, "                ");
    }
    break;
    case 2:
    {
        ver_select = 2;
        displayline(LINE1, "方向盘LOW 检测  ");
        displayline(LINE2, "                ");
    }
    break;
    default:
    {
        ver_select = 1;
        displayline(LINE1, "方向盘MID 检测  ");
        displayline(LINE2, "                ");
    }
    break;
    }
    while(1)
    {
        key_scan();
        key_process();
        delay_nms(10);
    }
}