/**************************************









***************************************/
#include "ioCC2530.h"
#include <stdio.h>
#include <string.h>
#include "DHT11.h"
#include "BH1750.h"
#include "motor.h"
#include "remote.h"


#define MQ_PIN P0_6            //����P0.6��Ϊ�������������
#define Relay_PIN P0_5
#define LED P1_0

char is_serial_receive = 0;
void UartSenddata(unsigned char *Data, int len) 
{
    int j; 
    for(j=0;j<len;j++) 
    { 
        U0DBUF = *Data++; 
        while(UTX0IF == 0); 
        UTX0IF = 0; 
    } 
    
}

void main(void)
{
  unsigned int temp_now,hum_now,Guang;
  uchar MQ_data=1;
  uchar Window=0;
  uchar mode = 0;
  uchar j=0;
  CLKCONCMD &= ~0x40;      // ����ϵͳʱ��ԴΪ 32MHZ����
  while(CLKCONSTA & 0x40); // �ȴ������ȶ� 
  CLKCONCMD &= ~0x47;      // ����ϵͳ��ʱ��Ƶ��Ϊ 32MHZ
  
  P1DIR |= 0x01;                              // P1.0 ����LED
  LED = 1;
  
  P0DIR &= ~0x40;              //P0.6����Ϊ����� MQ-2
  P0DIR |= 0x20;          //P0.5����Ϊ�̵������
  //P0SEL &=0x7f;
  Relay_PIN = 1;          //�̵����͵�ƽ��������ʼ��Ϊ��
  Init_BH1750();          // ���նȴ�����
  InitMotor();            // ���������ʼ��
  
  EA = 0;
 
  InitKey();
  uart0_init();                               //115200
  timer1_init();
  rf_init();
 
  EA = 1;

  printf("Server start!\r\n");
  while(1)
  {
    DHT11();             //��ȡ��ʪ��
    light();
    
   //Guang = ((unsigned int)buf[0]<<8)+buf[1];
   /*temp_now = wendu_shi*10 +wendu_ge;
   hum_now = shidu_shi*10+shidu_ge;*/
  
   UartSenddata("LIGHT:",6);
   UartSenddata(lx,5);
   UartSenddata("lx",2);
   UartSenddata("   ", 3);

    uchar temp[3]; 
    uchar humidity[3];   
    uchar strTemp[13]="Temperature:";
    uchar strHumidity[10]="Humidity:";
        //����ʪ�ȵ�ת�����ַ���
        temp[0]=wendu_shi+0x30;
        temp[1]=wendu_ge+0x30;
        humidity[0]=shidu_shi+0x30;
        humidity[1]=shidu_ge+0x30;    
        //��õ���ʪ��ͨ�����������������ʾ
        UartSenddata(strTemp, 12);
        UartSenddata(temp, 2);
        UartSenddata("   ", 3);
        UartSenddata(strHumidity, 9);
       UartSenddata(humidity, 2);
        UartSenddata("\n", 1);
  //��ӵ�

   
   if(MQ_PIN == 0)         //��Ũ�ȸ����趨ֵʱ ��ִ����������        
        {
            Delay_ms1(10);          //��ʱ������
            if(MQ_PIN == 0)     //ȷ�� Ũ�ȸ����趨ֵʱ ��ִ����������
            {
              MQ_data = 0;
            }
        }
   else
   {
      MQ_data = 1;
   }
   if(mode == 0)
   {
     if((MQ_data == 0)||(hum_now>50)||(temp_now>30))
     {
       Relay_PIN = 0;
     }
     else
     {
        Relay_PIN = 1;
     }
     
     if(((Guang>1000)||(Guang<10))&&(Window == 0))
     {
       MotorCW(3);
       Window = 1;
     }
     else if(((Guang<1000)&&(Guang>10)&&(Window == 0))&&( Window == 1))
     {
       MotorCCW(3);
       Window = 0;
     }
     else
     {
       MotorStop();
     }
   }

    if( is_serial_receive )
    {
      is_serial_receive = 0;
      uart0_sendbuf(bufer,sizeof(bufer));
      
      switch(bufer[3])
      {
      case 0x16: //'0'
        {
          if(mode == 0)
            mode =1;
          else if(mode ==1)
            mode = 0;
        }
      break;
      case 0x0C: //��1��
        LED = 0;
        break;
      case 0x18://'2'
        LED = 1;
        break;
      case 0x5E://'3'
        Relay_PIN = 0;
        break;
       case 0x08://'4'
        Relay_PIN = 1;
        break;
       case 0x1C://'5'
        MotorCW(6);
        break;
       case 0x5A://'6'
        MotorCCW(6);
        break;
       
      default:
        break;
      }
      uart0_flush_rxbuf();
    }
  }
}

