

/*
	矩阵按键扫描
	
	6*6  通过iic扩展出来的。
	
*/


#include "includes.h"

#define KEYS_IIC_ADDR (0)  //只表示A2A1A0 3个引脚的值
#define NCA9555_IIC_CONTROLER  IIC3_INDEX   //对应外部中断13  2021-12-07

		
//只有6行，这些数字用于分别扫描每一行
const static uint8_t key_scan_line[] = {0xfe,0xfd,0xfb,0xf7,0xef,0xdf};

// 6*6 的键盘矩阵，总共有33个按键，按键个数在h文件中定义
static BTN_INFO g_btns_info;

TaskHandle_t  TaskHandle_key_Matrix;   //存放按键任务指针



//端口的配置
void matrix_keys_init(void)
{
	uint8_t outcfg_dat[2]={0,0xff};   //IIC芯片GPIO输出模式，对应的位要设置为0
	//1. iic的初始化
	nca9555_init(NCA9555_IIC_CONTROLER);
		
	//矩阵按键，P0端口配置为输出，P1端口配置为输入，因为P1端口上用了上拉电阻
	nca9555_write_2config(NCA9555_IIC_CONTROLER,KEYS_IIC_ADDR,outcfg_dat);

	nca9555_write_outport(NCA9555_IIC_CONTROLER,KEYS_IIC_ADDR,0, 0); //P0端口输出0

#ifdef 	BTNS_USE_INT   //宏在btns_leds.h中定义
	//中断引脚初始化
	//2. 中断引脚的初始化 PB12，外部中断12
	//2.1 时钟使能
	rcu_periph_clock_enable(RCU_GPIOB);
	rcu_periph_clock_enable(RCU_AF);		
	
	gpio_init(GPIOB, GPIO_MODE_IPU, GPIO_OSPEED_2MHZ, GPIO_PIN_12);	
	
	//2.2 复用为外部中断引脚，
	gpio_exti_source_select(GPIO_PORT_SOURCE_GPIOB, GPIO_PIN_SOURCE_12);
	
	//设置触发方式，低电平触发
	exti_init(EXTI_12, EXTI_INTERRUPT, EXTI_TRIG_FALLING);
	exti_interrupt_enable(EXTI_12);
	exti_interrupt_flag_clear(EXTI_12);
	//2.3 nvic允许中断
	//中断控制器使能，使用的是外部中断12
	nvic_irq_enable(EXTI10_15_IRQn,  7, 0);   //允许中断，并设置优先级

	//初始化之后读取一次
	matrix_keys_row_scan();	

#endif			
	memset(&g_btns_info,0,sizeof(g_btns_info));   //数据清零	
}








static uint8_t matrix_keys_row_scan(void)
{
	uint8_t key_row_dat;
	
	if(nca9555_read_inport(NCA9555_IIC_CONTROLER,KEYS_IIC_ADDR,1,&key_row_dat) == 0)  //表示读取成功
	{
		if((key_row_dat&0x3f) != 0x3f)   //只判断低6位，不相等表示有按键按下
		{
			return key_row_dat&0x3f;
		}
		else
		{
			return 0x3f;
		//	printf("ERROR: KEY_ROW_SCAN key_row_dat == 0x3f\r\n");
		}
	}
	else //iic读取失败
	{
		printf("ERROR: KEY_ROW_SCAN nca9555_read_inport\r\n");
		return 0xff;
	}
}



#if 1   //2023-02-09  增加
static uint8_t release_report = 0;  //松开上报。


