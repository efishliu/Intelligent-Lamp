#include <ioCC2530.h>

#define S5 P0_4            //宏定义P0_4为S5的控制端口 
#define S6 P0_5            //宏定义P0_5为S6的控制端口 
#define LED1   P1_4        //定义P1_4为LED1的控制端口 
#define LED2   P1_1        //定义P1_1为LED2的控制端口 
#define LED3   P1_0        //定义P1_0为LED3的控制端口 


#define KEY_UP   1         //宏定义按键松开为1 
#define KEY_DOWN 0         //宏定义按键按下为0 

#define LED_MODE_ON    0   //定义0为LED灯打开 
#define LED_MODE_OFF   1   //定义0为LED灯关闭 

#define LED_MAX_LEVEL   5  //LED灯的最大亮度等级
#define LED_MAX_NUMBER  3  //LED灯的数量

#define CRITICAL_TIME  150 //长按的临界时间

int circle_time = 1000;    //亮度周期

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

/**************************** 
* 延时 
*****************************/ 
void Delay( int n) 
{  
  for( int j = 0;j <5;++j)
    for( int i = 0;i<n;++i);
} 

 /************************************************* 
* 
*函数名称：void  LED_Init(void) 
*功能描述：LED灯相应IO口的配置 
*参数说明：无 
* 
**************************************************/ 
 void  Led_Init(void) 
 { 
   P1SEL &= ~0X13;        //定义P1_4,P1_1,P1_0为普通IO   //0001 0011 
   P1DIR |= 0X13;         //定义P1_4,P1_1,P1_0为输出 
      
   //初始化全部LED灯
   LED1 = LED_MODE_OFF;   //LED1灯默认为关闭状态 
   LED2 = LED_MODE_OFF ;  //LED3灯默认为关闭状态
   LED3 = LED_MODE_OFF;   //LED3灯默认为关闭状态
} 

 /************************************************* 
* 
*函数名称：void  KEY_Init(void) 
*功能描述：按键相应IO口的配置 
*参数说明：无 
* 
**************************************************/ 
 void Key_Init(void) 
 { 
   //S5--->KEY1 = P04,S5--->KEY2 = P05   
    P0SEL &= ~0X30;   //定义P0_4 ,P0_5为普通IO 
    P0DIR &= ~0X30;   //定义P0_4 ,P0_5为输入 
    S5 = KEY_UP;
    S6 = KEY_UP;
 } 


 /************************************************* 
* 
*函数名称：void  PWM(int level,int led_number)
*功能描述：LED调光 
*参数说明：level为光亮程度，led_number为LED编号
* 
**************************************************/ 
void PWM(int level,int led_number)
{
  if(led_number == 1)       //LED1
  {
    LED3 = LED_MODE_OFF;
    if(level == 0) 
      LED1 = LED_MODE_OFF;
    else{
      int time = level * (circle_time/LED_MAX_LEVEL);   
      LED1 = LED_MODE_ON;
      Delay(time);
      LED1 = LED_MODE_OFF;
      Delay(circle_time - time);
  }
  }
  
  else if(led_number == 2)
  {
    LED1 = LED_MODE_OFF;
    if(level == 0) 
     LED2 = LED_MODE_OFF;
    else{
      int time = level * (circle_time/LED_MAX_LEVEL);
      LED2 = LED_MODE_ON;
      Delay(time);
      LED2 = LED_MODE_OFF;
      Delay(circle_time - time);
    }
  }
  
  else if(led_number == 3)
  {
    LED2 = LED_MODE_OFF;
    if(level == 0) 
      LED3 = LED_MODE_OFF;
    else{
      int time = level * (circle_time/LED_MAX_LEVEL);
      LED3 = LED_MODE_ON;
      Delay(time);
      LED3 = LED_MODE_OFF;
      Delay(circle_time - time);
    }
  }
}

void main()
{
   int level = 0;               //LED亮度等级
   int led_number = 1;          //LED编号
   int close_last_time = 0;     //长按关闭按钮持续时间
   int twinkle_last_time = 0;   //长按闪烁变换按钮持续时间
   Led_Init();
   Key_Init();
  
  while(1)
  {    
    if(S5 == KEY_DOWN && S6 == KEY_UP )  //s5按下，s6未按下
    {
      
      --level;                  //减少亮度等级
      if(level < 0) level = 0; 
      twinkle_last_time = 0;
      if(level != 0)
      {
        while(S5 == KEY_DOWN && S6 == KEY_UP) 
        {
          PWM(level,led_number);
          ++twinkle_last_time;
          if(circle_time == 1000)
          {
            if(twinkle_last_time > CRITICAL_TIME)   //达到长按临界时间
            {
              circle_time = 10000;         //变换亮度周期
              break;
            }
          }
          else
          {
            if(twinkle_last_time > CRITICAL_TIME/10 )
            {
              circle_time = 1000;
              break;
            }
          }
        }
      }
      while(S5 == KEY_DOWN && S6 == KEY_UP) PWM(level,led_number);
    }

    if(S6 == KEY_DOWN && S5 == KEY_UP)  //s6按下，s5未按下
    {
      ++level;
      if(level > LED_MAX_LEVEL ) level = LED_MAX_LEVEL;
      close_last_time = 0;
      while(S6 == KEY_DOWN && S5 == KEY_UP) 
      {
        PWM(level,led_number);
        ++close_last_time;
        if(circle_time == 1000)
        {
          if(close_last_time > CRITICAL_TIME)
          {
            level = 0;
            break;
          }
        }
        else
        {
          if(close_last_time > CRITICAL_TIME/10 )
          {
            level = 0;
            break;
          }
        }
      }
      while(S6 == KEY_DOWN && S5 == KEY_UP) PWM(level,led_number);
    }
    
    if(S5 == KEY_DOWN && S6 == KEY_DOWN) //s5,s6同时按下,切换LED灯
    {
      ++led_number;
      if(led_number > LED_MAX_NUMBER ) led_number = 1;
      level = 1;
      while(S6 == KEY_DOWN || S5 == KEY_DOWN) PWM(level,led_number);
    }

    PWM(level,led_number);
    
  }
  
}

