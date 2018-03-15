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

//功能文件
#include "usart2.h" 
#include "hx711.h" //称重
#include "motor.h"
#include "sim800c.h"
#include "jansson.h"	//使用该库时Options for target 中在target下勾选Use MicroLIB
#include "wave.h"

/************************************************
 ALIENTEK Mini STM32F103开发板 FreeRTOS实验4-3
 FreeRTOS任务挂起和恢复实验-库函数版本
 技术支持：www.openedv.com
 淘宝店铺：http://eboard.taobao.com 
 关注微信公众平台微信号："正点原子"，免费获取STM32资料。
 广州市星翼电子科技有限公司  
 作者：正点原子 @ALIENTEK
************************************************/

//任务优先级
#define START_TASK_PRIO		1
//任务堆栈大小	
#define START_STK_SIZE 		128  
//任务句柄
TaskHandle_t StartTask_Handler;
//任务函数
void start_task(void *pvParameters);

//任务优先级
#define KEY_TASK_PRIO		2
//任务堆栈大小	
#define KEY_STK_SIZE 		128  
//任务句柄
TaskHandle_t KeyTask_Handler;
//任务函数
void key_task(void *pvParameters);

//任务优先级
#define TASK1_TASK_PRIO		4
//任务堆栈大小	
#define TASK1_STK_SIZE 		128  
//任务句柄
TaskHandle_t Task1Task_Handler;
//任务函数
void task1_task(void *pvParameters);

//任务优先级
#define TASK2_TASK_PRIO		3
//任务堆栈大小	
#define TASK2_STK_SIZE 		256
//任务句柄
TaskHandle_t Task2Task_Handler;
//任务函数
void task2_task(void *pvParameters);


#define GapValue 104.8
#define ABS(v) (v > 0 ? v : -v)

u8 send_enable = 0;	//该值为0时，意味sim800c没有连接上服务器，不能在有垃圾时发送垃圾重量到服务器

u8 send_trash_enable = 0; 

float Weight_Shiwu = 0;
float Weight_Send = 0;

extern u8 key_wakeup;

////LCD刷屏时使用的颜色
//int lcd_discolor[14]={	WHITE, BLACK, BLUE,  BRED,      
//						GRED,  GBLUE, RED,   MAGENTA,       	 
//						GREEN, CYAN,  YELLOW,BROWN, 			
//						BRRED, GRAY };

						
int main(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);//设置系统中断优先级分组4	 
	delay_init();	    				//延时函数初始化	 
	uart_init(115200);					//初始化串口
	LED_Init();		  					//初始化LED
	KEY_Init();							//初始化按键
	EXTIX_Init();						//初始化外部中断
//	LCD_Init();							//初始化LCD
	
	USART2_Init(115200);	         //初始化串口2--串口2连接STM800C
	
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
	
	//创建开始任务
    xTaskCreate((TaskFunction_t )start_task,            //任务函数
                (const char*    )"start_task",          //任务名称
                (uint16_t       )START_STK_SIZE,        //任务堆栈大小
                (void*          )NULL,                  //传递给任务函数的参数
                (UBaseType_t    )START_TASK_PRIO,       //任务优先级
                (TaskHandle_t*  )&StartTask_Handler);   //任务句柄              
    vTaskStartScheduler();          //开启任务调度
}

//开始任务任务函数
void start_task(void *pvParameters)
{
    taskENTER_CRITICAL();           //进入临界区
	//创建KEY任务
	xTaskCreate((TaskFunction_t )key_task,             
                (const char*    )"key_task",           
                (uint16_t       )KEY_STK_SIZE,        
                (void*          )NULL,                  
                (UBaseType_t    )KEY_TASK_PRIO,        
                (TaskHandle_t*  )&KeyTask_Handler);  
    //创建TASK1任务
    xTaskCreate((TaskFunction_t )task1_task,             
                (const char*    )"task1_task",           
                (uint16_t       )TASK1_STK_SIZE,        
                (void*          )NULL,                  
                (UBaseType_t    )TASK1_TASK_PRIO,        
                (TaskHandle_t*  )&Task1Task_Handler);   
    //创建TASK2任务
    xTaskCreate((TaskFunction_t )task2_task,     
                (const char*    )"task2_task",   
                (uint16_t       )TASK2_STK_SIZE,
                (void*          )NULL,
                (UBaseType_t    )TASK2_TASK_PRIO,
                (TaskHandle_t*  )&Task2Task_Handler); 
    vTaskDelete(StartTask_Handler); //删除开始任务
    taskEXIT_CRITICAL();            //退出临界区
}