//检测到按键释放后，扫描一次行。
//P1(L信号)输出高，P0(H信号) 改为输入，读到某些位高电平表示有按键被按下，如果是0则表示按键都松开
//col : 0-5 分别表示L1 - L6
uint8_t matrix_keys_col_scan(uint8_t col)
{
	uint8_t outcfg_dat[2]={0xff,0};  //P0改为输入，P1改为输出，并且输出高
	uint8_t key_row_dat;
	uint8_t outdat = 0;
	uint8_t ret;
	
	//P1 输出1
	outdat = 1<< col;
	nca9555_write_outport(NCA9555_IIC_CONTROLER,KEYS_IIC_ADDR,1, outdat); //P1（L信号）端口输出0
	
	//P0改输入之前，先输出0，释放一下电平
	nca9555_write_outport(NCA9555_IIC_CONTROLER,KEYS_IIC_ADDR,0, 0);
	//配置
	nca9555_write_2config(NCA9555_IIC_CONTROLER,KEYS_IIC_ADDR,outcfg_dat);
	//读P0（H信号）端口,没有按键时，应该是0，有按键时不是0
	if(nca9555_read_inport(NCA9555_IIC_CONTROLER,KEYS_IIC_ADDR,0,&key_row_dat) == 0)  //表示读取成功
	{
		if((key_row_dat & 0x3f) != 0)   //只判断低6位，不相等表示有按键按下
		{
			ret = key_row_dat&0x3f;
		}
		else
		{
			ret = 0x7f;
		//	printf("ERROR: KEY_ROW_SCAN key_row_dat == 0x3f\r\n");
		}
	}
	else //iic读取失败
	{
		printf("ERROR: KEY_ROW_SCAN nca9555_read_inport\r\n");
		ret = 0xff;
	}
	
	//配置，默认是P0(H信号)输出，P1(L信号) 改为输入
	outcfg_dat[0]=0;   
	outcfg_dat[1]=0xff;
	nca9555_write_2config(NCA9555_IIC_CONTROLER,KEYS_IIC_ADDR,outcfg_dat);
	
	return ret;
}


//识别是哪个按键
//col : 0-5 分别表示L1 - L6
void matrix_keys_scan_col(uint8_t col,uint8_t dat)
{	
	uint8_t index,j;
	if(!dat || (dat >= 0x3f))
		return;
	
	if(col > 5)
		return;
	
	for(j=0;j<ROW_NUM;j++)  //每一行扫描一次
	{
		index = 6*j+col;
		if(index > 35)
			continue;
		if(((dat>>j)&(1))) //高电平表示按下，低表示松开
		{
			if(!g_btns_info.value[index])
			{	
			//	g_btns_info.pressCnt[index]++;
			//	if(g_btns_info.pressCnt[index] == 2)//检测到不止1次
				{   //条件限制上报一次
					g_btns_info.value[index] = 1;
				//	g_btns_info.reportEn[index] = 1;  //按下上报
					send_btn_change_to_cpu(index+1,1); //发送按键按下/松开
					if(more_debug_info)
						printf("@#@#btn:%d press\r\n",index+1);
					release_report = 1;   //记录需要释放标志
				}
			}
		}
		else if(g_btns_info.value[index]) //之前的状态是按下
		{								
			g_btns_info.value[index] = 0;
		//	g_btns_info.reportEn[index] = 2;   //松开上报
			send_btn_change_to_cpu(index+1,0); //发送按键按下/松开
			if(more_debug_info)
				printf("@@@btn:%d release\r\n",index+1);
			g_btns_info.pressCnt[index] = 0;			
		}
	}	
}



#endif




							

/***
 *函数名：KEY_SCAN
 *功  能：6*6按键扫描
 *返回值：1~36，对应36个按键,0表示没有检测到
 */
