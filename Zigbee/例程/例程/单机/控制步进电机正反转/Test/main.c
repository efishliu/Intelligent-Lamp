/****************************************************************************
* �� �� ��: main.c
* ��    ��: Andy
* ��    ��: 2014-10-08
* ��    ��: 1.0
* ��    ��: ��P04 05 06 07���Ʋ������
****************************************************************************/
#include <ioCC2530.h>

typedef unsigned char uchar;
typedef unsigned int  uint;


#define A1 P0_4 //���岽��������Ӷ˿�
#define B1 P0_5
#define C1 P0_6
#define D1 P0_7

uchar phasecw[4] ={0x80,0x40,0x20,0x10};//��ת �����ͨ���� D-C-B-A
uchar phaseccw[4]={0x10,0x20,0x40,0x80};//��ת �����ͨ���� A-B-C-D

void MotorData(uchar data)
{
  A1 = 1&(data>>4);
  B1 = 1&(data>>5);
  C1 = 1&(data>>6);
  D1 = 1&(data>>7);
}

//ms��ʱ����
void Delay_MS(uint x)
{
  uint i,j;
  for(i=0;i<x;i++)
    for(j=0;j<535;j++);
}

//˳ʱ��ת��
void MotorCW(uchar Speed)
{
  uchar i;
  for(i=0;i<4;i++)
  {
    MotorData(phasecw[i]);
    Delay_MS(Speed);//ת�ٵ���
  }
}
//��ʱ��ת��
void MotorCCW(uchar Speed)
{
  uchar i;
  for(i=0;i<4;i++)
  {
    MotorData(phaseccw[i]);
    Delay_MS(Speed);//ת�ٵ���
  }
}

//ֹͣת��
void MotorStop(void)
{
  MotorData(0x00);
}

/****************************************************************************
* ��    ��: InitIO()
* ��    ��: ��ʼ��IO�ڳ���
* ��ڲ���: ��
* ���ڲ���: ��
****************************************************************************/
void InitIO(void)
{
  P0SEL &= 0x0F;  //P04 05 06 07����Ϊ��ͨIO
  P0DIR |= 0xF0;  //P04 05 06 07����Ϊ���
}

/****************************************************************************
* ������ں���
****************************************************************************/
void main(void)
{
  uint i;
  uchar ucSpeed;
  
  InitIO();
  
  //�ı�����������Ե������ת�٣�����ԽС��ת��Խ��,����ԽС
  ucSpeed = 3;    //�����ٶ�   ������2-10��Χ��

  Delay_MS(50);   //�ȴ�ϵͳ�ȶ�
  while(1)
  {
    for(i=0;i<500;i++)
    {
      MotorCW(ucSpeed); //˳ʱ��ת��
    } 
    MotorStop();        //ֹͣת��   
    Delay_MS(2000);
    
    for(i=0;i<500;i++)
    {
      MotorCCW(ucSpeed);//��ʱ��ת��
    } 
    
    MotorStop();        //ֹͣת��
    Delay_MS(2000);  
  }
  
}
