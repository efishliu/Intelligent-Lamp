#include "iocc2530.h"
#include "stdio.h"
#include "string.h"

#define LED1 P1_4
#define TRIG1 P2_0
#define uint unsigned int
#define uchar unsigned char
/*

vcc 接 +5v J6 2
GND 接 J6 10
TRIG 接 P2_0
ECHO 接 P1_3

*/

float D;
float count=0; 
uchar count_start; 
uchar H1;
uchar H2;
uchar L2;
uchar L1;
uchar cycle; 
char buffer[30];


void setSysClock()
{
    CLKCONCMD &= ~0x40;           //32MHz 
    while(CLKCONSTA & 0x40);      
    CLKCONCMD &= ~0x47;           
}

void Led_Init(void) {
 //P1SEL&=~0X08;
 //P1DIR&=~0x08;
  //LED1 灯默认为关闭状态
  P2SEL &= ~0X01; //定义 P2_0 为普通 IO //0001 0000
  P2DIR |=0X01;
  
  P1SEL &= ~0X10; //定义 P1_4 为普通 IO //0001 0000
  P1DIR |= 0X10; 
 
}
void Delay_1us(uint microSecs) 
{  
  while(microSecs--) 
  {    /* 32 NOPs == 1 usecs ???????????,??31?nop*/ 
    asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); 
    asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); 
    asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); 
    asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); 
    asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); 
    asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); 
    asm("nop"); //asm('nop')???????,??????1/32M
  } 
} 
void Delay_10us(uint n)
{ /* 320NOPs == 10usecs 因为延时还有计算的缘故，用了310个nop*/
uint tt,yy;
for(tt = 0;tt<n;tt++);
for(yy = 310;yy>0;yy--);
{asm("NOP");}
}

void Delay_1s(uint n)
{ uint ulloop=1000;
uint tt;
for(tt =n ;tt>0;tt--);
for( ulloop=1000;ulloop>0;ulloop--)
{
Delay_10us(100);
}
}
 /* 
void time_init(){ 
  
  T3CTL|=0X08;   //t3测回响波时间
  T3IE=1;
  T3CTL|=0XE0;     //128分频
  T3CTL&=~0X04;
  T3CC0=0X00;
  
  T3CCTL0=0X43;
  P1SEL|=(1<<3);
  
  T3CTL|=0X10;
 EA=1;
}*/

void UltrasoundRanging1() 
{   
  EA = 0; 
  TRIG1 =1;
  
  Delay_1us(10);       //????10us?????? 
  TRIG1 =0;
  
  T1CNTL=0; 
  T1CNTH=0;  
  
  while(!P1_3);   
  T1CTL = 0x09;        //??0,????,32??;??????(0x0000->0xffff); 
  L1=T1CNTL;  
  H1=T1CNTH;
  EA = 1;  
   Delay_10us(60000);     
  Delay_10us(60000); 
} 

/*+++++++++++串口函数++++++++++*/
 
void uart1Init()
{
    PERCFG = 0x00;   // ????,UART0??????1 
    P0SEL = 0x0c;    // ??0????,P0_2?P0_3????
    P2DIR &= ~0xC0;  // ??0???????,?2??0,USART0??

    U0CSR |= 0x80;   // UART??
    U0GCR |= 11;
    U0UCR |=0x80; //流控制禁止;
    U0BAUD |= 216;   // ??32MHz?????,??????115200
    UTX0IF = 0; 
     //使能中断
    EA=1; //中断总开关// UART0 TX????????0

}


void uart0SendStr(char *str,int len)
{
   for(int i=0;i<len;i++) {
        U0DBUF = *str++;
        while(UTX0IF != 1);  // 
        UTX0IF = 0; 
    }
   
   
}

void Init_UltrasoundRanging() 
{  
  P1DIR |= 0x0d;        //0???1???  00100000  ??TRIG P1_1????? 
  TRIG1=0; //?TRIG ??????
  
  
  P1INP &= ~0x08;   
  P1IEN |= 0x08;       //P1_3 ???? 
  PICTL |= 0x02;       //??P0_6??,???????   
  IEN2 |= 0x10;        // ??0???? P0IE = 1; 
  P1IFG = 0;  
  
} 


/* #pragma vector= T3_VECTOR
__interrupt void T3_ISR(){
  if(T3CH0IF==1){
    if(P1_3==1){
      cycle=0;
      count=0;
      count_start=T3CC0;
      LED1=0;
    }
   else{
     if(
      count=T3CC0-count_start+0xff*cycle;
      float distance=count*0.068;             //计算距离
      uart0SendStr(buffer,sprintf(buffer,"%f cm\r\n",distance));//串口发送
      LED1=1;
        
      
  }
  }
 else
  {
    cycle++;
  }
  T3IF=0X00;
}
*/

#pragma vector = P1INT_VECTOR 
__interrupt void P1_ISR(void) 
{ 
  EA=0;  
  L2=T1CNTL;  
  H2=T1CNTH;  
  
  if(P1IFG&0x08)          //??ECHO???? 
  {    
    P1IFG = 0;        
  } 
  else if(P1IFG&0x08)
  {
    P1IFG = 0;  
  }
  P1IF = 0;             //????? 
  
} 
float cal(){
  uint y;
  float distance;
  y=H2*256+L2-L1-256*H1;
  distance=(float)y*340/10000;
  return distance;
}

  void main(){
    
    setSysClock();
    Led_Init();
    Init_UltrasoundRanging();
    uart1Init();
  
   while(1){
    
     for(int i=0;i<10;i++){
     UltrasoundRanging1();
        
     D=cal();
     count+=D;
     
     Delay_10us(60000); 
     } 
    uart0SendStr(buffer,sprintf(buffer,"distance is %.2f cm\r\n",count/10));
    count=0;
   
   }  
    
  
}