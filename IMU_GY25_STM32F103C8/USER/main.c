#include "led.h"
#include "delay.h"
#include "sys.h"
#include "key.h"
#include "usart.h"

//ALIENTEK Mini STM32开发板范例代码3
//串口实验
//技术论坛:www.openedv.com	
	void dis_play(int16_t num,u8 n)	
{
	if(num<0)
	{
		num=-num;
		USART_SendData(USART1,'-');
	}
	else
		USART_SendData(USART1,'+');
	//if(n)
	USART_SendData(USART1,0x30|(num/10000));	
	USART_SendData(USART1,0x30|(num%10000/1000));
	USART_SendData(USART1,0x30|(num%1000/100));
	USART_SendData(USART1,0x2e);
	USART_SendData(USART1,0x30|(num%100/10));
	USART_SendData(USART1,0x30|(num%10));
	USART_SendData(USART1,',');
}
 int main(void)
 {
	
	u8 t;
	u8 len;	
	static u8  times=0;  	
 	SystemInit();//系统时钟等初始化
	delay_init(72);	     //延时初始化
	NVIC_Configuration();//设置NVIC中断分组2:2位抢占优先级，2位响应优先级
	uart_init(115200);//串口初始化为115200
	while(1)
	{
		if(flag==0)
		{
		  USART_SendData(USART1,0xa5);
			USART_SendData(USART1,0x51);//单一查询指令
		}
		delay_ms(10);
//		if(times==0)
//		{
//			times=1;//只发一次
//		  USART_SendData(USART1,0xa5);
//			USART_SendData(USART1,0x52);//自动发送16进制数据指令
//		}
		
		if(flag==1)
		{	
			 flag=0;
			 USART_SendData(USART1,'#');
	     USART_SendData(USART1,'Y');
	     USART_SendData(USART1,'P');
	     USART_SendData(USART1,'R');
	     USART_SendData(USART1,'=');
			 dis_play(YAW,1);
			 dis_play(PITCH,0);
			 dis_play(ROLL,0);
			 USART_SendData(USART1,0X0d);
	     USART_SendData(USART1,0X0a);
		}
	}	 

 }

