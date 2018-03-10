#ifndef __HX711_H__ 
#define __HX711_H__ 

//这里声明了三个称重传感器的函数
void ADInit(void);
unsigned long HX711_Read(void);

void ADInit_Right(void);
unsigned long HX711_Read_Right(void);


void ADInit_Left(void);
unsigned long HX711_Read_Left(void);



#endif 
