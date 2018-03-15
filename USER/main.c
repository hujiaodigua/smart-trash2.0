#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "timer.h"
#include "lcd.h"
#include "key.h"
#include "exti.h"
#include "FreeRTOS.h"
#include "task.h"

//�����ļ�
#include "usart2.h" 
#include "hx711.h" //����
#include "motor.h"
#include "sim800c.h"
#include "jansson.h"	//ʹ�øÿ�ʱOptions for target ����target�¹�ѡUse MicroLIB
#include "wave.h"

/************************************************
 ALIENTEK Mini STM32F103������ FreeRTOSʵ��4-3
 FreeRTOS�������ͻָ�ʵ��-�⺯���汾
 ����֧�֣�www.openedv.com
 �Ա����̣�http://eboard.taobao.com 
 ��ע΢�Ź���ƽ̨΢�źţ�"����ԭ��"����ѻ�ȡSTM32���ϡ�
 ������������ӿƼ����޹�˾  
 ���ߣ�����ԭ�� @ALIENTEK
************************************************/

//�������ȼ�
#define START_TASK_PRIO		1
//�����ջ��С	
#define START_STK_SIZE 		128  
//������
TaskHandle_t StartTask_Handler;
//������
void start_task(void *pvParameters);

//�������ȼ�
#define KEY_TASK_PRIO		2
//�����ջ��С	
#define KEY_STK_SIZE 		128  
//������
TaskHandle_t KeyTask_Handler;
//������
void key_task(void *pvParameters);

//�������ȼ�
#define TASK1_TASK_PRIO		4
//�����ջ��С	
#define TASK1_STK_SIZE 		128  
//������
TaskHandle_t Task1Task_Handler;
//������
void task1_task(void *pvParameters);

//�������ȼ�
#define TASK2_TASK_PRIO		3
//�����ջ��С	
#define TASK2_STK_SIZE 		256
//������
TaskHandle_t Task2Task_Handler;
//������
void task2_task(void *pvParameters);


#define GapValue 104.8
#define ABS(v) (v > 0 ? v : -v)

u8 send_enable = 0;	//��ֵΪ0ʱ����ζsim800cû�������Ϸ�������������������ʱ��������������������

u8 send_trash_enable = 0; 

float Weight_Shiwu = 0;
float Weight_Send = 0;

extern u8 key_wakeup;

////LCDˢ��ʱʹ�õ���ɫ
//int lcd_discolor[14]={	WHITE, BLACK, BLUE,  BRED,      
//						GRED,  GBLUE, RED,   MAGENTA,       	 
//						GREEN, CYAN,  YELLOW,BROWN, 			
//						BRRED, GRAY };

						
int main(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);//����ϵͳ�ж����ȼ�����4	 
	delay_init();	    				//��ʱ������ʼ��	 
	uart_init(115200);					//��ʼ������
	LED_Init();		  					//��ʼ��LED
	KEY_Init();							//��ʼ������
	EXTIX_Init();						//��ʼ���ⲿ�ж�
//	LCD_Init();							//��ʼ��LCD
	
	USART2_Init(115200);	         //��ʼ������2--����2����STM800C
	
	ADInit();
	ADInit_Left();
	ADInit_Right();
	
	Timer_SRD_Init(5000,7199);
	Wave_SRD_Init();
	
//    POINT_COLOR = RED;
//	LCD_ShowString(30,10,200,16,16,"ATK STM32F103/407");	
//	LCD_ShowString(30,30,200,16,16,"FreeRTOS Examp 6-3");
//	LCD_ShowString(30,50,200,16,16,"Task Susp and Resum");
//	LCD_ShowString(30,70,200,16,16,"ATOM@ALIENTEK");
//	LCD_ShowString(30,90,200,16,16,"2016/11/25");
	
	//������ʼ����
    xTaskCreate((TaskFunction_t )start_task,            //������
                (const char*    )"start_task",          //��������
                (uint16_t       )START_STK_SIZE,        //�����ջ��С
                (void*          )NULL,                  //���ݸ��������Ĳ���
                (UBaseType_t    )START_TASK_PRIO,       //�������ȼ�
                (TaskHandle_t*  )&StartTask_Handler);   //������              
    vTaskStartScheduler();          //�����������
}