char matrix_keys_scan(void)
{    
    uint8_t key_row_num=0;        //行扫描结果记录
    uint8_t i,j;
	uint8_t index,col_dat;   //
	uint8_t unpress_count = 0;   //明明进了中断，但是又没见有按键，考虑是列不能识别  
	
	
	key_row_num = matrix_keys_row_scan();
	if(key_row_num < 0x3f)   //读取到了一个有效的按键触发
	{
		for(i=0;i<COL_NUM;i++)  //每一列扫描一次
		{
			if(nca9555_write_outport(NCA9555_IIC_CONTROLER,KEYS_IIC_ADDR,0, key_scan_line[i])) //P0端口输出0
			{
				printf("ERROR: KEY_ROW_SCAN nca9555_write_outport i=%d\r\n",i);
				continue;  //写入失败，直接往下试试
				//	return -1;
			}
			//再次读取输入
			key_row_num = matrix_keys_row_scan();
			if(key_row_num >= 0x3f)   //扫描到这一行没有按下
				unpress_count++;

			for(j=0;j<ROW_NUM;j++)  //每一行扫描一次
			{
				index = 6*i+j;
				if(!((key_row_num>>j)&(1))) //按下
				{
					if(g_btns_info.pressCnt[index] < 2)
					{	
						g_btns_info.pressCnt[index]++;
						if(g_btns_info.pressCnt[index] == 2)//检测到不止1次
						{   //条件限制上报一次
							g_btns_info.value[index] = 1;
						//	g_btns_info.reportEn[index] = 1;  //按下上报
							send_btn_change_to_cpu(index+1,1); //发送按键按下/松开
							if(more_debug_info)
								printf("----btn:%d press\r\n",index+1);
							release_report = 1;   //记录需要释放标志
						}
					}
				}
				else //松开
				{
					if(g_btns_info.value[index]) //之前的状态是按下
					{
						#if 1
						col_dat = matrix_keys_col_scan(j);
				//		printf("j = %d dat = %#x,index = %d\r\n",j,col_dat,index);
						if(col_dat >= 0x3f)
						{						
							g_btns_info.value[index] = 0;
						//	g_btns_info.reportEn[index] = 2;   //松开上报
							send_btn_change_to_cpu(index+1,0); //发送按键按下/松开
							if(more_debug_info)
								printf("++++btn:%d release\r\n",index+1);
							g_btns_info.pressCnt[index] = 0;
						}
						else{
							matrix_keys_scan_col(j,col_dat);
						}
						#else
						g_btns_info.value[index] = 0;
					//	g_btns_info.reportEn[index] = 2;   //松开上报
						send_btn_change_to_cpu(index+1,0); //发送按键按下/松开
						printf("++++btn:%d release\r\n",index+1);
						g_btns_info.pressCnt[index] = 0;
						#endif
					}		
				}
			}
		}
		
		if(unpress_count >= 6) //明明进了中断，但是又没见有按键
		{
			for(index=0;index<COL_NUM;index++)   //再进行一次行扫描
			{
				col_dat = matrix_keys_col_scan(index);
			//	printf("33- dat = %#x,index = %d\r\n",col_dat,index);
				if(col_dat < 0x3f)
					matrix_keys_scan_col(index,col_dat);
			}
		}		
	}
	else
	{
		if(release_report)  //需要上报释放信息。
		{		
			for(index=0;index<COL_NUM*ROW_NUM;index++)
			{
				if(g_btns_info.value[index]) //之前的状态是按下
				{
					#if 1
					j = index%6;
					col_dat = matrix_keys_col_scan(j);
				//	printf("22- j = %d dat = %#x,index = %d\r\n",j,col_dat,index);
					if(col_dat >= 0x3f)
					{						
						g_btns_info.value[index] = 0;
					//	g_btns_info.reportEn[index] = 2;   //松开上报
						send_btn_change_to_cpu(index+1,0); //发送按键按下/松开
						if(more_debug_info)
							printf("####btn:%d release\r\n",index+1);
						g_btns_info.pressCnt[index] = 0;
					}
					#else
				
					g_btns_info.value[index] = 0;
				//	g_btns_info.reportEn[index] = 2;   //松开上报
					send_btn_change_to_cpu(index+1,0); //发送按键按下/松开
					printf("#####btn:%d release\r\n",index+1);
					g_btns_info.pressCnt[index] = 0;
					#endif
				}
				else{
					matrix_keys_scan_col(j,col_dat);
				}
				
				//检测到的按键次数全部清零
				g_btns_info.pressCnt[index] = 0;
				
			}
		//	btn_start_scan = 0;   //按键不再扫描
			release_report = 0;
		}
	}
//	unpress_count = 0;   //计数没有按键次数清零
	nca9555_write_outport(NCA9555_IIC_CONTROLER,KEYS_IIC_ADDR,0, 0);	
	
	return 0;
}



#if 0
/***
 *函数名：KEY_SCAN
 *功  能：6*6按键扫描
 *返回值：1~36，对应36个按键,0表示没有检测到
 */
