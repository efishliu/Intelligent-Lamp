#ifndef __BH1750_H__
#define __BH1750_H__

#define uint unsigned int 
#define uchar unsigned char
extern uchar buf[2];//lx[5];//ge,shi,bai,qian,wan;            //��ʾ����
extern void light(void);
extern void Init_BH1750(void);
#endif