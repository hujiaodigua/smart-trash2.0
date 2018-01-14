#include "motor.h"
#include "led.h"
#include "key.h"
#include "delay.h"
/**********************************************
注:判断是否有垃圾写了一个查询函数
两个开关控制电机则使用外部中断，在exti.c中写有
**********************************************/
int JudegTrash(float m)
{ 
		//int k;
	
		if(m > 100)    //超过100g认为有垃圾
		{
			LED0 = 1;								//减速电机正转
			LED1 = 0;
			
			return 1; 	 
		}
		return 0;
}