//key任务函数--这个任务函数留给SIM800C模块进行GPRS通信使用
void key_task(void *pvParameters)
{
	//u8 key,statflag=0;
	u8 task3_num=0;
	u8 key=0; 							//这个变量是做什么用的？
	
//	u8 timex=0;
	u8 sim_ready=0;
	
	json_t *root;
  char *out;
	
	json_t *root_heartbeat;
  char *out_heartbeat;

	
	//json
//	char* str = "{\"downv\": 1,\"info\": 1,\"xx\": 12}";

	
	while(sim800c_send_cmd("AT","OK",100))//检测是否应答AT指令 
	{
		printf("AT Falied\r\n");
	} 
	key+=sim800c_send_cmd("ATE0","OK",200);//不回显
	ntp_update();//网络同步时间
	
	if(sim800c_gsminfo_show()) 														//返回值1意味检查运营商和信号强度成功，因为设置GPRS透传后
	{																											//经过串口2的任何数据都被当作GPRS数据发送给远端服务器，除非退出透传模式，所以把这个检测暂时放在这里，本来是应该放在循环中多次检测的
		sim_ready = 1;					//意味着sim卡可以发送数据了
	}else sim_ready = 0;			//否则sim卡不能发送数据
	
	if(sim800c_gsm_config())  //返回值1意味设置GPRS透传成功
	{
		printf("sim800c set ok\r\n");
		send_enable = 1;				//发送标记变量置1，允许有垃圾时发送垃圾重量到服务器
	}
	
	root_heartbeat = json_pack("{sisisisisisisi}", 	//startup_stm32f10x_hd.s中默认的Heap_Size = 0x200 只有512B，改为0xc00 即3KB即可
																 "upv",    	1,
																 "e_type", 	0,
																 "uuid",   	12,
																 "major", 	12,
																 "d_check", 32,
																 "info",	 	2,  //测到没有垃圾时发心跳包
																 "beat",	 	0 );
	out_heartbeat = json_dumps(root_heartbeat, JSON_ENCODE_ANY);
	
	root = json_pack("{sisisisisisisi}", 	//startup_stm32f10x_hd.s中默认的Heap_Size = 0x200 只有512B，改为0xc00 即3KB即可
																 "upv",    1,
																 "e_type",   0,
																 "uuid",   12,
																 "major", 12,
																 "d_check", 32,
																 "info",	 1,		//测到有垃圾发数据包
																 "weight",json_integer(Weight_Send)  );
	
	while(1)
	{
		task3_num++;	//任务1执行次数加1 注意task3_num3加到255的时候会清零
		printf("任务3已经执行：%d次\r\n",task3_num);
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
			
//		timex = sim800c_send_cmd("ABCDEFG","ABCDEFG",100);    //发送ABCDEFG给服务器测试通信
		
/*		sim_at_response(1);//检查GSM模块发送过来的数据,及时上传给电脑
		
		vTaskDelay(10);
		if(sim800c_gsminfo_show()) //返回值1意味检查成功
		{
			sim_ready = 1;					//意味着sim卡可以发送数据了
		}else sim_ready = 0;			//否则sim卡不能发送数据

		if(sim_ready)//SIM卡就绪.
		{
			
		}
*/

		
//		if(timex==0)		//2.5秒左右更新一次
//			if(sim800c_gsminfo_show(40,225)==0)sim_ready=1;
//			else sim_ready=0;	
		
//		key=KEY_Scan(0);
//		switch(key)
//		{
//			case WKUP_PRES:
//				statflag=!statflag;
//				if(statflag==1)
//				{
//					vTaskSuspend(Task1Task_Handler);//挂起任务
//					printf("挂起任务1的运行!\r\n");
//				}
//				else if(statflag==0)
//				{
//					vTaskResume(Task1Task_Handler);	//恢复任务1
//					printf("恢复任务1的运行!\r\n");
//				}		
//				break;
////			case KEY1_PRES:
////				vTaskSuspend(Task2Task_Handler);//挂起任务2
////				printf("挂起任务2的运行!\r\n");
////				break;
//		}
//		vTaskDelay(10);			//延时10ms 
	}
}

