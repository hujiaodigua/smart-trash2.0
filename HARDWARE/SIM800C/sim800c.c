#include "sim800c.h"
#include "usart.h"		
#include "delay.h"	
#include "led.h"   	 
#include "key.h"	 	 	 	 	 
#include "lcd.h" 	  

#include <stdlib.h>
#include "string.h"    
#include "usart2.h" 
#include "jansson.h"

int gsm_disconnect_var = 0;	//gsm断开连接时此变量赋值为1

//usmart支持部分 
//将收到的AT指令应答数据返回给电脑串口
//mode:0,不清零USART2_RX_STA;
//     1,清零USART2_RX_STA;
void sim_at_response(u8 mode)
{
	if(USART2_RX_STA&0X8000)		//接收到一次数据了
	{ 
		USART2_RX_BUF[USART2_RX_STA&0X7FFF]=0;//添加结束符
		printf("%s",USART2_RX_BUF);	//发送到串口
		if(mode)USART2_RX_STA=0;
	} 
}

void sim_touchuan_recv(u8 mode)
{
	char Downlink_data[50];
	
	json_t *root_down;
	json_error_t error; 
	int value_down;
	int value_info;
	int value_check;
	
	int k = 1;
	
	if(USART2_RX_STA&0X8000)		//接收到一次数据了
	{ 
		USART2_RX_BUF[USART2_RX_STA&0X7FFF]=0;//添加结束符		
		sprintf(Downlink_data,"%s",USART2_RX_BUF); //不定长度sprintf会溢出
		if(mode)USART2_RX_STA=0;
		printf("%s\r\n",Downlink_data);	//发送到串口
		
		Downlink_data[49]='\0';
		printf("test ok\r\n");
		if(Downlink_data[0] == 0x7B && Downlink_data[1] == 0x22)
		{	
				printf("recv ok\r\n");
				root_down = json_loads(Downlink_data, JSON_DECODE_ANY, &error);
			
				if(!json_is_object(root_down)){								//判断是否为json对象可以在sim800c透传模式下判断是否与服务器断开连接
					LED0 = 1; 	//一旦发生连接断开的情况，要让电机停转，防止因为sim800c通信进程的死锁使中断失效，电机失控
				  LED1 = 1;		//所以sim800c一旦与服务器断开之后就要尝试重新连接服务器，这是现在遗留的一个bug，一定要解决
					gsm_disconnect_var = 1;
					printf("error: root_down is not json object\r\n");
					return ;
				}else {
					printf("root_down is a json object\r\n");
				}
				
				if(json_unpack(root_down,"{s:i, s:i, s:i}","downv", &value_down ,"info", &value_info, "check", &value_check) == 0) 														//Returns 0 on success and -1 on failure.
				{
					printf("unpack ok\r\n");
					printf("value_down:%d\r\n", value_down);
					printf("value_info:%d\r\n", value_info);
					printf("value_check:%d\r\n", value_check);
					
				}
				json_object_clear(root_down);
				json_decref(root_down); // root_down 刚被创建的时候引用为1
				root_down = NULL;				//此处使用free()函数将与微库造成冲突，引起程序卡死
		}
	}	
}

//NTP网络同步时间
void ntp_update(void)
{  
	 sim800c_send_cmd("AT+SAPBR=3,1,\"Contype\",\"GPRS\"","OK",200);//配置承载场景1
	 sim800c_send_cmd("AT+SAPBR=3,1,\"APN\",\"CMNET\"","OK",200);
	 sim800c_send_cmd("AT+SAPBR=1,1",0,200);                        //激活一个GPRS上下文
   delay_ms(5);
   sim800c_send_cmd("AT+CNTPCID=1","OK",200);                     //设置CNTP使用的CID
	 sim800c_send_cmd("AT+CNTP=\"202.120.2.101\",32","OK",200);     //设置NTP服务器和本地时区(32时区 时间最准确)
   sim800c_send_cmd("AT+CNTP","+CNTP: 1",600);                    //同步网络时间
}

u8 Scan_Wtime = 0;//保存扫描需要的时间

//sim800C发送命令后,检测接收到的应答
//str:期待的应答结果
//返回值:0,没有得到期待的应答结果
//其他,期待应答结果的位置(str的位置)
u8* sim800c_check_cmd(u8 *str)
{
	char *strx=0;
	if(USART2_RX_STA&0X8000)		//接收到一次数据了
	{ 
		USART2_RX_BUF[USART2_RX_STA&0X7FFF]=0;//添加结束符
		strx=strstr((const char*)USART2_RX_BUF,(const char*)str);
	} 
	return (u8*)strx;
}

