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

int gsm_disconnect_var = 0;	//gsm�Ͽ�����ʱ�˱�����ֵΪ1

//usmart֧�ֲ��� 
//���յ���ATָ��Ӧ�����ݷ��ظ����Դ���
//mode:0,������USART2_RX_STA;
//     1,����USART2_RX_STA;
void sim_at_response(u8 mode)
{
	if(USART2_RX_STA&0X8000)		//���յ�һ��������
	{ 
		USART2_RX_BUF[USART2_RX_STA&0X7FFF]=0;//��ӽ�����
		printf("%s",USART2_RX_BUF);	//���͵�����
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
	
	if(USART2_RX_STA&0X8000)		//���յ�һ��������
	{ 
		USART2_RX_BUF[USART2_RX_STA&0X7FFF]=0;//��ӽ�����		
		sprintf(Downlink_data,"%s",USART2_RX_BUF); //��������sprintf�����
		if(mode)USART2_RX_STA=0;
		printf("%s\r\n",Downlink_data);	//���͵�����
		
		Downlink_data[49]='\0';
		printf("test ok\r\n");
		if(Downlink_data[0] == 0x7B && Downlink_data[1] == 0x22)
		{	
				printf("recv ok\r\n");
				root_down = json_loads(Downlink_data, JSON_DECODE_ANY, &error);
			
				if(!json_is_object(root_down)){								//�ж��Ƿ�Ϊjson���������sim800c͸��ģʽ���ж��Ƿ���������Ͽ�����
					LED0 = 1; 	//һ���������ӶϿ��������Ҫ�õ��ͣת����ֹ��Ϊsim800cͨ�Ž��̵�����ʹ�ж�ʧЧ�����ʧ��
				  LED1 = 1;		//����sim800cһ����������Ͽ�֮���Ҫ�����������ӷ���������������������һ��bug��һ��Ҫ���
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
				json_decref(root_down); // root_down �ձ�������ʱ������Ϊ1
				root_down = NULL;				//�˴�ʹ��free()��������΢����ɳ�ͻ�����������
		}
	}	
}

//NTP����ͬ��ʱ��
void ntp_update(void)
{  
	 sim800c_send_cmd("AT+SAPBR=3,1,\"Contype\",\"GPRS\"","OK",200);//���ó��س���1
	 sim800c_send_cmd("AT+SAPBR=3,1,\"APN\",\"CMNET\"","OK",200);
	 sim800c_send_cmd("AT+SAPBR=1,1",0,200);                        //����һ��GPRS������
   delay_ms(5);
   sim800c_send_cmd("AT+CNTPCID=1","OK",200);                     //����CNTPʹ�õ�CID
	 sim800c_send_cmd("AT+CNTP=\"202.120.2.101\",32","OK",200);     //����NTP�������ͱ���ʱ��(32ʱ�� ʱ����׼ȷ)
   sim800c_send_cmd("AT+CNTP","+CNTP: 1",600);                    //ͬ������ʱ��
}

u8 Scan_Wtime = 0;//����ɨ����Ҫ��ʱ��

//sim800C���������,�����յ���Ӧ��
//str:�ڴ���Ӧ����
//����ֵ:0,û�еõ��ڴ���Ӧ����
//����,�ڴ�Ӧ������λ��(str��λ��)
u8* sim800c_check_cmd(u8 *str)
{
	char *strx=0;
	if(USART2_RX_STA&0X8000)		//���յ�һ��������
	{ 
		USART2_RX_BUF[USART2_RX_STA&0X7FFF]=0;//��ӽ�����
		strx=strstr((const char*)USART2_RX_BUF,(const char*)str);
	} 
	return (u8*)strx;
}

//��sim800C��������
//cmd:���͵������ַ���(����Ҫ��ӻس���),��cmd<0XFF��ʱ��,��������(���緢��0X1A),���ڵ�ʱ�����ַ���.
//ack:�ڴ���Ӧ����,���Ϊ��,���ʾ����Ҫ�ȴ�Ӧ��
//waittime:�ȴ�ʱ��(��λ:10ms)
//����ֵ:0,���ͳɹ�(�õ����ڴ���Ӧ����)
//       1,����ʧ��
u8 sim800c_send_cmd(u8 *cmd,u8 *ack,u16 waittime)
{
	u8 res=0; 
	USART2_RX_STA=0;
	if((u32)cmd<=0XFF)
	{
		while(DMA1_Channel7->CNDTR!=0);	//�ȴ�ͨ��7�������   
		USART2->DR=(u32)cmd;
	}else u2_printf("%s\r\n",cmd);		//��������
	
	if(ack&&waittime)			//��Ҫ�ȴ�Ӧ��
	{ 
	   while(--waittime)	//�ȴ�����ʱ
	   {
		   delay_ms(10);
		   if(USART2_RX_STA&0X8000)//���յ��ڴ���Ӧ����
		   {
			   if(sim800c_check_cmd(ack))break;//�õ���Ч���� 
			   USART2_RX_STA=0;
		   } 
	   }
	   if(waittime==0)res=1; 
	}
	return res;
} 

//GSM��Ϣ��ѯ(�ź�����,��ص���,����ʱ��)
//����ֵ:1,����
//����,�������
u8 sim800c_gsminfo_show(void)
{
	u8 *p,*p1,*p2;
	u8 res = 1;			//����ֵ
	u8 mes[50];
	
	p=mes;
	
	USART2_RX_STA=0;
	if(sim800c_send_cmd("AT+CPIN?","OK",200));			//��ѯSIM���Ƿ���λ 
	USART2_RX_STA=0;
	if(sim800c_send_cmd("AT+COPS?","OK",200)==0)		//��ѯ��Ӫ�����֣�ע�⣺���ﷵ��ֵ��0�����ͳɹ�
	{
		p1=(u8*)strstr((const char*)(USART2_RX_BUF),"\"");
		if(p1)//����Ч����
		{
			p2=(u8*)strstr((const char*)(p1+1),"\"");
			p2[0]=0;//���������			
			sprintf((char*)p,"��Ӫ��:%s",p1+1);
			printf("%s\r\n",p);
		}
		USART2_RX_STA=0;
	}else {res = 0;}
	
	if(sim800c_send_cmd("AT+CSQ","+CSQ:",200)==0)		//��ѯ�ź�����
	{
		p1=(u8*)strstr((const char*)(USART2_RX_BUF),":");
		p2=(u8*)strstr((const char*)(p1),",");
		p2[0]=0;//���������
		sprintf((char*)p,"�ź�����:%s",p1+2);
		printf("%s\r\n",p);
		USART2_RX_STA=0;
	}else {res = 0;}
//	if(sim800c_send_cmd("AT+CBC","+CBC:",200)==0);		//��ѯ��ص���
//	if(sim800c_send_cmd("AT+CCLK?","+CCLK:",200)==0);		//��ѯ��ص���
	
	return res;
} 

//GSM����GPRS͸��
//����ֵ:1,����
//����,�������
u8 sim800c_gsm_config(void)
{
//	u8 *p,*p1,*p2;
	u8 res = 1;			//����ֵ
//	u8 mes[50];
	
//	p=mes;
	
	USART2_RX_STA=0;
	if(sim800c_send_cmd("+++","OK",200));	//+++��ջ��棬����õ�����ֵ0����ζ����+++����ok���˳�͸��ģʽ
	if(sim800c_send_cmd("+++","OK",200));	//+++��ջ��棬����õ�����ֵ1����ζ����+++û�з��������sim������
	USART2_RX_STA=0;
	if(sim800c_send_cmd("AT+CGATT=1","OK",200)==0)		//��GPRS����Ϊ����
	{
		printf("GPRS����\r\n");
	}else {res = 0;}
	
	if(sim800c_send_cmd("AT+CIPSHUT","OK",200)==0)		//�ر�֮ǰ���е�����
	{
		printf("�ر�֮ǰ���е�����\r\n");
	}else {res = 0;}
	
	if(sim800c_send_cmd("AT+CIPMODE=1","OK",200)==0)		//��ģ������Զ�͸��ģʽ
	{
		printf("�����Զ�͸��ģʽ\r\n");
	}else {res = 0;}
	
	if(sim800c_send_cmd("AT+CLPORT=\"TCP\",\"2000\"","OK",200)==0)		//���ñ���TCP�˿�Ϊ2000
	{
		printf("���ñ��ض˿�Ϊ2000\r\n");
	}else {res = 0;}
	
	if(sim800c_send_cmd("AT+CIPSTART=\"TCP\",\"120.78.144.55\",\"20006\"","OK",200)==0)		//���ӷ�������sim800cʹ�õ��Ǵ���2
	{																																										 	//GPRS����Ϊ͸��ģʽ�����е����ݽ�ͨ������2ֱ�ӷ��͵�������
		printf("���ӷ�����\r\n");																														//���Ƿ���+++����ok���˳�͸��ģʽ
	}else {res = 0;}	
//	if(sim800c_send_cmd("AT+CBC","+CBC:",200)==0);		//��ѯ��ص���
//	if(sim800c_send_cmd("AT+CCLK?","+CCLK:",200)==0);		//��ѯ��ص���
	
	return res;
} 
