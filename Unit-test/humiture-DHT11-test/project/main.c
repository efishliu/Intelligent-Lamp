#include <ioCC2530.h>

typedef unsigned char uchar;
typedef unsigned int  uint;

#define DATA_PIN P0_7

/*
vcc 液晶屏 6
GND 液晶屏 8
DATA P0_7

*/

//温湿度定义
uchar ucharFLAG,uchartemp;
uchar shidu_shi,shidu_ge,wendu_shi,wendu_ge=4;
uchar ucharT_data_H,ucharT_data_L,ucharRH_data_H,ucharRH_data_L,ucharcheckdata;
uchar ucharT_data_H_temp,ucharT_data_L_temp,ucharRH_data_H_temp,ucharRH_data_L_temp,ucharcheckdata_temp;
uchar ucharcomdata;
/************************************************* 
* 
*函数名称：void Delay()
*功能描述：各延时函数
*参数说明：无 
* 
**************************************************/ 
void Delay_us(uint n)
{
  while(n--)
  {
    asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");
    asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");
    asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");
    asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");
    asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");
    asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");
    asm("nop");
  }
}
void Delay_ms(uint n)//n ms延时
{
    uint i;
    while(n--)
    {
      for(i = 0;i < 1000;++i)
        Delay_us(1);
    }
}
void Delay_s(uint n)  //n s延时
{
  uint i = 0;
  while(n--)
  {
    for(i = 0;i < 1000;++i)
      Delay_ms(1);
  }
}

/*
//温湿度延时函数
void Delay_us() //1 us延时
{
    asm("nop");
    asm("nop");
    asm("nop");
   asm("nop");
    asm("nop");    
    asm("nop");
    asm("nop");
    asm("nop");    
    asm("nop"); 
}

void Delay_10us() //10 us延时
{
  /*Delay_us();
  Delay_us();
  Delay_us();
  Delay_us();
  Delay_us();
  Delay_us();
  Delay_us();
  Delay_us();
  Delay_us();
  Delay_us(); 
uchar i=18;
  for(;i>0;i--);  
}

void Delay_ms(uint Time)//n ms延时
{
    unsigned char i;
    while(Time--)
    {
        for(i=0;i<100;i++)
            Delay_10us();
    }
}
*/
//温湿度传感
void COM(void)    // 温湿写入
{     
    uchar i;         
    for(i=0;i<8;i++)    
    {
        ucharFLAG=2; 
        while((!DATA_PIN)&&ucharFLAG++);
        Delay_us(10);
        Delay_us(10);
        Delay_us(10);
        uchartemp=0;
        if(DATA_PIN)uchartemp=1;
        ucharFLAG=2;
        while((DATA_PIN)&&ucharFLAG++);   
        if(ucharFLAG==1)break;    
        ucharcomdata<<=1;
        ucharcomdata|=uchartemp; 
    }    
}

void DHT11(void)   //温湿传感启动
{
    DATA_PIN=0;
    Delay_ms(19);  //>18MS
    DATA_PIN=1; 
    P0DIR &= ~0x80; //重新配置IO口方向
    Delay_us(10);
    Delay_us(10);                     
    Delay_us(10);
    Delay_us(10);  
    if(!DATA_PIN) 
    {
        ucharFLAG=2; 
        while((!DATA_PIN)&&ucharFLAG++);
        ucharFLAG=2;
        while((DATA_PIN)&&ucharFLAG++); 
        COM();
        ucharRH_data_H_temp=ucharcomdata;
        COM();
        ucharRH_data_L_temp=ucharcomdata;
        COM();
        ucharT_data_H_temp=ucharcomdata;
        COM();
        ucharT_data_L_temp=ucharcomdata;
        COM();
        ucharcheckdata_temp=ucharcomdata;
        DATA_PIN=1; 
        uchartemp=(ucharT_data_H_temp+ucharT_data_L_temp+ucharRH_data_H_temp+ucharRH_data_L_temp);
        if(uchartemp==ucharcheckdata_temp)
        {
            ucharRH_data_H=ucharRH_data_H_temp;
            ucharRH_data_L=ucharRH_data_L_temp;
            ucharT_data_H=ucharT_data_H_temp;
            ucharT_data_L=ucharT_data_L_temp;
            ucharcheckdata=ucharcheckdata_temp;
        }
        wendu_shi=ucharT_data_H/10; 
        wendu_ge=ucharT_data_H%10;
        
        shidu_shi=ucharRH_data_H/10; 
        shidu_ge=ucharRH_data_H%10;        
    } 
    else //没用成功读取，返回0
    {
        wendu_shi=0; 
        wendu_ge=0;
        
        shidu_shi=0; 
        shidu_ge=0;  
    } 
    
    P0DIR |= 0x80; //IO口需要重新配置 
}

void CLK_INIT(void)                                         
{ 
  
  //unsigned int i;                                                                                                                     
  //SLEEPCMD &= ~0x04;               /* power on 16MHz RC and 32MHz XOSC */                
  //while (!(SLEEPSTA &0x40));          /* wait for 32MHz XOSC stable */
  //asm("NOP");                     
  //for (i=0; i<504; i++) asm("NOP");          /* Require 63us delay for all revs */                
  CLKCONCMD &= ~0x40;     //设置系统时钟源为32MHZ晶振
  while(CLKCONSTA & 0x40);   //等待晶振稳定
  CLKCONCMD &= ~0x47;    //设置系统主时钟频率为32MHZ
               //此时的CLKCONSTA为0x88。即普通时钟和定时器时钟都是32M。                        /* turn off 16MHz RC */                                                                
}

void Uart0_Send_String(unsigned char *Data,int len) 
{ 
  int i; 
  for(i=0;i<len;i++) 
  { 
    U0DBUF = *Data++; 
    while(UTX0IF == 0); 
    UTX0IF = 0; 
  } 
} 
void Uart0_Init(void)
{  

  PERCFG = 0x00;    //位置1 P0口 
  P0SEL = 0x0c;    //P0用作串口 
  P2DIR &= ~0XC0;                             //P0优先作为UART0     
 
  U0CSR |= 0x80;    //串口设置为UART方式 
  U0GCR |= 8;     
  U0BAUD |= 59;    //波特率设为9600 
  UTX0IF = 1;  
  U0CSR |= 0X40;    //允许接收 
  IEN0 |= 0x84;
}




void main()
{
CLK_INIT();
Uart0_Init();
while(1)
{
  DHT11();
  uchar buffer[4];
  buffer[0] = wendu_shi + '0';
  buffer[1] = wendu_ge + '0';
  buffer[2] = shidu_shi + '0';
  buffer[3] = shidu_ge + '0';

  char *output;
  sprintf(output,"temp: %c%c,hum: %c%c\n",buffer[0],buffer[1],buffer[2],buffer[3]);
  //if(buffer[0] != '0')
    Uart0_Send_String(output,strlen(output));
  Delay_ms(200);
}
}