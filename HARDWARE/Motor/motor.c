#include "motor.h"
#include "led.h"
#include "key.h"
#include "delay.h"
/**********************************************
ע:�ж��Ƿ�������д��һ����ѯ����
�������ؿ��Ƶ����ʹ���ⲿ�жϣ���exti.c��д��
**********************************************/
int JudegTrash(float m)
{ 
		//int k;
	
		if(m > 100)    //����100g��Ϊ������
		{
			LED0 = 1;								//���ٵ����ת
			LED1 = 0;
			
			return 1; 	 
		}
		return 0;
}
