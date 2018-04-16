说明：
	开发工具为 keil 5
	按照正点原子FreeRtos实验教程编写

	代码为：当有垃圾时按照json格式发送数据给服务器初步
	
	只有上行数据功能，未有下行数据功能

	关于下行数据时 free()函数与微库的问题，已经放弃free()函数  而使用
	
	json_object_clear(root_down);
	json_decref(root_down);
	root_down = NULL;
	
注意：
	代码未优化，部分地方逻辑扔不合适

注释：	
	20180416
	1. 具体制作硬件时需要给sim800c设置一个单独的开关，因为工程透传使用的特殊性
	这一点很必要
	2. 当sim800c已经配置成功然后返回TCP:CLOSED时，这是连接了错误的端口或服务器
	关闭了相应端口的TCP连接