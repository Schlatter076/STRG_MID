/******************************************/
/*           ����������������޹�˾     */
/*Project:     �ղ����������俪�ؼ��     */
/*Guest:                                  */
/*Name:             main.c                */
/*Mcu chip:         Atmega64              */
/*Main clock:       �ⲿ����11.0592MHz    */
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

//������ȡָ��
const unsigned char read_current[] = {0x88, 0xAE, 0x00, 0x11};

char *cur_str;

/***********USART0�����жϷ����� start**********************/
//USART���ջ�����
#define RX_BUFFER_SIZE 8                  //���ջ�������С���ɸ�����Ҫ�޸ġ�
unsigned char rx_buffer[RX_BUFFER_SIZE];   //������ջ�����
unsigned char rx_counter = 0;              //����rx_counterΪ����ڶ����е��ѽ��յ��ַ�������

//����һ����־λUsart0_RECVFlag1:=1��ʾ����0���յ���һ�����������ݰ�
//��port.h�ж���

#pragma interrupt_handler usart0_rxc_isr:19  //�����жϷ������
void usart0_rxc_isr(void)
{
    uchar status, data;
    status = UCSR0A;
    data = UDR0;
    if((flag1 & (1 << Usart0_RECVFlag1)) == 0) //�ж��Ƿ��������һ���µ����ݰ�
    {
        if ((status & (USART0_FRAMING_ERROR | USART0_PARITY_ERROR | USART0_DATA_OVERRUN)) == 0)
        {
            rx_buffer[rx_counter] = data;
            rx_counter++;
            switch (rx_counter)
            {
            case 1:       // ������ʼ�ַ�
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
                if (data != 0x00) rx_counter = 0; //�豸��ַ
            }
            break;
            case 8:      // ��������ַ�
            {
                rx_counter = 0;
                //check_byte = (rx_buffer[3] + rx_buffer[4] + rx_buffer[5] + rx_buffer[6]) & 0xff;
                //if (data == check_byte)
                set_bit(flag1, Usart0_RECVFlag1); // Usart0_RecvFlag=1����ʾ��ȷ���յ�һ�����ݰ�
            }
            break;
            default:
                break;
            }
        }
    }
}
/***************USART0�����жϷ����� end**********************/
/*============================================================*/
/*============================================================*/
/***************USART0�����жϷ����� start********************/
#define TX_BUFFER_SIZE 4
unsigned char tx_buffer[TX_BUFFER_SIZE];
unsigned char tx_wr_index = 0, tx_rd_index = 0, tx_counter = 0;

#pragma interrupt_handler usart0_txc_isr:21  //�����жϷ������
void usart0_txc_isr(void)
{
    if (tx_counter)//���в�Ϊ��
    {
        --tx_counter;//������
        UDR0 = tx_buffer[tx_rd_index];
        if (++tx_rd_index == TX_BUFFER_SIZE) tx_rd_index = 0;
    }
}
/***********USART0�����жϷ����� end**********************/

/*============================================================*/
/***********USART0����һ���ַ����� start**********************/
void USART0_putchar(unsigned char c)
{
    while (tx_counter == TX_BUFFER_SIZE);
    CLI();//#asm("cli")�ر�ȫ���ж�����
    if (tx_counter || ((UCSR0A & USART0_DATA_REGISTER_EMPTY) == 0)) //���ͻ�������Ϊ��
    {
        tx_buffer[tx_wr_index] = c; //���ݽ������
        if (++tx_wr_index == TX_BUFFER_SIZE) tx_wr_index = 0; //��������
        ++tx_counter;
    }
    else
        UDR0 = c;
    SEI(); //#asm("sei")��ȫ���ж�����
}
/***********USART0���ͷ����� end**********************/
//��װָ��ͺ���
void function(void (*fuc)())
{
    uchar func_cnt = 0;
    do
    {
        if(func_cnt++ >= 3)
        {
            func_cnt = 0;
            break; //������Դ�������RETYR_TIMES,����ѭ��
        }
        (*fuc)();    //���Ͳ���ָ��
        delay_nms(300); //�ȴ���������ݷ���
    }
    while((flag1 & (1 << Usart0_RECVFlag1)) == 0);
}
//���Ͷ�ȡ����ָ��
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

//�������ֽ�����ָ������
void fill_cur_vals(void)
{
    cur_U.buf[0] = rx_buffer[6];
    cur_U.buf[1] = rx_buffer[5];
    cur_U.buf[2] = rx_buffer[4];
    cur_U.buf[3] = rx_buffer[3];
}

