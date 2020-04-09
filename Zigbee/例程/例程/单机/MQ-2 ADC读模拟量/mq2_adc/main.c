/****************************************************************************
* �� �� ��: main.c
* ��    ��: Andy
* ��    ��: 2014-03-01
* ��    ��: 1.0
* ��    ��: MQ-2���崫����,������Ũ�ȴ����趨Ũ��ʱ��LED1����˸,MQ-2�ϵ�DD-LED
*          Ҳ�᳤�����������һ��IO�ӷ������Ϳɱ����ˣ��Լ�DIY��!
****************************************************************************/
#include "ioCC2530.h" 
#include "string.h"


typedef unsigned char uchar;
typedef unsigned int  uint;
typedef signed short int16;
typedef unsigned short uint16;

char TxBuf[5];
uint16 GasData;

uint16 ReadGasData( void );

/****************************************************************************
* ��    ��: InitUart()
* ��    ��: ���ڳ�ʼ������
* ��ڲ���: ��
* ���ڲ���: ��
****************************************************************************/
void InitUart(void)
{ 
  PERCFG = 0x00;           //������ƼĴ��� USART 0��IOλ��:0ΪP0��λ��1 
  P0SEL = 0x0c;            //P0_2,P0_3�������ڣ����蹦�ܣ�
  P2DIR &= ~0XC0;          //P0������ΪUART0
  
  U0CSR |= 0x80;           //����ΪUART��ʽ
  U0GCR |= 11;				       
  U0BAUD |= 216;           //��������Ϊ115200
  UTX0IF = 0;              //UART0 TX�жϱ�־��ʼ��λ0
}

/****************************************************************************
* ��    ��: UartSendString()
* ��    ��: ���ڷ��ͺ���
* ��ڲ���: Data:���ͻ�����   len:���ͳ���
* ���ڲ���: ��
****************************************************************************/
void UartSendString(char *Data, int len)
{
  uint i;
  
  for(i=0; i<len; i++)
  {
    U0DBUF = *Data++;
    while(UTX0IF == 0);
    UTX0IF = 0;
  }
}

/****************************************************************************
* ��    ��: DelayMS()
* ��    ��: �Ժ���Ϊ��λ��ʱ 16MʱԼΪ535,32MʱҪ����,ϵͳʱ�Ӳ��޸�Ĭ��Ϊ16M
* ��ڲ���: msec ��ʱ������ֵԽ����ʱԽ��
* ���ڲ���: ��
****************************************************************************/
void DelayMS(uint msec)
{  
  uint i,j;
  
  for (i=0; i<msec; i++)
    for (j=0; j<1070; j++);
}

uint16 ReadGasData( void )
{
  uint16 reading = 0;
  
  /* Enable channel */
  ADCCFG |= 0x40;
  
  /* writing to this register starts the extra conversion */
  ADCCON3 = 0x86;// AVDD5 ����  00�� 64 ��ȡ��(7 λENOB)  0110�� AIN6
  
  /* Wait for the conversion to be done */
  while (!(ADCCON1 & 0x80));
  
  /* Disable channel after done conversion */
  ADCCFG &= (0x40 ^ 0xFF); //��λ�����1010^1111=0101�������ƣ�
  
  /* Read the result */
  reading = ADCL;
  reading |= (int16) (ADCH << 8); 
  
  reading >>= 8;
  
  return (reading);
}

void main(void)
{
  CLKCONCMD &= ~0x40;         //����ϵͳʱ��ԴΪ32MHZ����
  while(CLKCONSTA & 0x40);    //�ȴ������ȶ�Ϊ32M
  CLKCONCMD &= ~0x47;         //����ϵͳ��ʱ��Ƶ��Ϊ32MHZ  
  
  InitUart();                   //���ô�����ؼĴ���
  
  while(1)
  {
    GasData = ReadGasData();  //��ȡ�������������ϵ�adת��ֵ����û�л�����ܱ�ʾ����Ũ�ȵ�ֵ
    //��ʾ���ʹ��2530оƬ��AD���ܣ��������������и���
    
    //��ȡ������ֵת�����ַ����������ں������
    TxBuf[0] = GasData / 100 + '0';
    TxBuf[1] = GasData / 10%10 + '0';
    TxBuf[2] = GasData % 10 + '0';
    TxBuf[3] = '\n';
    TxBuf[4] = 0;
    
    UartSendString(TxBuf, 4); //�봮�������ͳ����ݣ���������115200      
    DelayMS(2000);            //��ʱ����
  }
}


