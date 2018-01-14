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