//task1任务函数--判断是否有垃圾
//判断是否有垃圾考虑设置成手动的，称重检测或者超声波检测其实都不如手动的方便
void task1_task(void *pvParameters)
{
	u8 task1_num=0;

//	u8 Weight_Trash[4]; //传感器量程只有20kg=20000g，16位已经够用，这里预留32位空间
	
	//json 定义
//	json_t *root;
//  char *Uplink_data; 
	
//	POINT_COLOR = BLACK;
//	
//	LCD_DrawRectangle(5,110,115,314); 	//画一个矩形	
//	LCD_DrawLine(5,130,115,130);		//画线
//	POINT_COLOR = BLUE;
//	LCD_ShowString(6,111,110,16,16,"Task1 Run:000");

	while(1)
	{	
		if(Weight_Shiwu > 100)	//真的有垃圾执行处理函数
		{
			
//			//立即执行处理函数
//			if(key_wakeup == 1) 	//WKUP按键被按下意味投放者确认自己垃圾投放完毕 查询响应速度太慢绝对不行，必须写成全局中断
//			{
//				key_wakeup = 0;
//				//vTaskDelay(1000);//等1秒确认放上的是垃圾 
//				if(JudegTrash(Weight_Shiwu))
//				{
//					send_trash_enable = 1; //允许key里发送重量数据
//					Weight_Send = Weight_Shiwu; //有垃圾了，就把垃圾的重量保存起来,因为测重任务不停止，所以两个变量就需要区分
//	//			sprintf((char*)Weight_Trash,"%f",Weight_Shiwu);
//	//			sim800c_send_cmd(Weight_Trash,Weight_Trash,100);

//	// 关于发送json的如下代码，当执行json_dumps函数后，将引起某个机智然后挂起所有任务，无论json_dumps函数放在哪里，只要被执行
//	// 但是放到优先级为1的key任务里却可以执行			
//	//			root = json_pack("{sisisisisisisi}", 	//startup_stm32f10x_hd.s中默认的Heap_Size = 0x200 只有512B，改为0xc00 即3KB即可
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
//					printf("挂起任务1的运行!\r\n");
//					printf("Weight_Shiwu:%f\r\n",Weight_Send);
//					vTaskSuspend(Task1Task_Handler);//挂起任务1，从而暂停检测垃圾，当电机逻辑执行完毕后，在那里恢复任务1
//					Weight_Shiwu = 0;
//				}
//			}
			if(key_wakeup == 0) //WKUP按键没有被按下意味投放者忘记确认自己垃圾投放完毕
			{
				vTaskDelay(10000);//等10s执行处理函数
					vTaskDelay(1000);//等1秒确认放上的是垃圾 
					if(JudegTrash(Weight_Shiwu))
					{
						send_trash_enable = 1; //允许key里发送重量数据
						Weight_Send = Weight_Shiwu; //有垃圾了，就把垃圾的重量保存起来,因为测重任务不停止，所以两个变量就需要区分
						printf("挂起任务1的运行!\r\n");
						printf("Weight_Shiwu:%f\r\n",Weight_Send);
						vTaskSuspend(Task1Task_Handler);//挂起任务1，从而暂停检测垃圾，当电机逻辑执行完毕后，在那里恢复任务1
						Weight_Shiwu = 0;
					}
			}
		}
		
//		else
//		{
//			vTaskResume(Task2Task_Handler);	//恢复任务1
//			printf("挂起任务2的运行!\r\n");
//		}
		
		
		
		
		task1_num++;	//任务执1行次数加1 注意task1_num1加到255的时候会清零！！
//		LED0=!LED0;
//		printf("任务1已经执行：%d次\r\n",task1_num);
//		LCD_Fill(6,131,114,313,lcd_discolor[task1_num%14]); //填充区域
//		LCD_ShowxNum(86,111,task1_num,3,16,0x80);	//显示任务执行次数
        vTaskDelay(10);                           //延时0.01秒
	}
}

//task2任务函数--测重
void task2_task(void *pvParameters)
{
	u8 task2_num=0;
	
	u8 i = 0;
//	u8 k = 0;
	
	float ad_val0 = 0;	//空载净重 
	float ad_val1 = 0; 	//测量重量
	
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

//	LCD_DrawRectangle(125,110,234,314); //画一个矩形	
//	LCD_DrawLine(125,130,234,130);		//画线
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
//			json_decref(root_down); // root_down 刚被创建的时候引用为1
//			free(root_down);
//			root_down = NULL;		
//		}
		
//		if(json_object_clear(root_down) == 0)
//		{printf("clear json_object");free(root_down);}
		//接收服务器返回值
		if(send_enable == 1)
		{
			sim_touchuan_recv(1);
		}
		ad_val1 = HX711_Read() + HX711_Read_Left() + HX711_Read_Right();
		
//		if(ABS(ad_val1-ad_val0) < 800) //自动标定0
//		{
//			ad_val0 = ad_val1;
//		}
		Weight_Shiwu = (ad_val1-ad_val0)/GapValue;
//		printf("Weight_Shiwu: %f\r",Weight_Shiwu);
		
		
		
		task2_num++;	//任务2执行次数加1 注意task1_num2加到255的时候会清零！！
//        LED1=!LED1;
//		printf("任务2已经执行：%d次\r\n",task2_num);
//		LCD_ShowxNum(206,111,task2_num,3,16,0x80);  //显示任务执行次数
//		LCD_Fill(126,131,233,313,lcd_discolor[13-task2_num%14]); //填充区域
        vTaskDelay(100);                           //延时0.1s
	}
}

