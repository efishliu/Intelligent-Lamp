#include <ioCC2530.h> //包含头文件，一般情况不需要改动，头文件包含特殊功能寄存器的定义
#include "BH1750.h"
//uchar buf[2];

/****************************************************************************
* 名    称: InitUart()
* 功    能: 串口初始化函数
* 入口参数: 无
* 出口参数: 无
****************************************************************************/
void InitUart(void)
{ 
    PERCFG = 0x00;           //外设控制寄存器 USART 0的IO位置:0为P0口位置1 
    P0SEL = 0x0c;            //P0_2,P0_3用作串口（外设功能）
    P2DIR &= ~0xC0;          //P0优先作为UART0
    
    U0CSR |= 0x80;           //设置为UART方式
    U0GCR |= 11;				       
    U0BAUD |= 216;           //波特率设为115200
    UTX0IF = 0;              //UART0 TX中断标志初始置位0
    U0CSR |= 0x40;           //允许接收 
    IEN0 |= 0x84;            //开总中断允许接收中断  
}
/****************************************************************************
* 名    称: UartSendString()
* 功    能: 串口发送函数
* 入口参数: Data:发送缓冲区   len:发送长度
* 出口参数: 无
****************************************************************************/
void UartSenddata(char *Data,char len)
{
    unsigned int i;
  
    for(i=0;i<len;i++)
    {
        U0DBUF = *Data ++;
        while(UTX0IF == 0);
        UTX0IF = 0;
    }
   
}
/*------------------------------------------------
主函数
------------------------------------------------*/
void main()
{
  unsigned int str;     // 定义临时变量
  
  CLKCONCMD &= ~0x40;                        //设置系统时钟源为32MHZ晶振
  while(CLKCONSTA & 0x40);                   //等待晶振稳定为32M
  CLKCONCMD &= ~0x47;                        //设置系统主时钟频率为32MHZ   
  
  InitUart();
  
  while(1)
  {  
    light();
    
    UartSenddata(buf,2);
    Delay_ms(200);
     Delay_ms(200);
      Delay_ms(200);
       Delay_ms(200);
        Delay_ms(200);
  }
}