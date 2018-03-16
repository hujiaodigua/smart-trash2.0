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
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//Mini STM32������
//�ⲿ�ж� ��������			   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//�޸�����:2010/12/01  
//�汾��V1.0
//��Ȩ���У�����ؾ���
//Copyright(C) ����ԭ�� 2009-2019
//All rights reserved	  
////////////////////////////////////////////////////////////////////////////////// 	  
 
u8 key_wakeup = 0; 	//Ĭ��ֵ0��Ͷ�Ű�������1����Ҫ��������2
 
extern u8 send_trash_enable; 
extern float Weight_Shiwu;
extern float Weight_Send; 
 
//�ⲿ�жϳ�ʼ������
void EXTIX_Init(void)
{
 
	EXTI_InitTypeDef EXTI_InitStructure;	
	NVIC_InitTypeDef NVIC_InitStructure;

  	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);//�ⲿ�жϣ���Ҫʹ��AFIOʱ��

	KEY_Init();//��ʼ��������Ӧioģʽ

    //GPIOC.5 �ж����Լ��жϳ�ʼ������
  	GPIO_EXTILineConfig(GPIO_PortSourceGPIOC,GPIO_PinSource5);

  	EXTI_InitStructure.EXTI_Line=EXTI_Line5;
  	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;	
  	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;//�½��ش���
  	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  	EXTI_Init(&EXTI_InitStructure);	 	//����EXTI_InitStruct��ָ���Ĳ�����ʼ������EXTI�Ĵ���
 
	  //GPIOA.15	  �ж����Լ��жϳ�ʼ������
  	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA,GPIO_PinSource15);

  	EXTI_InitStructure.EXTI_Line=EXTI_Line15;
  	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;	
  	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
  	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  	EXTI_Init(&EXTI_InitStructure);	  	//����EXTI_InitStruct��ָ���Ĳ�����ʼ������EXTI�Ĵ���
	
    //GPIOA.0 �ж����Լ��жϳ�ʼ������
  	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA,GPIO_PinSource0);

  	EXTI_InitStructure.EXTI_Line=EXTI_Line0;
  	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;	
  	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;//�����ش���
  	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  	EXTI_Init(&EXTI_InitStructure);	 	//����EXTI_InitStruct��ָ���Ĳ�����ʼ������EXTI�Ĵ���

		
	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;			//ʹ�ܰ������ڵ��ⲿ�ж�ͨ��
  	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x06;//��ռ���ȼ�6 
  	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x02;		//�����ȼ�0
  	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				//ʹ���ⲿ�ж�ͨ��
  	NVIC_Init(&NVIC_InitStructure); 
		
		
  NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;			//ʹ�ܰ������ڵ��ⲿ�ж�ͨ��
  	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x06;	//��ռ���ȼ�2�� 
  	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x03;					//�����ȼ�1
  	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;								//ʹ���ⲿ�ж�ͨ��
  	NVIC_Init(&NVIC_InitStructure); 
		
  NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;			//ʹ�ܰ������ڵ��ⲿ�ж�ͨ��
  	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x06;	//��ռ���ȼ�5 
  	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x01;					//�����ȼ�1
  	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;								//ʹ���ⲿ�ж�ͨ��
  	NVIC_Init(&NVIC_InitStructure); 		
		
}


//������
extern TaskHandle_t Task1Task_Handler;

void EXTI9_5_IRQHandler(void)
{	
	//BaseType_t YieldRequired;	
	delay_xms(20);   //����			 
	if(KEY0==0)	
	{
		
			LED0 = 1; 	//���ü��ٵ��ͣת
			LED1 = 1;
			delay_xms(50);   //�����һ�������ʱ��		
			LED0 = 0; 	//���ü��ٵ����ת
			LED1 = 1;	
			
//		YieldRequired=xTaskResumeFromISR(Task2Task_Handler);//�ָ�����2
//		printf("�ָ�����2������!\r\n");
//		if(YieldRequired==pdTRUE)
//		{
//			/*�������xTaskResumeFromISR()����ֵΪpdTRUE����ô˵��Ҫ�ָ������
//			������������ȼ����ڻ��߸����������е�����(���жϴ�ϵ�����),������
//			�˳��жϵ�ʱ��һ��Ҫ�����������л���*/
//			portYIELD_FROM_ISR(YieldRequired);
//		}
	}
 	 EXTI_ClearITPendingBit(EXTI_Line5);    //���LINE5�ϵ��жϱ�־λ  
}


void EXTI15_10_IRQHandler(void)
{
	BaseType_t YieldRequired;	
  delay_xms(20);    //����			 
  if(KEY1==0)			 
	{
		LED0 = 1;			  //���ٵ��ͣת
		LED1 = 1;
		
		key_wakeup = 0;		//��һ����Ҫ���������������еĺ�����ִ��Ҳ�ǿ��Ե�
		printf("�г̿�����key_wakeupֵ��%d\r\n",key_wakeup);
		YieldRequired=xTaskResumeFromISR(Task1Task_Handler);//�ָ�����2
		printf("�ָ�����1������!\r\n");
		if(YieldRequired==pdTRUE)
		{
			portYIELD_FROM_ISR(YieldRequired);
		}
	}	
	 EXTI_ClearITPendingBit(EXTI_Line15);  //���LINE15��·����λ
}

void EXTI0_IRQHandler(void)
{
	if(Weight_Shiwu > 200)	//���������ִ�д�����
		{
			delay_xms(100);   //����
			if(WK_UP==1)			 
			{
				printf("����Ͷ�����");
				key_wakeup = 1;
				printf("Ͷ�Ű�����key_wakeupֵ��%d\r\n",key_wakeup);
			}
			//����ִ�д�����
			if(key_wakeup == 1) 	//WKUP������������ζͶ����ȷ���Լ�����Ͷ����� ��ѯ��Ӧ�ٶ�̫�����Բ��У�����д��ȫ���ж�
			{
				if(JudegTrash(Weight_Shiwu))
				{
					send_trash_enable = 1; //����key�﷢����������
					Weight_Send = Weight_Shiwu; //�������ˣ��Ͱ�������������������,��Ϊ��������ֹͣ������������������Ҫ����
				
					printf("Weight_Shiwu:%f\r\n",Weight_Send);

					Weight_Shiwu = 0;
				}
			}
		}
		EXTI_ClearITPendingBit(EXTI_Line0);    //���LINE0�ϵ��жϱ�־λ 
}


