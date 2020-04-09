/****************************************************************************
* 文 件 名: main.c
* 作    者: Andy
* 修    订: 2014-10-08
* 版    本: 1.0
* 描    述: 用P04 05 06 07控制步进电机
****************************************************************************/
#include <ioCC2530.h>

typedef unsigned char uchar;
typedef unsigned int  uint;


#define A1 P1_4 //定义步进电机连接端口
#define B1 P1_5
#define C1 P1_6
#define D1 P1_7

uchar phasecw[4] ={0x80,0x40,0x20,0x10};//正转 电机导通相序 D-C-B-A
uchar phaseccw[4]={0x10,0x20,0x40,0x80};//反转 电机导通相序 A-B-C-D

void MotorData(uchar data)
{
  A1 = 1&(data>>4);
  B1 = 1&(data>>5);
  C1 = 1&(data>>6);
  D1 = 1&(data>>7);
}

//ms延时函数
void Delay_MS(uint x)
{
  uint i,j;
  for(i=0;i<x;i++)
    for(j=0;j<535;j++);
}
void MotorStop(void)
{
  MotorData(0x00);
}
//顺时针转动
void MotorCW(uchar Speed)
{
  uchar i,j;
  for(j=0;j<150;j++)
  {
    for(i=0;i<4;i++)
    {
      MotorData(phasecw[i]);
      Delay_MS(Speed);//转速调节
    }
  }
   MotorStop();
}
//逆时针转动
void MotorCCW(uchar Speed)
{
  uchar i,j;
  for(j=0;j<150;j++)
  {
    for(i=0;i<4;i++)
    {
      MotorData(phaseccw[i]);
      Delay_MS(Speed);//转速调节
    }
  }
  MotorStop();
}

//停止转动


/****************************************************************************
* 名    称: InitIO()
* 功    能: 初始化IO口程序
* 入口参数: 无
* 出口参数: 无
****************************************************************************/
void InitMotor(void)
{
  P1SEL &= 0x0F;  //P04 05 06 07定义为普通IO
  P1DIR |= 0xF0;  //P04 05 06 07定义为输出
}