char matrix_keys_scan(void)
{    
    uint8_t key_row_num=0;        //行扫描结果记录
    uint8_t i,j;
	uint8_t index;   //
	static uint8_t release_report = 0;  //松开上报。
	
	key_row_num = matrix_keys_row_scan();
	if(key_row_num < 0x3f)   //读取到了一个有效的按键触发
	{
		for(i=0;i<COL_NUM;i++)  //每一列扫描一次
		{
			if(nca9555_write_outport(NCA9555_IIC_CONTROLER,KEYS_IIC_ADDR,0, key_scan_line[i])) //P0端口输出0
			{
				printf("ERROR: KEY_ROW_SCAN nca9555_write_outport i=%d\r\n",i);
				continue;  //写入失败，直接往下试试
				//	return -1;
			}
			//再次读取输入
			key_row_num = matrix_keys_row_scan();

			for(j=0;j<ROW_NUM;j++)  //每一行扫描一次
			{
				index = 6*i+j;
				if(!((key_row_num>>j)&(1))) //按下
				{
					if(g_btns_info.pressCnt[index] < 2)
					{	
						g_btns_info.pressCnt[index]++;
						if(g_btns_info.pressCnt[index] == 2)//检测到不止1次
						{   //条件限制上报一次
							g_btns_info.value[index] = 1;
						//	g_btns_info.reportEn[index] = 1;  //按下上报
							send_btn_change_to_cpu(index+1,1); //发送按键按下/松开
							printf("----btn:%d press\r\n",index+1);
							release_report = 1;   //记录需要释放标志
						}
					}
				}
				else //松开
				{
					if(g_btns_info.value[index]) //之前的状态是按下
					{
						g_btns_info.value[index] = 0;
					//	g_btns_info.reportEn[index] = 2;   //松开上报
						send_btn_change_to_cpu(index+1,0); //发送按键按下/松开
						printf("++++btn:%d release\r\n",index+1);
						g_btns_info.pressCnt[index] = 0;
					}		
				}
			}
		}
	}
	else
	{
		if(release_report)  //需要上报释放信息。
		{		
			for(index=0;index<COL_NUM*ROW_NUM;index++)
			{
				if(g_btns_info.value[index]) //之前的状态是按下
				{
					g_btns_info.value[index] = 0;
				//	g_btns_info.reportEn[index] = 2;   //松开上报
					send_btn_change_to_cpu(index+1,0); //发送按键按下/松开
					printf("#####btn:%d release\r\n",index+1);
					g_btns_info.pressCnt[index] = 0;
				}
				
			}
		//	btn_start_scan = 0;   //按键不再扫描
			release_report = 0;
		}
	}

	nca9555_write_outport(NCA9555_IIC_CONTROLER,KEYS_IIC_ADDR,0, 0);	
	
	return 0;
}

#endif






#ifdef 	BTNS_USE_INT
//外部中断12的处理函数,按键按下和松开都会触发中断！！！！
void exint12_handle(void)
{

	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	
	xTaskNotifyFromISR(TaskHandle_key_Matrix, 0, eIncrement, &xHigherPriorityTaskWoken);  //唤醒休眠的任务
	//并且禁止中断
	exti_interrupt_disable(EXTI_12);   //扫描完毕之后再使能
}

#endif

#if 0
void EXTI10_15_IRQHandler(void)
{
	if(exti_interrupt_flag_get(EXTI_12))
	{
		
		exint12_handle();
		//	exint456_handle();
	}
	exti_interrupt_flag_clear(EXTI_12);  //清冲断标志
}


#endif


/*
	main函数周期扫描任务，30ms一次,查询方式

	btn_start_scan 由中断函数设置，按下和松开都会设置为非0
	抖动触发。
*/
void task_matrix_keys_scan(void* arg)
{	
	//1. 矩阵按键扫描初始化
	matrix_keys_init();

	while(1)
	{	
		//等待任务被唤醒
		ulTaskNotifyTake(pdFALSE, portMAX_DELAY);   //减1，然后无限等待
		{	
			matrix_keys_scan();

			vTaskDelay(30);    //延时30ms
#ifdef 	BTNS_USE_INT	
			exti_interrupt_enable(EXTI_12);   //扫描完毕之后再使能		
#endif

		}		
	}
}