//��ʼ����������
void start_task(void *pvParameters)
{
    taskENTER_CRITICAL();           //�����ٽ���
	//����KEY����
	xTaskCreate((TaskFunction_t )key_task,             
                (const char*    )"key_task",           
                (uint16_t       )KEY_STK_SIZE,        
                (void*          )NULL,                  
                (UBaseType_t    )KEY_TASK_PRIO,        
                (TaskHandle_t*  )&KeyTask_Handler);  
    //����TASK1����
    xTaskCreate((TaskFunction_t )task1_task,             
                (const char*    )"task1_task",           
                (uint16_t       )TASK1_STK_SIZE,        
                (void*          )NULL,                  
                (UBaseType_t    )TASK1_TASK_PRIO,        
                (TaskHandle_t*  )&Task1Task_Handler);   
    //����TASK2����
    xTaskCreate((TaskFunction_t )task2_task,     
                (const char*    )"task2_task",   
                (uint16_t       )TASK2_STK_SIZE,
                (void*          )NULL,
                (UBaseType_t    )TASK2_TASK_PRIO,
                (TaskHandle_t*  )&Task2Task_Handler); 
    vTaskDelete(StartTask_Handler); //ɾ����ʼ����
    taskEXIT_CRITICAL();            //�˳��ٽ���
}

//key������--�������������SIM800Cģ�����GPRSͨ��ʹ��
void key_task(void *pvParameters)
{
	//u8 key,statflag=0;
	u8 task3_num=0;
	u8 key=0; 							//�����������ʲô�õģ�
	
//	u8 timex=0;
	u8 sim_ready=0;
	
	json_t *root;
  char *out;
	
	json_t *root_heartbeat;
  char *out_heartbeat;

	
	//json
//	char* str = "{\"downv\": 1,\"info\": 1,\"xx\": 12}";

	
	while(sim800c_send_cmd("AT","OK",100))//����Ƿ�Ӧ��ATָ�� 
	{
		printf("AT Falied\r\n");
	} 
	key+=sim800c_send_cmd("ATE0","OK",200);//������
	ntp_update();//����ͬ��ʱ��
	
	if(sim800c_gsminfo_show()) 														//����ֵ1��ζ�����Ӫ�̺��ź�ǿ�ȳɹ�����Ϊ����GPRS͸����
	{																											//��������2���κ����ݶ�������GPRS���ݷ��͸�Զ�˷������������˳�͸��ģʽ�����԰���������ʱ�������������Ӧ�÷���ѭ���ж�μ���
		sim_ready = 1;					//��ζ��sim�����Է���������
	}else sim_ready = 0;			//����sim�����ܷ�������
	
	if(sim800c_gsm_config())  //����ֵ1��ζ����GPRS͸���ɹ�
	{
		printf("sim800c set ok\r\n");
		send_enable = 1;				//���ͱ�Ǳ�����1������������ʱ��������������������
	}
	
	root_heartbeat = json_pack("{sisisisisisisi}", 	//startup_stm32f10x_hd.s��Ĭ�ϵ�Heap_Size = 0x200 ֻ��512B����Ϊ0xc00 ��3KB����
																 "upv",    	1,
																 "e_type", 	0,
																 "uuid",   	12,
																 "major", 	12,
																 "d_check", 32,
																 "info",	 	2,  //�⵽û������ʱ��������
																 "beat",	 	0 );
	out_heartbeat = json_dumps(root_heartbeat, JSON_ENCODE_ANY);
	
	root = json_pack("{sisisisisisisi}", 	//startup_stm32f10x_hd.s��Ĭ�ϵ�Heap_Size = 0x200 ֻ��512B����Ϊ0xc00 ��3KB����
																 "upv",    1,
																 "e_type",   0,
																 "uuid",   12,
																 "major", 12,
																 "d_check", 32,
																 "info",	 1,		//�⵽�����������ݰ�
																 "weight",json_integer(Weight_Send)  );
	
	while(1)
	{
		task3_num++;	//����1ִ�д�����1 ע��task3_num3�ӵ�255��ʱ�������
		printf("����3�Ѿ�ִ�У�%d��\r\n",task3_num);
		vTaskDelay(2000); 
		if(send_trash_enable)
		{
			json_object_set(root, "weight", json_integer(Weight_Send));
			out = json_dumps(root, JSON_ENCODE_ANY);
//			printf("out:%s\r\n", out);
			sim800c_send_cmd((u8*)out,(u8*)out,100);
//			free(root);
			free(out);
			send_trash_enable = 0;
		}else
	  {
			
			if(root_heartbeat == NULL)
				sim800c_send_cmd("root_heartbeat is null","",100);
			if(out_heartbeat == NULL)
				sim800c_send_cmd("out_heartbeat is null","",100);
			sim800c_send_cmd(out_heartbeat,"",100);
//			free(root);free(out);
//			sim800c_send_cmd("{\"upv\": 1,\"type\": 1,\"uuid\": 12,\"type\": 1,\"major\": 12, \"major\": 32, \"info\": 1,\"beat\": 0,}"," ",100);
    }
			
//		timex = sim800c_send_cmd("ABCDEFG","ABCDEFG",100);    //����ABCDEFG������������ͨ��
		
/*		sim_at_response(1);//���GSMģ�鷢�͹���������,��ʱ�ϴ�������
		
		vTaskDelay(10);
		if(sim800c_gsminfo_show()) //����ֵ1��ζ���ɹ�
		{
			sim_ready = 1;					//��ζ��sim�����Է���������
		}else sim_ready = 0;			//����sim�����ܷ�������

		if(sim_ready)//SIM������.
		{
			
		}
*/

		
//		if(timex==0)		//2.5�����Ҹ���һ��
//			if(sim800c_gsminfo_show(40,225)==0)sim_ready=1;
//			else sim_ready=0;	
		
//		key=KEY_Scan(0);
//		switch(key)
//		{
//			case WKUP_PRES:
//				statflag=!statflag;
//				if(statflag==1)
//				{
//					vTaskSuspend(Task1Task_Handler);//��������
//					printf("��������1������!\r\n");
//				}
//				else if(statflag==0)
//				{
//					vTaskResume(Task1Task_Handler);	//�ָ�����1
//					printf("�ָ�����1������!\r\n");
//				}		
//				break;
////			case KEY1_PRES:
////				vTaskSuspend(Task2Task_Handler);//��������2
////				printf("��������2������!\r\n");
////				break;
//		}
//		vTaskDelay(10);			//��ʱ10ms 
	}
}

