#include "iocc2530.h"
//#include "MT_UART.h"
//#include "MT_APP.h"
//#include "MT.h"
#define uint unsigned int 
#define uchar unsigned char
void Start_I2c(void);                  //��ʼ�ź�
void  Stop_I2c(void);                    //ֹͣ�ź�
void BH1750_SendACK(void);       //Ӧ��ACK
void BH1750_SendNCK(void) ;    //Ӧ��ACK
uchar RcvByte(void);
void  SendByte(unsigned char c);  //IIC�����ֽ�д
uchar ISendByte(uchar sla,uchar c);//
uchar IRcvByte(uchar sla,uchar *c); //IIC�����ֽڶ�
uchar IRcvStrExt(uchar sla,uchar *s,uchar no);        //�����Ķ�ȡ�ڲ��Ĵ������� 
void Init_BH1750(void);//��ʼ
//void conversion(uint temp_data) ;
void Delay_us_1750(uint timeout);
void Delay_ms_1750(uint Time);//n ms��ʱ
void light(void);

#define DPOWR  0X00         //�ϵ�
#define POWER  0X01         //SHANG DIAN
#define RESET    0X07         //CHONG ZHI
#define CHMODE  0X10        //����H�ֱ���
#define CHMODE2 0X11         //����H�ֱ���2
#define CLMODE   0X13           //�����ͷֱ�
#define H1MODE   0X20           //һ��H�ֱ���
#define H1MODE2 0X21          //һ��H�ֱ���2
#define L1MODE    0X23           //һ��L�ֱ���ģʽ
#define  SCL P0_0      //IICʱ�����Ŷ���
#define  SDA P1_2     //IIC�������Ŷ���
#define	 SlaveAddress   0x46 //����������IIC�����еĴӵ�ַ,����ALT  ADDRESS��ַ���Ų�ͬ�޸�
                              //ALT  ADDRESS���Žӵ�ʱ��ַΪ0xA6���ӵ�Դʱ��ַΪ0x3A
uchar ack;
uchar buf[2];                         //�������ݻ�����  
//uint sun;
//uchar lx[5];//ge,shi,bai,qian,wan;            //��ʾ����
//float s;

//-----------------------------------

//void Initial() //ϵͳ��ʼ��
//{
  //CLKCONCMD = 0x80;      //ѡ��32M����
  //while(CLKCONSTA&0x40); //�ȴ������ȶ�
  //UartInitial();         //���ڳ�ʼ��
  //P1SEL &= 0xfb;         //DS18B20��io�ڳ�ʼ��
//}

/****************************
        ��ʱ����
*****************************/
void Delay_us_1750(uint timeout) //1 us��ʱuint16 timeout )
{
  while (timeout--)
  {
    asm("NOP");
    asm("NOP");
    asm("NOP");
  } 
}


void Delay_ms_1750(uint Time)//n ms��ʱ
{
  unsigned char i;
  while(Time--)
  {
    for(i=0;i<100;i++)
     Delay_us_1750(10);
  }
}
//*********************************************************

/**************************************
��ʼ�ź�
**************************************/
void Start_I2c()
{
    P1DIR|=(1<<2);
    P0DIR|=1;
    SDA = 1;                    //����������
    SCL = 1;                    //����ʱ����
    Delay_us_1750(5);                 //��ʱ
    SDA = 0;                    //�����½���
    Delay_us_1750(5);                 //��ʱ
    SCL = 0;                    //����ʱ����
}


/**************************************
ֹͣ�ź�
**************************************/
void  Stop_I2c()
{
     P1DIR|=(1<<2);
    P0DIR|=1;
    SDA = 0;                    //����������
    SCL = 1;                    //����ʱ����
    Delay_us_1750(5);               //��ʱ
    SDA = 1;                    //����������
    Delay_us_1750(5);                //��ʱ
}

/**************************************
����Ӧ���ź�
��ڲ���:ack (0:ACK 1:NAK)
**************************************/
void BH1750_SendACK()
{
     P1DIR|=(1<<2);
    P0DIR|=1;
    SDA = 0;                  //дӦ���ź�
    SCL = 1;                    //����ʱ����
    Delay_us_1750(5);                //��ʱ
    SCL = 0;                    //����ʱ����
    SDA = 1;  
}

void BH1750_SendNCK()
{
      P1DIR|=(1<<2);
    P0DIR|=1;
    SDA = 1;                  //дӦ���ź�
    SCL = 1;                    //����ʱ����
    Delay_us_1750(5);                //��ʱ
    SCL = 0;                    //����ʱ����
    SDA = 0; 
}

