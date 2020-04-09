## 概述
&emsp;&emsp;这是一个基于Zstack协议栈的智能台灯，它具有温湿度、距离的采集和LED智能控制的功能。实现了通过Zstack协议栈进行数据无线通信和台灯的智能控制。

## 工具和器材
* 1、CC2530开发板*2
* 2、温湿度传感器、测距传感器
* 3、IAR集成开发环境

## 架构概述
&emsp;&emsp;智能台灯通过Zigbee协议进行组网操作，使网络协调器能收到台灯传感器的数据，并通过串口传给计算机，计算机进行相应的显示和控制工作，实现台灯和计算机的无线双向通信。  
<div align=center><img src="https://github.com/efishliu/Intelligent-lamp/blob/master/image/framework.png" width = 50% height = 50% /></div>  

## 主要功能
* **温湿度传感器**  
&emsp;&emsp;它应用专用的数字模块采集技术和温湿度传感技术，通过P0_7口进行温湿度采集。主要通过[DHT11函数](https://github.com/efishliu/Intelligent-lamp/tree/master/Unit-test/humiture-DHT11-test)实现。

* **测距传感器**  
&emsp;&emsp;它应用超声波测距技术，通过TRIG (P2_0)、ECHO(P1_3)触发定时器T1，通过定时器的数据差来计算距离，主要通过[Distance函数](https://github.com/efishliu/Intelligent-lamp/tree/master/Unit-test/distance-test)实现。

* **LED冷暖光灯/LED提示灯**  
&emsp;&emsp;提示灯：它应用PWM调光原理，通过亮度周期来调节LED的闪烁，通过P1_4口来进行调节。主要通过[PWM函数](https://github.com/efishliu/Intelligent-lamp/tree/master/Unit-test/lamp-test)实现。  
&emsp;&emsp;~~冷暖光灯：它应用PWM调光原理，通过亮度占空比来调节LED的亮度，通过P1_0、P1_1口来进行调节。主要通过[PWM函数](https://github.com/efishliu/Intelligent-lamp/tree/master/Unit-test/lamp-test)实现。(存在问题)~~  

* **Windows展示界面（QT实现）**  
&emsp;&emsp;通过串口获取CC2530开发板的各个传感器的数据，并通过可视化的形式进行展示；  
&emsp;&emsp;通过界面区别台灯不同的运行模式，并做出相应的反应；  
&emsp;&emsp;通过调节滑动条调节灯光；  
&emsp;&emsp;通过测距传感器的数据进行坐姿预警；
[QT源码](https://github.com/efishliu/Intelligent-lamp/tree/master/Qt-intelligent-led/project)  

##  智能台灯通信方式
* **通信数据格式**  
&emsp;&emsp;采集数据： C 1 2 3 4 5 6 7  其中12温度、 34湿度、567距离  
&emsp;&emsp;控制命令： D 1 2 3 4 5      其中12亮度、 34色温、5提示灯  
* **通信方式**  
*串口通信：*  
1、串口初始化 MT_UartInit ()  
2、串口登记任务号 MT_UartRegisterTaskID()  
3、串口发送 HalUARTWrite()  
4、串口接收 回传函数MT_UartProcessZToolData()  
*无线通信：*  
1、在应用层SampleApp_Init中初始化配置  
2、使用AF_DataRequest()函数广播发送数据  
3、通过AF_INCOMING_MSG_CMD 事件进行响应，从而完成接收  

## 智能台灯的显示界面
<div align=center><img src="https://github.com/efishliu/Intelligent-lamp/blob/master/image/operation-interface.png" width = 50% height = 50%  /></div>    

