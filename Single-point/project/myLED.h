#include <ioCC2530.h>

#define COLD_LED      P1_0    //冷光灯
#define HOT_LED       P1_1    //暖光灯
#define ALERT_LED     P1_4    //提示灯

#define DATA_PIN      P0_7    //温湿度

#define TRIG          P2_0    //测距
#define ECHO          P1_3    

#define LED_MODE_ON   0       //LED灯开
#define LED_MODE_OFF  1

#define LED_CYCLE_TIME  1000  //亮度周期

typedef unsigned char uchar;
typedef unsigned int  uint;

int receiveflag = 0;
void alert_led();
 /************************************************* 
* 
*函数名称：void  Sysclk_Init(void) 
*功能描述：晶振稳定
*参数说明：无 
* 
**************************************************/ 
 void Sysclk_Init(void)
{
   CLKCONCMD &= ~0x40;
   while( CLKCONSTA&0x40); //等待晶振稳定 
   CLKCONCMD &=~0x47;
}

 /************************************************* 
* 
*函数名称：void LED_Init(void) 
*功能描述：初始化LED灯
*参数说明：无 
* 
**************************************************/ 
 void Led_Init(void) 
 { 
   P1SEL &= ~0X13;        //定义P1_4,P1_1,P1_0为普通IO   //0001 0011 
   P1DIR |= 0X13;         //定义P1_4,P1_1,P1_0为输出 
   
   //初始化测距TRIG
   P2SEL &= ~0X01; //定义 P2_0 为普通 IO //0001 0000
   P2DIR |= 0X01;          
      
   //初始化全部LED灯
   COLD_LED = LED_MODE_OFF;   //LED1灯默认为关闭状态 
   HOT_LED  = LED_MODE_OFF ;  //LED3灯默认为关闭状态
   ALERT_LED = LED_MODE_OFF;   //LED3灯默认为关闭状态
} 
 /************************************************* 
* 
*函数名称：void Uart0_Init(void）
*功能描述：初始化串口
*参数说明：无 
* 
**************************************************/ 
void Uart0_Init(void)
{  

  PERCFG = 0x00;    //位置1 P0口 
  P0SEL = 0x0c;    //P0用作串口 
  P2DIR &= ~0XC0;                             //P0优先作为UART0     
 
  U0CSR |= 0x80;    //串口设置为UART方式 
  U0GCR |= 8;   
 
  U0BAUD |= 59;    //波特率设为9600 
  UTX0IF = 0;  
  U0CSR |= 0X40;    //允许接收 
  IEN0 |= 0x84;
  
/*
    PERCFG = 0x00;   // ????,UART0??????1 
    P0SEL = 0x0c;    // ??0????,P0_2?P0_3????
    P2DIR &= ~0xC0;  // ??0???????,?2??0,USART0??

    U0CSR |= 0x80;   // UART??
    U0GCR |= 11;
    U0UCR |=0x80; //流控制禁止;
    U0BAUD |= 59;   // ??32MHz?????,??????115200
    UTX0IF = 0; 
     //使能中断
    EA=1; //中断总开关// UART0 TX????????0
  */
  EA=1;
}

unsigned char rxTemp = 0; //传感器接收临时数据
unsigned int rb_count = 0; //receiece_buffer count
unsigned char recieve_buf[6];//接收数据
 /************************************************* 
* 
*函数名称：Uart0_Send_String(unsigned char *Data,int len)
*功能描述：传感器数据发送到串口
*参数说明：无 
* 
**************************************************/ 
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
/************************************************* 
* 
*函数名称：Uart0_Send_String(unsigned char *Data,int len)
*功能描述：传感器数据发送到串口
*参数说明：无 
* 
**************************************************/ 
void Usart0_Receive_String()
{
  if(rxTemp != 0)       //接收到数据
    {
       recieve_buf[rb_count] = rxTemp; 
       rb_count++;
       if(rb_count >= 6)
       {
         rb_count = 0;
         receiveflag = 1;
       }
       else
       {
         receiveflag = 0;
       }
    }
}
/************************************************** 69. 
函 数 名 : Uart0_ISR 
70. 功能描述 : 中断服务函数 
71. 输入参数 : NONE 
72. 输出参数 : NONE 
73. 返 回 值 : NONE 
74. ***************************************************/ 

