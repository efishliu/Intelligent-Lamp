#include "myLED.h"
#include "ioCC2530.h"
#include "string.h"


unsigned char send_buf[8];
unsigned char temp_buf[30];
float distance = 0.0;
int lightlevel = 0;
int colorlevel = 0;

void main()
{
  Sysclk_Init();
  Led_Init();
  Uart0_Init();
  Init_UltrasoundRanging();
  memset(send_buf,'0',8*sizeof(char));  //初始化
  memset(recieve_buf,0,6*sizeof(char));
  send_buf[0] = 'C';   //帧头
  while(1)
  {
    //获取温湿度
    DHT11();
    send_buf[1] = wendu_shi + '0';
    send_buf[2] = wendu_ge + '0';
    send_buf[3] = shidu_shi + '0';
    send_buf[4] = shidu_ge + '0';

    //获取距离

    UltrasoundRanging1(); 
    distance = caldistance();
    Delay_ms(2); 
    //Uart0_Send_String(temp_buf,sprintf(temp_buf,"d:%.2f \n",distance));
    inputdistance(send_buf,distance); 
    Uart0_Send_String(send_buf,8);
    memset(send_buf+1,'0',7*sizeof(char));
    if(receiveflag  == 1)
    {
      lightlevel = (recieve_buf[1]-'0')*10 + (recieve_buf[2]-'0');
      colorlevel = (recieve_buf[3]-'0')*10 + (recieve_buf[4]-'0');
      receiveflag = 0;
      if(recieve_buf[5] == '1')
        alert_led();
      
      //Uart0_Send_String(recieve_buf,6);
      //Uart0_Send_String(temp_buf,sprintf(temp_buf,"l:%d,c:%d\n",lightlevel,colorlevel));
      memset(recieve_buf,0,6*sizeof(char));
    }
    PWM(lightlevel,colorlevel);
    //Delay_ms(100);
    
  }
  
}