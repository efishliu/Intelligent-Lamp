#include <ioCC2530.h>

#define S5 P0_4            //�궨��P0_4ΪS5�Ŀ��ƶ˿� 
#define S6 P0_5            //�궨��P0_5ΪS6�Ŀ��ƶ˿� 
#define LED1   P1_4        //����P1_4ΪLED1�Ŀ��ƶ˿� 
#define LED2   P1_1        //����P1_1ΪLED2�Ŀ��ƶ˿� 
#define LED3   P1_0        //����P1_0ΪLED3�Ŀ��ƶ˿� 


#define KEY_UP   1         //�궨�尴���ɿ�Ϊ1 
#define KEY_DOWN 0         //�궨�尴������Ϊ0 

#define LED_MODE_ON    0   //����0ΪLED�ƴ� 
#define LED_MODE_OFF   1   //����0ΪLED�ƹر� 

#define LED_MAX_LEVEL   5  //LED�Ƶ�������ȵȼ�
#define LED_MAX_NUMBER  3  //LED�Ƶ�����

#define CRITICAL_TIME  150 //�������ٽ�ʱ��

int circle_time = 1000;    //��������

 /************************************************* 
* 
*�������ƣ�void  Sysclk_Init(void) 
*���������������ȶ�
*����˵������ 
* 
**************************************************/ 
 void Sysclk_Init(void)
{
   CLKCONCMD &= ~0x40;
   while( CLKCONSTA&0x40); //�ȴ������ȶ� 
   CLKCONCMD &=~0x47;
}

/**************************** 
* ��ʱ 
*****************************/ 
void Delay( int n) 
{  
  for( int j = 0;j <5;++j)
    for( int i = 0;i<n;++i);
} 

 /************************************************* 
* 
*�������ƣ�void  LED_Init(void) 
*����������LED����ӦIO�ڵ����� 
*����˵������ 
* 
**************************************************/ 
 void  Led_Init(void) 
 { 
   P1SEL &= ~0X13;        //����P1_4,P1_1,P1_0Ϊ��ͨIO   //0001 0011 
   P1DIR |= 0X13;         //����P1_4,P1_1,P1_0Ϊ��� 
      
   //��ʼ��ȫ��LED��
   LED1 = LED_MODE_OFF;   //LED1��Ĭ��Ϊ�ر�״̬ 
   LED2 = LED_MODE_OFF ;  //LED3��Ĭ��Ϊ�ر�״̬
   LED3 = LED_MODE_OFF;   //LED3��Ĭ��Ϊ�ر�״̬
} 

 /************************************************* 
* 
*�������ƣ�void  KEY_Init(void) 
*����������������ӦIO�ڵ����� 
*����˵������ 
* 
**************************************************/ 
 void Key_Init(void) 
 { 
   //S5--->KEY1 = P04,S5--->KEY2 = P05   
    P0SEL &= ~0X30;   //����P0_4 ,P0_5Ϊ��ͨIO 
    P0DIR &= ~0X30;   //����P0_4 ,P0_5Ϊ���� 
    S5 = KEY_UP;
    S6 = KEY_UP;
 } 


 /************************************************* 
* 
*�������ƣ�void  PWM(int level,int led_number)
*����������LED���� 
*����˵����levelΪ�����̶ȣ�led_numberΪLED���
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
   int level = 0;               //LED���ȵȼ�
   int led_number = 1;          //LED���
   int close_last_time = 0;     //�����رհ�ť����ʱ��
   int twinkle_last_time = 0;   //������˸�任��ť����ʱ��
   Led_Init();
   Key_Init();
  
  while(1)
  {    
    if(S5 == KEY_DOWN && S6 == KEY_UP )  //s5���£�s6δ����
    {
      
      --level;                  //�������ȵȼ�
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
            if(twinkle_last_time > CRITICAL_TIME)   //�ﵽ�����ٽ�ʱ��
            {
              circle_time = 10000;         //�任��������
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

    if(S6 == KEY_DOWN && S5 == KEY_UP)  //s6���£�s5δ����
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
    
    if(S5 == KEY_DOWN && S6 == KEY_DOWN) //s5,s6ͬʱ����,�л�LED��
    {
      ++led_number;
      if(led_number > LED_MAX_NUMBER ) led_number = 1;
      level = 1;
      while(S6 == KEY_DOWN || S5 == KEY_DOWN) PWM(level,led_number);
    }

    PWM(level,led_number);
    
  }
  
}

