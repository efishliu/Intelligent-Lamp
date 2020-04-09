#include <ioCC2530.h>

typedef unsigned char uchar;
typedef unsigned int  uint;

#define DATA_PIN P0_7

/*
vcc Һ���� 6
GND Һ���� 8
DATA P0_7

*/

//��ʪ�ȶ���
uchar ucharFLAG,uchartemp;
uchar shidu_shi,shidu_ge,wendu_shi,wendu_ge=4;
uchar ucharT_data_H,ucharT_data_L,ucharRH_data_H,ucharRH_data_L,ucharcheckdata;
uchar ucharT_data_H_temp,ucharT_data_L_temp,ucharRH_data_H_temp,ucharRH_data_L_temp,ucharcheckdata_temp;
uchar ucharcomdata;
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

/*
//��ʪ����ʱ����
void Delay_us() //1 us��ʱ
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

void Delay_10us() //10 us��ʱ
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

void Delay_ms(uint Time)//n ms��ʱ
{
    unsigned char i;
    while(Time--)
    {
        for(i=0;i<100;i++)
            Delay_10us();
    }
}
*/
//��ʪ�ȴ���
void COM(void)    // ��ʪд��
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

void DHT11(void)   //��ʪ��������
{
    DATA_PIN=0;
    Delay_ms(19);  //>18MS
    DATA_PIN=1; 
    P0DIR &= ~0x80; //��������IO�ڷ���
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
    else //û�óɹ���ȡ������0
    {
        wendu_shi=0; 
        wendu_ge=0;
        
        shidu_shi=0; 
        shidu_ge=0;  
    } 
    
    P0DIR |= 0x80; //IO����Ҫ�������� 
}

void CLK_INIT(void)                                         
{ 
  
  //unsigned int i;                                                                                                                     
  //SLEEPCMD &= ~0x04;               /* power on 16MHz RC and 32MHz XOSC */                
  //while (!(SLEEPSTA &0x40));          /* wait for 32MHz XOSC stable */
  //asm("NOP");                     
  //for (i=0; i<504; i++) asm("NOP");          /* Require 63us delay for all revs */                
  CLKCONCMD &= ~0x40;     //����ϵͳʱ��ԴΪ32MHZ����
  while(CLKCONSTA & 0x40);   //�ȴ������ȶ�
  CLKCONCMD &= ~0x47;    //����ϵͳ��ʱ��Ƶ��Ϊ32MHZ
               //��ʱ��CLKCONSTAΪ0x88������ͨʱ�ӺͶ�ʱ��ʱ�Ӷ���32M��                        /* turn off 16MHz RC */                                                                
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

  PERCFG = 0x00;    //λ��1 P0�� 
  P0SEL = 0x0c;    //P0�������� 
  P2DIR &= ~0XC0;                             //P0������ΪUART0     
 
  U0CSR |= 0x80;    //��������ΪUART��ʽ 
  U0GCR |= 8;     
  U0BAUD |= 59;    //��������Ϊ9600 
  UTX0IF = 1;  
  U0CSR |= 0X40;    //������� 
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