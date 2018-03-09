#include "stm32f10x.h" 
#include "delay.h" 
#include "hx711.h" 

#define ADIO GPIOB        
#define DATA GPIO_Pin_12	  //
#define CLK  GPIO_Pin_13	  //
#define ADCLK RCC_APB2Periph_GPIOB  //

void ADInit_Right(void) //
{ 
	GPIO_InitTypeDef GPIO_InitStructure; 

	RCC_APB2PeriphClockCmd(ADCLK,ENABLE); 
	
	GPIO_InitStructure.GPIO_Pin = CLK; 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; //
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
	GPIO_Init(ADIO,&GPIO_InitStructure); 

		GPIO_InitStructure.GPIO_Pin = DATA; 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
	GPIO_Init(ADIO,&GPIO_InitStructure);	
} 

unsigned long HX711_Read_Right(void)  //
{ 
	unsigned long val = 0; 
	unsigned char i = 0; 

	GPIO_SetBits(ADIO,DATA); 
	GPIO_ResetBits(ADIO,CLK); 
	while(GPIO_ReadInputDataBit(ADIO,DATA)); 
	delay_us(1); 
	for(i=0;i<24;i++) 
	{ 
		GPIO_SetBits(ADIO,CLK); 
		val=val<<1; 
		delay_us(1);  
		GPIO_ResetBits(ADIO,CLK); 
		if(GPIO_ReadInputDataBit(ADIO,DATA)) 
		val++; 
		delay_us(1); 
	} 
	GPIO_SetBits(ADIO,CLK); 
	val = val^0x800000; 
	delay_us(1); 
	GPIO_ResetBits(ADIO,CLK); 
	delay_us(1);  
	return val; 	
}
