#include "wave.h"

#include "led.h"
#include "key.h"
#include "delay.h"
#include "usart.h"
#include "FreeRTOS.h"
#include "task.h"

#define Trig GPIO_Pin_1

#define Echo GPIO_Pin_0

float Distance = 0;
float Distance_s = 1;

void Wave_SRD_Init(void)
{
	GPIO_InitTypeDef  GPIO_InitSture;
	EXTI_InitTypeDef  EXTI_InitSture;
	NVIC_InitTypeDef  NVIC_InitSture;
	//如果外部中断的话则一定使能AFIO复用功能
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO|RCC_APB2Periph_GPIOB,ENABLE);
	
	
	//配置IO端口
	GPIO_InitSture.GPIO_Mode=GPIO_Mode_Out_PP;   //推挽输出模式
	GPIO_InitSture.GPIO_Pin=Trig;                //将PB1于Trig相连
	GPIO_InitSture.GPIO_Speed=GPIO_Speed_50MHz;  
	GPIO_Init(GPIOB,&GPIO_InitSture);
	
	GPIO_InitSture.GPIO_Mode=GPIO_Mode_IPD;      //拉输入模式
	GPIO_InitSture.GPIO_Pin=Echo;                //将PB0于Echo相连
	GPIO_InitSture.GPIO_Speed=GPIO_Speed_50MHz;  
	GPIO_Init(GPIOB,&GPIO_InitSture);
	
	//中断和0端口映射一起
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB,GPIO_PinSource0);
	
	//外部中断配置
	EXTI_InitSture.EXTI_Line=EXTI_Line2;
	EXTI_InitSture.EXTI_LineCmd=ENABLE;
	EXTI_InitSture.EXTI_Mode=EXTI_Mode_Interrupt;
	EXTI_InitSture.EXTI_Trigger=EXTI_Trigger_Rising;
	EXTI_Init(&EXTI_InitSture);
	
	
	//中断优先级管理
	NVIC_InitSture.NVIC_IRQChannel=EXTI2_IRQn;
	NVIC_InitSture.NVIC_IRQChannelCmd=ENABLE;
	NVIC_InitSture.NVIC_IRQChannelPreemptionPriority=0x06;
	NVIC_InitSture.NVIC_IRQChannelSubPriority=0x04;
	NVIC_Init(&NVIC_InitSture);	
}

void EXTI2_IRQHandler(void)
{
	delay_us(10);
	
	
	if(EXTI_GetITStatus(EXTI_Line2)!=RESET)
	{
		TIM_SetCounter(TIM3,0);
		TIM_Cmd(TIM3,ENABLE);
		
		while(GPIO_ReadInputDataBit(GPIOB,Echo));  //等待低电平
		
		TIM_Cmd(TIM3,DISABLE);
		
		Distance=TIM_GetCounter(TIM3);//*340/200.0;
		
		if(Distance > 2 )//测距和设置其实没有什么问题，就是这里的测量方式有问题
		{
			Distance_s = Distance;
			//printf("Distance:%f cm\r\n",Distance);
		}
		if(Distance <= 2)	//意味回波没有发生变化，距离不变
		{
			//printf("Distance:%f cm\r\n",Distance_s);
		}
			
		EXTI_ClearITPendingBit(EXTI_Line2);
	}
}

void Wave_SRD_Strat(void)
{
	GPIO_SetBits(GPIOB,Trig);   //将Trig设置为高电平
	delay_us(20);               //持续大于10us触发，触发超声波模块工作
	GPIO_ResetBits(GPIOB,Trig); 
	delay_us(20);
	GPIO_SetBits(GPIOB,Trig); 
}












