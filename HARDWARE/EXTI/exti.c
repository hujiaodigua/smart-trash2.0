#include "exti.h"
#include "led.h"
#include "key.h"
#include "delay.h"
#include "usart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "motor.h"

#include "hx711.h"
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//Mini STM32开发板
//外部中断 驱动代码			   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//修改日期:2010/12/01  
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 正点原子 2009-2019
//All rights reserved	  
////////////////////////////////////////////////////////////////////////////////// 	  
 
u8 key_wakeup = 0; 	//默认值0，投放按键按下1，需要挂起任务2
 
extern u8 send_trash_enable; 
extern float Weight_Shiwu;
extern float Weight_Send; 
 
//外部中断初始化函数
void EXTIX_Init(void)
{
 
	EXTI_InitTypeDef EXTI_InitStructure;	
	NVIC_InitTypeDef NVIC_InitStructure;

  	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);//外部中断，需要使能AFIO时钟

	KEY_Init();//初始化按键对应io模式

    //GPIOC.5 中断线以及中断初始化配置
  	GPIO_EXTILineConfig(GPIO_PortSourceGPIOC,GPIO_PinSource5);

  	EXTI_InitStructure.EXTI_Line=EXTI_Line5;
  	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;	
  	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;//下降沿触发
  	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  	EXTI_Init(&EXTI_InitStructure);	 	//根据EXTI_InitStruct中指定的参数初始化外设EXTI寄存器
 
	  //GPIOA.15	  中断线以及中断初始化配置
  	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA,GPIO_PinSource15);

  	EXTI_InitStructure.EXTI_Line=EXTI_Line15;
  	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;	
  	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
  	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  	EXTI_Init(&EXTI_InitStructure);	  	//根据EXTI_InitStruct中指定的参数初始化外设EXTI寄存器
	
    //GPIOA.0 中断线以及中断初始化配置
  	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA,GPIO_PinSource0);

  	EXTI_InitStructure.EXTI_Line=EXTI_Line0;
  	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;	
  	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;//上升沿触发
  	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  	EXTI_Init(&EXTI_InitStructure);	 	//根据EXTI_InitStruct中指定的参数初始化外设EXTI寄存器

		
	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;			//使能按键所在的外部中断通道
  	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x06;//抢占优先级6 
  	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x02;		//子优先级0
  	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				//使能外部中断通道
  	NVIC_Init(&NVIC_InitStructure); 
		
		
  NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;			//使能按键所在的外部中断通道
  	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x06;	//抢占优先级2， 
  	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x03;					//子优先级1
  	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;								//使能外部中断通道
  	NVIC_Init(&NVIC_InitStructure); 
		
  NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;			//使能按键所在的外部中断通道
  	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x06;	//抢占优先级5 
  	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x01;					//子优先级1
  	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;								//使能外部中断通道
  	NVIC_Init(&NVIC_InitStructure); 		
		
}


//任务句柄
extern TaskHandle_t Task1Task_Handler;

void EXTI9_5_IRQHandler(void)
{	
	//BaseType_t YieldRequired;	
	delay_xms(20);   //消抖			 
	if(KEY0==0)	
	{
		
			LED0 = 1; 	//先让减速电机停转
			LED1 = 1;
			delay_xms(50);   //给电机一个缓冲的时间		
			LED0 = 0; 	//再让减速电机反转
			LED1 = 1;	
			
//		YieldRequired=xTaskResumeFromISR(Task2Task_Handler);//恢复任务2
//		printf("恢复任务2的运行!\r\n");
//		if(YieldRequired==pdTRUE)
//		{
//			/*如果函数xTaskResumeFromISR()返回值为pdTRUE，那么说明要恢复的这个
//			任务的任务优先级等于或者高于正在运行的任务(被中断打断的任务),所以在
//			退出中断的时候一定要进行上下文切换！*/
//			portYIELD_FROM_ISR(YieldRequired);
//		}
	}
 	 EXTI_ClearITPendingBit(EXTI_Line5);    //清除LINE5上的中断标志位  
}


void EXTI15_10_IRQHandler(void)
{
	BaseType_t YieldRequired;	
  delay_xms(20);    //消抖			 
  if(KEY1==0)			 
	{
		LED0 = 1;			  //减速电机停转
		LED1 = 1;
		
		key_wakeup = 0;		//不一定非要挂起任务，让任务中的函数不执行也是可以的
		printf("行程开关中key_wakeup值：%d\r\n",key_wakeup);
		YieldRequired=xTaskResumeFromISR(Task1Task_Handler);//恢复任务2
		printf("恢复任务1的运行!\r\n");
		if(YieldRequired==pdTRUE)
		{
			portYIELD_FROM_ISR(YieldRequired);
		}
	}	
	 EXTI_ClearITPendingBit(EXTI_Line15);  //清除LINE15线路挂起位
}

void EXTI0_IRQHandler(void)
{
	if(Weight_Shiwu > 200)	//真的有垃圾执行处理函数
		{
			delay_xms(100);   //消抖
			if(WK_UP==1)			 
			{
				printf("垃圾投放完毕");
				key_wakeup = 1;
				printf("投放按键中key_wakeup值：%d\r\n",key_wakeup);
			}
			//立即执行处理函数
			if(key_wakeup == 1) 	//WKUP按键被按下意味投放者确认自己垃圾投放完毕 查询响应速度太慢绝对不行，必须写成全局中断
			{
				if(JudegTrash(Weight_Shiwu))
				{
					send_trash_enable = 1; //允许key里发送重量数据
					Weight_Send = Weight_Shiwu; //有垃圾了，就把垃圾的重量保存起来,因为测重任务不停止，所以两个变量就需要区分
				
					printf("Weight_Shiwu:%f\r\n",Weight_Send);

					Weight_Shiwu = 0;
				}
			}
		}
		EXTI_ClearITPendingBit(EXTI_Line0);    //清除LINE0上的中断标志位 
}