//task1������--�ж��Ƿ�������
//�ж��Ƿ��������������ó��ֶ��ģ����ؼ����߳����������ʵ�������ֶ��ķ���
void task1_task(void *pvParameters)
{
	u8 task1_num=0;

//	u8 Weight_Trash[4]; //����������ֻ��20kg=20000g��16λ�Ѿ����ã�����Ԥ��32λ�ռ�
	
	//json ����
//	json_t *root;
//  char *Uplink_data; 
	
//	POINT_COLOR = BLACK;
//	
//	LCD_DrawRectangle(5,110,115,314); 	//��һ������	
//	LCD_DrawLine(5,130,115,130);		//����
//	POINT_COLOR = BLUE;
//	LCD_ShowString(6,111,110,16,16,"Task1 Run:000");

	while(1)
	{	
		if(Weight_Shiwu > 100)	//���������ִ�д�����
		{
			
//			//����ִ�д�����
//			if(key_wakeup == 1) 	//WKUP������������ζͶ����ȷ���Լ�����Ͷ����� ��ѯ��Ӧ�ٶ�̫�����Բ��У�����д��ȫ���ж�
//			{
//				key_wakeup = 0;
//				//vTaskDelay(1000);//��1��ȷ�Ϸ��ϵ������� 
//				if(JudegTrash(Weight_Shiwu))
//				{
//					send_trash_enable = 1; //����key�﷢����������
//					Weight_Send = Weight_Shiwu; //�������ˣ��Ͱ�������������������,��Ϊ��������ֹͣ������������������Ҫ����
//	//			sprintf((char*)Weight_Trash,"%f",Weight_Shiwu);
//	//			sim800c_send_cmd(Weight_Trash,Weight_Trash,100);

//	// ���ڷ���json�����´��룬��ִ��json_dumps�����󣬽�����ĳ������Ȼ�����������������json_dumps�����������ֻҪ��ִ��
//	// ���Ƿŵ����ȼ�Ϊ1��key������ȴ����ִ��			
//	//			root = json_pack("{sisisisisisisi}", 	//startup_stm32f10x_hd.s��Ĭ�ϵ�Heap_Size = 0x200 ֻ��512B����Ϊ0xc00 ��3KB����
//	//																		 "upv",    1,
//	//																		 "type",   0,
//	//																		 "uuid",   12,
//	//																		 "major", 12,
//	//																		 "check", 32,
//	//																		 "info",	 1,
//	//																		 "xx",		 (uint32_t)Weight_Shiwu  );
//	//			Uplink_data = json_dumps(root, JSON_ENCODE_ANY);
//	//			printf("out:%s\r\n", Uplink_data);
//	//			free(root);free(Uplink_data);
//				
//					printf("��������1������!\r\n");
//					printf("Weight_Shiwu:%f\r\n",Weight_Send);
//					vTaskSuspend(Task1Task_Handler);//��������1���Ӷ���ͣ���������������߼�ִ����Ϻ�������ָ�����1
//					Weight_Shiwu = 0;
//				}
//			}
			if(key_wakeup == 0) //WKUP����û�б�������ζͶ��������ȷ���Լ�����Ͷ�����
			{
				vTaskDelay(10000);//��10sִ�д�����
					vTaskDelay(1000);//��1��ȷ�Ϸ��ϵ������� 
					if(JudegTrash(Weight_Shiwu))
					{
						send_trash_enable = 1; //����key�﷢����������
						Weight_Send = Weight_Shiwu; //�������ˣ��Ͱ�������������������,��Ϊ��������ֹͣ������������������Ҫ����
						printf("��������1������!\r\n");
						printf("Weight_Shiwu:%f\r\n",Weight_Send);
						vTaskSuspend(Task1Task_Handler);//��������1���Ӷ���ͣ���������������߼�ִ����Ϻ�������ָ�����1
						Weight_Shiwu = 0;
					}
			}
		}
		
//		else
//		{
//			vTaskResume(Task2Task_Handler);	//�ָ�����1
//			printf("��������2������!\r\n");
//		}
		
		
		
		
		task1_num++;	//����ִ1�д�����1 ע��task1_num1�ӵ�255��ʱ������㣡��
//		LED0=!LED0;
//		printf("����1�Ѿ�ִ�У�%d��\r\n",task1_num);
//		LCD_Fill(6,131,114,313,lcd_discolor[task1_num%14]); //�������
//		LCD_ShowxNum(86,111,task1_num,3,16,0x80);	//��ʾ����ִ�д���
        vTaskDelay(10);                           //��ʱ0.01��
	}
}