#pragma vector = URX0_VECTOR  
__interrupt void Uart0_ISR(void) 
{ 
 
  URX0IF = 0; // 清中断标志 
  while(!U0DBUF);
  rxTemp = U0DBUF;//读取接收到的数据 
  Usart0_Receive_String();

}

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

void Delay( int n) 
{  
  for( int j = 0;j <5;++j)
    for( int i = 0;i<n;++i);
} 

//温湿度延时函数
void wDelay_us() //1 us延时
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

void wDelay_10us() //10 us延时
{
uchar i=18;
  for(;i>0;i--);  
}

void wDelay_ms(uint Time)//n ms延时
{
    unsigned char i;
    while(Time--)
    {
        for(i=0;i<100;i++)
            wDelay_10us();
    }
}
 /************************************************* 
* 
*函数名称：void PMW(int lightlevel,int colorlevel)
*功能描述：LED调光
*参数说明：lightlevel 亮度,colorlevel 色温
* 
**************************************************/ 
void PWM(int lightlevel,int colorlevel)
{
    if(lightlevel == 0)
    {
      COLD_LED = LED_MODE_OFF;
      HOT_LED = LED_MODE_OFF;
    }
    else{
      int cold_led_time = lightlevel * (LED_CYCLE_TIME/100) * (100-colorlevel) /100;
      int hot_led_time = lightlevel * (LED_CYCLE_TIME/100) * colorlevel /100;
      int i = 1000;

      while(i--)
      {
        if(colorlevel >= 50)  //冷光亮的时间少
        {      
          COLD_LED = LED_MODE_ON;
          HOT_LED = LED_MODE_ON;
          Delay(cold_led_time);
          COLD_LED = LED_MODE_OFF;
          Delay(hot_led_time - cold_led_time);
          HOT_LED = LED_MODE_OFF;
          Delay(LED_CYCLE_TIME - hot_led_time);
      }
        else          //暖光亮的时间少
        {
          COLD_LED = LED_MODE_ON;
          HOT_LED = LED_MODE_ON;
          Delay(hot_led_time);
          HOT_LED = LED_MODE_OFF;
          Delay(cold_led_time - hot_led_time);
          COLD_LED = LED_MODE_OFF;
          Delay(LED_CYCLE_TIME - cold_led_time);      
        }
      }
    }
}
 /************************************************* 
* 
*函数名称：void alert_led()
*功能描述：提示灯闪烁五秒
*参数说明：无
* 
**************************************************/ 
void alert_led()
{
    for(uint i = 0;i < 50;++i)
    {
      ALERT_LED = LED_MODE_ON;
      Delay_ms(5);
      ALERT_LED = LED_MODE_OFF;
      Delay_ms(5);
    }
}

//温湿度定义
uchar ucharFLAG,uchartemp;
uchar shidu_shi,shidu_ge,wendu_shi,wendu_ge=4;
uchar ucharT_data_H,ucharT_data_L,ucharRH_data_H,ucharRH_data_L,ucharcheckdata;
uchar ucharT_data_H_temp,ucharT_data_L_temp,ucharRH_data_H_temp,ucharRH_data_L_temp,ucharcheckdata_temp;
uchar ucharcomdata;