//��ʱ��0��ʼ��
void init_TIMER0_OVF(void)
{
    TCCR0 = 0x06; //256��Ƶ
    TCNT0 = 256 - CRYSTAL / 256 / 2 * 0.5; //0.5s��ʱ
    TIMSK |= (1 << TOIE0); //��ʱ��0�ж�ʹ��
    SREG = 0x80;
}
#pragma interrupt_handler timer0_isr:17
void timer0_isr(void)
{
    TCNT0 = 256 - CRYSTAL / 256 / 2 * 0.5; //0.5s��ʱ
}
/***************USART01��ʼ������ start*************************/
void init_usart0(void)
{
    UCSR0B = 0x00;
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);  //�첽��8λ���ݣ�����żУ�飬һ��ֹͣλ���ޱ���
    UBRR0L = BAUD_L;                        //�趨������
    UBRR0H = BAUD_H;
    UCSR0A = 0x00;
    UCSR0B = (1 << RXCIE0) | (1 << TXCIE0) | (1 << RXEN0) | (1 << TXEN0); //0XD8  ���ա�����ʹ�ܣ� ���ж�ʹ��
}
/***************USART0��ʼ������ end***************************/
/***************ϵͳ��ʼ������ start ***********/
void init_cpu(void)
{
    EIMSK = 0x00; //����INT0~INT1�������ⲿ�ж�
    clr_bit(SFIOR, PUD); //������������������Ч
    DDRA = 0xff; //1�������0������
    PORTA = 0x00; //LCD�����ݿ�

    DDRB = 0xff; //1�������0������
    PORTB = 0x00;

    DDRC = 0xff;
    PORTC = 0x00;

    DDRD  = 0x4c; //PIND0/1/4/5/7Ϊ����
    PORTD = 0xb3;  //�������������������

    DDRE = 0xfe; //RXD0���룬��������Ч
    PORTE = 0x01; //TXD0���

    DDRF = 0x00; //PINFȫ������
    PORTF = 0xff;

    DDRG = 0xff;
    PORTG = 0x00; //PING��ȫ���

    init_usart0();
    init_TIMER0_OVF();
    SEI();

    flag1 = 0;
    flag2 = 0;
    flag3 = 0;
    flagerr = 0;
}
/***************ϵͳ��ʼ������ end ***********/
void strg_mid_auto(void)
{
    uchar line_cnt = 0;
    /*
    //���Ը�����ת�ַ�����ʾ�ô���
    uchar i;
    displayline(LINE1, "������MID ���  ");
    displayline(LINE2, "step7 ��������  ");
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
        displayline(LINE1, "������MID ���  ");
        displayline(LINE2, "step1 CN1-1-BR  ");
        displayline(LINE3, "                ");
        displayline(LINE4, "�������        ");
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
        //��������
        displayline(LINE1, "������MID ���  ");
        displayline(LINE2, "step7 ��������  ");
        displayline(LINE3, "                ");
        displayline(LINE4, "15mA-20mA       ");
        set_bit(PORTB, PB6);
        set_bit(PORTB, PB7);

        delay_nms(800);
        function(send_current);

        if((flag1 & (1 << Usart0_RECVFlag1)) != 0)
        {
            clr_bit(flag1, Usart0_RECVFlag1);

            fill_cur_vals(); //��ȡ�������ص���ֵ

            cur_str = ftoa(cur_U.v, 0); //��ʽ������Ϊ�ַ���
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
                displayline(LINE3, "��Ʒ            ");
                displayline(LINE4, "�밴�Ӽ���ʼ����");
            }
        }
        else
        {
            displayline(LINE3, "ͨѶ����, ������");
            set_bit(flag2, is_NGFlag2);
            goto error;
        }//*/

error:
        PORTB = 0x00;
        PORTC = 0x00;
        if((flag2 & (1 << is_NGFlag2)) != 0)
        {
            set_bit(PORTC, PC6); //��������
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
        displayline(LINE1, "������LOW ���  ");
        displayline(LINE2, "step1 CN1-1-V   ");
        displayline(LINE3, "                ");
        displayline(LINE4, "�������        ");
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
        
        //��������
        displayline(LINE1, "������LOW ���  ");
        displayline(LINE2, "step5 ��������  ");
        displayline(LINE3, "                ");
        displayline(LINE4, "1.75mA-2.20mA   ");
        set_bit(PORTB, PB6);
        set_bit(PORTB, PB7);

        delay_nms(800);
        function(send_current);

        if((flag1 & (1 << Usart0_RECVFlag1)) != 0)
        {
            clr_bit(flag1, Usart0_RECVFlag1);

            fill_cur_vals(); //��ȡ�������ص���ֵ

            cur_str = ftoa(cur_U.v, 0); //��ʽ������Ϊ�ַ���
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
                displayline(LINE3, "��Ʒ            ");
                displayline(LINE4, "�밴�Ӽ���ʼ����");
            }
        }
        else
        {
            displayline(LINE3, "ͨѶ����, ������");
            set_bit(flag2, is_NGFlag2);
            goto error;
        }//*/

error:
        PORTB = 0x00;
        PORTC = 0x00;
        if((flag2 & (1 << is_NGFlag2)) != 0)
        {
            set_bit(PORTC, PC6); //��������
        }
    }
}
//����������===============================================
void key_scan(void)
{
    if((flag1 && (1 << keyprq_flag1)) == 0)  //���û�а�������
    {
        if((PIND & (1 << key1)) == 0) //�������԰���
        {
            key_now = 1;
        }
        else if((PIND & (1 << key2)) == 0) //�ֶ�
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
//����������===============================================
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
            set_bit(flag1, keyeff_flag1);  //������Ч
        }
        if((flag1 & (1 << keyeff_flag1)) != 0)
        {
            clr_bit(flag1, keyeff_flag1);
            switch(key_old)
            {
            case 1:  //�������԰�������
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
                //men����
                num++;
                switch(num)
                {
                case 1:
                {
                    ver_select = 1;
                    displayline(LINE1, "������MID ���  ");
                    displayline(LINE2, "                ");
                }
                break;
                case 2:
                {
                    ver_select = 2;
                    displayline(LINE1, "������LOW ���  ");
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
/***************������ start *******************/
void main(void)
{
    init_cpu();    //��ʼ��CPU
    init_lcd();     //��ʼ��LCD
    delay_nms(20);
    DispWelcome();  //��ʾ��ӭ����
    delay_nms(200);
    Disp_Autokey();
    temp_jizhong = EEPROMread(0x20);
    switch(temp_jizhong)
    {
    case 1:
    {
        ver_select = 1;
        displayline(LINE1, "������MID ���  ");
        displayline(LINE2, "                ");
    }
    break;
    case 2:
    {
        ver_select = 2;
        displayline(LINE1, "������LOW ���  ");
        displayline(LINE2, "                ");
    }
    break;
    default:
    {
        ver_select = 1;
        displayline(LINE1, "������MID ���  ");
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