/*----------------------------------------------------------------              
I2Cд��һ��8λ������������λ��ǰ��λ�ں�
------------------------------------------------------------------*/
void  SendByte(unsigned char c)
{
 unsigned char BitCnt;
  P1DIR|=(1<<2);
    P0DIR|=1;
 for(BitCnt=0;BitCnt<8;BitCnt++)  //Ҫ���͵����ݳ���Ϊ8λ
    {
     
      if((c<<BitCnt)&0x80) SDA=1;   //�жϷ���λ
       else  SDA=0;                
     SCL=1;               //��ʱ����Ϊ�ߣ�֪ͨ��������ʼ��������λ
     Delay_us_1750(5);         //��֤ʱ�Ӹߵ�ƽ���ڴ���4��      
     SCL=0; 
    }  
     SDA=1;
    Delay_us_1750(5);
    P1DIR&=~(1<<2);          //8λ��������ͷ������ߣ�׼������Ӧ��λ
    SCL=1;
    Delay_us_1750(5);
    if(SDA==1)ack=0;     
       else ack=1;        //�ж��Ƿ���յ�Ӧ���źţ���1����û��
    SCL=0;
}

/*******************************************************************
                 �����ӵ�ַ���������ֽ����ݺ���               
����ԭ��: bit  ISendByte(uchar sla,ucahr c);  
����:     ���������ߵ����͵�ַ�����ݣ��������ߵ�ȫ����,��������ַsla.
          �������1��ʾ�����ɹ��������������
ע�⣺    ʹ��ǰ�����ѽ������ߡ�
********************************************************************/
uchar ISendByte(uchar sla,uchar c)
{
   Start_I2c();               /*��������*/
   SendByte(sla);             /*����������ַ*/
   if(ack==0)return(0);
   SendByte(c);               /*��������*/
   if(ack==0)return(0);
   Stop_I2c();                /*��������*/ 
   return(1);
}

/*******************************************************************
I2C��ȡһ��8λ����������Ҳ�Ǹ�λ��ǰ��λ�ں�  
****************************************************************/	
uchar RcvByte()
{
  unsigned char retc;
  unsigned char BitCnt;
   retc=0; 
   P1DIR&=~(1<<2);         //��������Ϊ���뷽ʽ
  for(BitCnt=0;BitCnt<8;BitCnt++)
      {       
        SCL=0;      
        Delay_us_1750(5); //ʱ�ӵ͵�ƽ���ڴ���4.7us
        if(SDA==1)retc=retc+1; //������λ,���յ�����λ����retc��
        SCL=1;
        retc=retc<<1;
        Delay_us_1750(5);  
      }   
   SCL=0; 
  return(retc);
}

//*********************************************************
//��������BH1750�ڲ�����
//*********************************************************
uchar IRcvStrExt(uchar sla,uchar *s,uchar no)
{   uchar i;	
    Start_I2c();                         //��ʼ�ź�
    SendByte(sla+1);        //�����豸��ַ+���ź�
    if(ack==0)return(0);
      for (i=0; i<no-1; i++)                      //������ȡ6����ַ���ݣ��洢��BUF
      {
        *s=RcvByte();      
        BH1750_SendACK();                //��ӦACK
        s++;
      }		
        *s=RcvByte();
        BH1750_SendNCK();   //���һ��������Ҫ��NOACK             
        Stop_I2c();   
        return(1);
}



//��ʼ��BH1750��������Ҫ��ο�pdf�����޸�****
void Init_BH1750()
{
    P1DIR|=(1<<2);
    P0DIR|=1;
   ISendByte(0x46,0x01);  
}

//*********************************************************
/*void conversion(uint temp_data)  //  ����ת���� ����ʮ���٣�ǧ����
{  
    lx[0]=temp_data/10000+0x30 ;
    temp_data=temp_data%10000;   //ȡ������
     lx[1]=temp_data/1000+0x30 ;
    temp_data=temp_data%1000;    //ȡ������
     lx[2]=temp_data/100+0x30  ;
    temp_data=temp_data%100;     //ȡ������
     lx[3]=temp_data/10+0x30 ;
    temp_data=temp_data%10;     //ȡ������
     lx[4]=temp_data+0x30; 	
}*/
//*********************************************************
//������********
//*********************************************************
void light()
{  
    uchar *p=buf;
    Delay_ms_1750(100);	    //��ʱ100ms	
    Init_BH1750();       //��ʼ��BH1750
    ISendByte(0x46,0x01);   // power on
    ISendByte(0x46,0X20);   // H- resolution mode
    //uchar data[6]="Light="; //������ʾ��
   // char data1[2]="lx"; //��λ
    Delay_ms_1750(180);              //��ʱ180ms
    IRcvStrExt(0x46,p,2);       //�����������ݣ��洢��BUF��
  /*  sunh=buf[0];
    sun=(sun<<8)+buf[1];//�ϳ����ݣ�����������  
    s=(float)sun/1.2;    
    conversion((uint)s);         //�������ݺ���ʾ*/
} 