//温湿度传感
 /************************************************* 
* 
*函数名称：void COM(void)
*功能描述：温湿写入
*参数说明：无
* 
**************************************************/ 
void COM(void)    // 温湿写入
{     
    uchar i;         
    for(i=0;i<8;i++)    
    {
        ucharFLAG=2; 
        while((!DATA_PIN)&&ucharFLAG++);
        wDelay_10us();
        wDelay_10us();
        wDelay_10us();
        uchartemp=0;
        if(DATA_PIN)uchartemp=1;
        ucharFLAG=2;
        while((DATA_PIN)&&ucharFLAG++);   
        if(ucharFLAG==1)break;    
        ucharcomdata<<=1;
        ucharcomdata|=uchartemp; 
    }    
}
 /************************************************* 
* 
*函数名称：void DHT11(void)
*功能描述：温湿传感启动
*参数说明：无
* 
**************************************************/ 
void DHT11(void)   //温湿传感启动
{
    DATA_PIN=0;
    wDelay_ms(19);  //>18MS
    DATA_PIN=1; 
    P0DIR &= ~0x80; //重新配置IO口方向
    wDelay_10us();
    wDelay_10us();                        
    wDelay_10us();
    wDelay_10us(); 
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

//测距


uchar count_start; 
uchar H1;
uchar H2;
uchar L2;
uchar L1;
uchar cycle;

 /************************************************* 
* 
*函数名称：void UltrasoundRanging1()
*功能描述：激活测距传感器
*参数说明：无
* 
**************************************************/ 
void UltrasoundRanging1() 
{   
  EA = 0; 
  TRIG =1;
  
  Delay_us(15);       
  TRIG =0;
  
  T1CNTL=0; 
  T1CNTH=0;  
  
  while(!P1_3);   
  T1CTL = 0x09;        
  L1=T1CNTL;  
  H1=T1CNTH;
  EA = 1;  
  Delay_ms(200);     
} 
 /************************************************* 
* 
*函数名称：Init_UltrasoundRanging()
*功能描述：初始化端口
*参数说明：无
* 
**************************************************/ 
void Init_UltrasoundRanging() 
{  
  P1DIR |= 0x0d;    
  TRIG=0; 
  
  
  P1INP &= ~0x08;   
  P1IEN |= 0x08;       //P1_3 
  PICTL |= 0x02;       //P0_6 
  IEN2 |= 0x10;        // P0IE = 1; 
  P1IFG = 0;  
  
} 

 /************************************************* 
* 
*函数名称：void P1_ISR(void)
*功能描述：中断服务函数
*参数说明：无
* 
**************************************************/ 

#pragma vector = P1INT_VECTOR 
__interrupt void P1_ISR(void) 
{ 
  EA=0;  
  L2=T1CNTL;  
  H2=T1CNTH;  
  
  if(P1IFG&0x08)     
  {    
    P1IFG = 0;        
  } 
  else if(P1IFG&0x08)
  {
    P1IFG = 0;  
  }
  P1IF = 0;            
} 
 /************************************************* 
* 
*函数名称：float caldistance()
*功能描述：测距函数
*参数说明：返回距离(cm_)
* 
**************************************************/ 
float caldistance(){
  uint y;
  float distance;
  y=H2*256+L2-L1-256*H1;
  distance=(float)y*340/10000;
  return distance;
}
 /************************************************* 
* 
*函数名称：inputdistance(unsigned char *send_buf,unsigned int distance)
*功能描述：将距离化为规格的字符串
*参数说明：返回距离(cm_)
* 
**************************************************/ 
void inputdistance(unsigned char *send_buf,unsigned int distance)
{
    if(distance >= 200)
    {
         send_buf[5] = '2';
         send_buf[6] = '0';
         send_buf[7] = '0';
    }
    else if(distance >= 100)
    {
         send_buf[5] = distance / 100 + '0';
         send_buf[6] = distance % 100 / 10 + '0' ;
         send_buf[7] = distance % 10 + '0';
    }
    else if(distance >=10)
    {
         send_buf[5] = '0';
         send_buf[6] = distance / 10 + '0';
         send_buf[7] = distance % 10 + '0';
    }
    else if(distance >=0)
    {
         send_buf[5] = '0';
         send_buf[6] = '0';
         send_buf[7] = distance + '0';
    }
    else
    {
         send_buf[5] = '0';
         send_buf[6] = '0';
         send_buf[7] = '0';    
    }

}