//向sim800C发送命令
//cmd:发送的命令字符串(不需要添加回车了),当cmd<0XFF的时候,发送数字(比如发送0X1A),大于的时候发送字符串.
//ack:期待的应答结果,如果为空,则表示不需要等待应答
//waittime:等待时间(单位:10ms)
//返回值:0,发送成功(得到了期待的应答结果)
//       1,发送失败
u8 sim800c_send_cmd(u8 *cmd,u8 *ack,u16 waittime)
{
	u8 res=0; 
	USART2_RX_STA=0;
	if((u32)cmd<=0XFF)
	{
		while(DMA1_Channel7->CNDTR!=0);	//等待通道7传输完成   
		USART2->DR=(u32)cmd;
	}else u2_printf("%s\r\n",cmd);		//发送命令
	
	if(ack&&waittime)			//需要等待应答
	{ 
	   while(--waittime)	//等待倒计时
	   {
		   delay_ms(10);
		   if(USART2_RX_STA&0X8000)//接收到期待的应答结果
		   {
			   if(sim800c_check_cmd(ack))break;//得到有效数据 
			   USART2_RX_STA=0;
		   } 
	   }
	   if(waittime==0)res=1; 
	}
	return res;
} 

//GSM信息查询(信号质量,电池电量,日期时间)
//返回值:1,正常
//其他,错误代码
u8 sim800c_gsminfo_show(void)
{
	u8 *p,*p1,*p2;
	u8 res = 1;			//返回值
	u8 mes[50];
	
	p=mes;
	
	USART2_RX_STA=0;
	if(sim800c_send_cmd("AT+CPIN?","OK",200));			//查询SIM卡是否在位 
	USART2_RX_STA=0;
	if(sim800c_send_cmd("AT+COPS?","OK",200)==0)		//查询运营商名字，注意：这里返回值是0代表发送成功
	{
		p1=(u8*)strstr((const char*)(USART2_RX_BUF),"\"");
		if(p1)//有有效数据
		{
			p2=(u8*)strstr((const char*)(p1+1),"\"");
			p2[0]=0;//加入结束符			
			sprintf((char*)p,"运营商:%s",p1+1);
			printf("%s\r\n",p);
		}
		USART2_RX_STA=0;
	}else {res = 0;}
	
	if(sim800c_send_cmd("AT+CSQ","+CSQ:",200)==0)		//查询信号质量
	{
		p1=(u8*)strstr((const char*)(USART2_RX_BUF),":");
		p2=(u8*)strstr((const char*)(p1),",");
		p2[0]=0;//加入结束符
		sprintf((char*)p,"信号质量:%s",p1+2);
		printf("%s\r\n",p);
		USART2_RX_STA=0;
	}else {res = 0;}
//	if(sim800c_send_cmd("AT+CBC","+CBC:",200)==0);		//查询电池电量
//	if(sim800c_send_cmd("AT+CCLK?","+CCLK:",200)==0);		//查询电池电量
	
	return res;
} 

//GSM设置GPRS透传
//返回值:1,正常
//其他,错误代码
u8 sim800c_gsm_config(void)
{
//	u8 *p,*p1,*p2;
	u8 res = 1;			//返回值
//	u8 mes[50];
	
//	p=mes;
	
	USART2_RX_STA=0;
	if(sim800c_send_cmd("+++","OK",200));	//+++清空缓存，如果得到返回值0：意味发送+++返回ok是退出透传模式
	if(sim800c_send_cmd("+++","OK",200));	//+++清空缓存，如果得到返回值1：意味发送+++没有返回是清空sim卡缓存
	USART2_RX_STA=0;
	if(sim800c_send_cmd("AT+CGATT=1","OK",200)==0)		//将GPRS设置为附着
	{
		printf("GPRS附着\r\n");
	}else {res = 0;}
	
	if(sim800c_send_cmd("AT+CIPSHUT","OK",200)==0)		//关闭之前所有的连接
	{
		printf("关闭之前所有的连接\r\n");
	}else {res = 0;}
	
	if(sim800c_send_cmd("AT+CIPMODE=1","OK",200)==0)		//让模块进入自动透传模式
	{
		printf("进入自动透传模式\r\n");
	}else {res = 0;}
	
	if(sim800c_send_cmd("AT+CLPORT=\"TCP\",\"2000\"","OK",200)==0)		//设置本地TCP端口为2000
	{
		printf("设置本地端口为2000\r\n");
	}else {res = 0;}
	
	if(sim800c_send_cmd("AT+CIPSTART=\"TCP\",\"120.78.144.55\",\"20006\"","OK",200)==0)		//连接服务器，sim800c使用的是串口2
	{																																										 	//GPRS设置为透传模式后，所有的数据将通过串口2直接发送到服务器
		printf("连接服务器\r\n");																														//除非发送+++返回ok则退出透传模式
	}else {res = 0;}	
//	if(sim800c_send_cmd("AT+CBC","+CBC:",200)==0);		//查询电池电量
//	if(sim800c_send_cmd("AT+CCLK?","+CCLK:",200)==0);		//查询电池电量
	
	return res;
} 