//task2������--����
void task2_task(void *pvParameters)
{
	u8 task2_num=0;
	
	u8 i = 0;
//	u8 k = 0;
	
	float ad_val0 = 0;	//���ؾ��� 
	float ad_val1 = 0; 	//��������
	
//	char *Downlink_data = "{\"downv\": 1,\"info\": 1,\"check\": 12}"; 
//	json_t *root_down;
//	json_error_t error; 
//	int value_down;
//	int value_info;
//	int value_check;
	
	
	for(i=0;i<10;i++)
	{
		
		ad_val0 = HX711_Read() + HX711_Read_Left() + HX711_Read_Right();
		delay_ms(5);
	}
		

//	POINT_COLOR = BLACK;

//	LCD_DrawRectangle(125,110,234,314); //��һ������	
//	LCD_DrawLine(125,130,234,130);		//����
//	POINT_COLOR = BLUE;
//	LCD_ShowString(126,111,110,16,16,"Task2 Run:000");
		
	while(1)
	{
			Wave_SRD_Strat();
//		root_down = json_loads(Downlink_data, JSON_DECODE_ANY, &error); 
//		if(json_unpack(root_down,"{s:i, s:i, s:i}","downv", &value_down ,"info", &value_info, "check", &value_check) == 0) 														//Returns 0 on success and -1 on failure.
//		{
//			printf("value_down:%d\r\n", value_down);
//			printf("value_info:%d\r\n", value_info);
//			printf("value_check:%d\r\n", value_check);
//		}
//		
//		while(k --> 0)
//		{
//			json_object_clear(root_down);
//			json_decref(root_down); // root_down �ձ�������ʱ������Ϊ1
//			free(root_down);
//			root_down = NULL;		
//		}
		
//		if(json_object_clear(root_down) == 0)
//		{printf("clear json_object");free(root_down);}
		//���շ���������ֵ
		if(send_enable == 1)
		{
			sim_touchuan_recv(1);
		}
		ad_val1 = HX711_Read() + HX711_Read_Left() + HX711_Read_Right();
		
//		if(ABS(ad_val1-ad_val0) < 800) //�Զ��궨0
//		{
//			ad_val0 = ad_val1;
//		}
		Weight_Shiwu = (ad_val1-ad_val0)/GapValue;
//		printf("Weight_Shiwu: %f\r",Weight_Shiwu);
		
		
		
		task2_num++;	//����2ִ�д�����1 ע��task1_num2�ӵ�255��ʱ������㣡��
//        LED1=!LED1;
//		printf("����2�Ѿ�ִ�У�%d��\r\n",task2_num);
//		LCD_ShowxNum(206,111,task2_num,3,16,0x80);  //��ʾ����ִ�д���
//		LCD_Fill(126,131,233,313,lcd_discolor[13-task2_num%14]); //�������
        vTaskDelay(100);                           //��ʱ0.1s
	}
}

