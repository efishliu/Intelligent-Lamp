/*******************************************************
ULN2003����5V���ٲ����������
Target:STC89C52RC-40C
Crystal:12MHz
Author:ս��Ƭ��������
Platform:51&avr��Ƭ����Сϵͳ��+ULN2003������������׼�
http://zsmcu.taobao.com   QQ:284083167
*******************************************************
���߷�ʽ:
IN1 ---- P00
IN2 ---- P01
IN3 ---- P02
IN4 ---- P03
+   ---- +5V
-   ---- GND
*********************/
#include<reg52.h>
#define uchar unsigned char
#define uint  unsigned int
#define MotorData P0                    //����������ƽӿڶ���
uchar phasecw[4] ={0x08,0x04,0x02,0x01};//��ת �����ͨ���� D-C-B-A
uchar phaseccw[4]={0x01,0x02,0x04,0x08};//��ת �����ͨ���� A-B-C-D
uchar speed;
//ms��ʱ����
void Delay_xms(uint x)
{
 uint i,j;
 for(i=0;i<x;i++)
  for(j=0;j<112;j++);
}
//˳ʱ��ת��
void MotorCW(void)
{
 uchar i;
 for(i=0;i<4;i++)
  {
   MotorData=phasecw[i];
   Delay_xms(speed);//ת�ٵ���
  }
}
//ֹͣת��
void MotorStop(void)
{
 MotorData=0x00;
}
//������
void main(void)
{
 uint i;
 Delay_xms(50);//�ȴ�ϵͳ�ȶ�
 speed=4;
 while(1)
 {
 for(i=0;i<10;i++)
  {
   MotorCW();  //˳ʱ��ת��
  }  
  speed++;     //���� 
  if(speed>25)  
  {
   speed=4;    //���¿�ʼ�����˶�
   MotorStop();
   Delay_xms(500);
  }  
 }
}
