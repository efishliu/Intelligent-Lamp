#include <ioCC2530.h>

#define COLD_LED      P1_0    //����
#define HOT_LED       P1_1    //ů���
#define ALERT_LED     P1_4    //��ʾ��

#define DATA_PIN      P0_7    //��ʪ��

#define TRIG          P2_0    //���
#define ECHO          P1_3    

#define LED_MODE_ON   0       //LED�ƿ�
#define LED_MODE_OFF  1

#define LED_CYCLE_TIME  1000  //��������

typedef unsigned char uchar;
typedef unsigned int  uint;

int receiveflag = 0;
void alert_led();
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

 /************************************************* 
* 
*�������ƣ�void LED_Init(void) 
*������������ʼ��LED��
*����˵������ 
* 
**************************************************/ 
 void Led_Init(void) 
 { 
   P1SEL &= ~0X13;        //����P1_4,P1_1,P1_0Ϊ��ͨIO   //0001 0011 
   P1DIR |= 0X13;         //����P1_4,P1_1,P1_0Ϊ��� 
   
   //��ʼ�����TRIG
   P2SEL &= ~0X01; //���� P2_0 Ϊ��ͨ IO //0001 0000
   P2DIR |= 0X01;          
      
   //��ʼ��ȫ��LED��
   COLD_LED = LED_MODE_OFF;   //LED1��Ĭ��Ϊ�ر�״̬ 
   HOT_LED  = LED_MODE_OFF ;  //LED3��Ĭ��Ϊ�ر�״̬
   ALERT_LED = LED_MODE_OFF;   //LED3��Ĭ��Ϊ�ر�״̬
} 
 /************************************************* 
* 
*�������ƣ�void Uart0_Init(void��
*������������ʼ������
*����˵������ 
* 
**************************************************/ 
void Uart0_Init(void)
{  

  PERCFG = 0x00;    //λ��1 P0�� 
  P0SEL = 0x0c;    //P0�������� 
  P2DIR &= ~0XC0;                             //P0������ΪUART0     
 
  U0CSR |= 0x80;    //��������ΪUART��ʽ 
  U0GCR |= 8;   
 
  U0BAUD |= 59;    //��������Ϊ9600 
  UTX0IF = 0;  
  U0CSR |= 0X40;    //������� 
  IEN0 |= 0x84;
  
/*
    PERCFG = 0x00;   // ????,UART0??????1 
    P0SEL = 0x0c;    // ??0????,P0_2?P0_3????
    P2DIR &= ~0xC0;  // ??0???????,?2??0,USART0??

    U0CSR |= 0x80;   // UART??
    U0GCR |= 11;
    U0UCR |=0x80; //�����ƽ�ֹ;
    U0BAUD |= 59;   // ??32MHz?????,??????115200
    UTX0IF = 0; 
     //ʹ���ж�
    EA=1; //�ж��ܿ���// UART0 TX????????0
  */
  EA=1;
}

unsigned char rxTemp = 0; //������������ʱ����
unsigned int rb_count = 0; //receiece_buffer count
unsigned char recieve_buf[6];//��������
 /************************************************* 
* 
*�������ƣ�Uart0_Send_String(unsigned char *Data,int len)
*�������������������ݷ��͵�����
*����˵������ 
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
*�������ƣ�Uart0_Send_String(unsigned char *Data,int len)
*�������������������ݷ��͵�����
*����˵������ 
* 
**************************************************/ 
void Usart0_Receive_String()
{
  if(rxTemp != 0)       //���յ�����
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
�� �� �� : Uart0_ISR 
70. �������� : �жϷ����� 
71. ������� : NONE 
72. ������� : NONE 
73. �� �� ֵ : NONE 
74. ***************************************************/ 

#pragma vector = URX0_VECTOR  
__interrupt void Uart0_ISR(void) 
{ 
 
  URX0IF = 0; // ���жϱ�־ 
  while(!U0DBUF);
  rxTemp = U0DBUF;//��ȡ���յ������� 
  Usart0_Receive_String();

}

 /************************************************* 
* 
*�������ƣ�void Delay()
*��������������ʱ����
*����˵������ 
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
void Delay_ms(uint n)//n ms��ʱ
{
    uint i;
    while(n--)
    {
      for(i = 0;i < 1000;++i)
        Delay_us(1);
    }
}
void Delay_s(uint n)  //n s��ʱ
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

//��ʪ����ʱ����
void wDelay_us() //1 us��ʱ
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

void wDelay_10us() //10 us��ʱ
{
uchar i=18;
  for(;i>0;i--);  
}

void wDelay_ms(uint Time)//n ms��ʱ
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
*�������ƣ�void PMW(int lightlevel,int colorlevel)
*����������LED����
*����˵����lightlevel ����,colorlevel ɫ��
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
        if(colorlevel >= 50)  //�������ʱ����
        {      
          COLD_LED = LED_MODE_ON;
          HOT_LED = LED_MODE_ON;
          Delay(cold_led_time);
          COLD_LED = LED_MODE_OFF;
          Delay(hot_led_time - cold_led_time);
          HOT_LED = LED_MODE_OFF;
          Delay(LED_CYCLE_TIME - hot_led_time);
      }
        else          //ů������ʱ����
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
*�������ƣ�void alert_led()
*������������ʾ����˸����
*����˵������
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

//��ʪ�ȶ���
uchar ucharFLAG,uchartemp;
uchar shidu_shi,shidu_ge,wendu_shi,wendu_ge=4;
uchar ucharT_data_H,ucharT_data_L,ucharRH_data_H,ucharRH_data_L,ucharcheckdata;
uchar ucharT_data_H_temp,ucharT_data_L_temp,ucharRH_data_H_temp,ucharRH_data_L_temp,ucharcheckdata_temp;
uchar ucharcomdata;

//��ʪ�ȴ���
 /************************************************* 
* 
*�������ƣ�void COM(void)
*������������ʪд��
*����˵������
* 
**************************************************/ 
void COM(void)    // ��ʪд��
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
*�������ƣ�void DHT11(void)
*������������ʪ��������
*����˵������
* 
**************************************************/ 
void DHT11(void)   //��ʪ��������
{
    DATA_PIN=0;
    wDelay_ms(19);  //>18MS
    DATA_PIN=1; 
    P0DIR &= ~0x80; //��������IO�ڷ���
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
    else //û�óɹ���ȡ������0
    {
        wendu_shi=0; 
        wendu_ge=0;
        
        shidu_shi=0; 
        shidu_ge=0;  
    } 
    
    P0DIR |= 0x80; //IO����Ҫ�������� 
}

//���


uchar count_start; 
uchar H1;
uchar H2;
uchar L2;
uchar L1;
uchar cycle;

 /************************************************* 
* 
*�������ƣ�void UltrasoundRanging1()
*���������������ഫ����
*����˵������
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
*�������ƣ�Init_UltrasoundRanging()
*������������ʼ���˿�
*����˵������
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
*�������ƣ�void P1_ISR(void)
*�����������жϷ�����
*����˵������
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
*�������ƣ�float caldistance()
*������������ຯ��
*����˵�������ؾ���(cm_)
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
*�������ƣ�inputdistance(unsigned char *send_buf,unsigned int distance)
*���������������뻯Ϊ�����ַ���
*����˵�������ؾ���(cm_)
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