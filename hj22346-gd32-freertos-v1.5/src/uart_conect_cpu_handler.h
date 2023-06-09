
#ifndef UART_CONECT_CPU_HANDLER_H
#define UART_CONECT_CPU_HANDLER_H


#include <gd32f10x.h>
#include "uart.h"
#include "task.h"
/*
2022-04-21 调整（不主动发送呢，就可以节省cpu和单片机的精力，不用老是处理）


*/

extern TaskHandle_t  TaskHandle_ToCpu_Com;   //存放调试串口任务指针



//注意单片机与cpu保持一致  2022-07-28
//#pragma pack(1) 这个会设置全局的，注释掉
//数据总共4个字节，这里不含帧头
typedef struct
{
	unsigned char data_type;   //led的控制，状态的获取，lcd的熄灭
	unsigned char data;
//	mcu_data_t data;
	unsigned char crc;     //校验和
}__attribute__((packed))com_frame_t;    //注意对齐方式




typedef enum
{	
	eMCU_LED_STATUS_TYPE=50,  //获得led的状态
	eMCU_KEY_STATUS_TYPE,    //51.获得按键的状态
	eMCU_LED_SETON_TYPE,    //52.设置对应的led亮
	eMCU_LED_SETOFF_TYPE,    //53.设置对应的led灭
	eMCU_LCD_SETONOFF_TYPE,  //54.lcd打开关闭
	eMCU_KEY_CHANGE_TYPE,    //55.按键被修改上报
    eMCU_LEDSETALL_TYPE,     //56.对所有led进行控制，点亮或者熄灭
	eMCU_LEDSETPWM_TYPE,     //57.设置所有led的亮度 
	eMCU_GET_TEMP_TYPE,      //58.获得单片机内部温度	
	eMCU_HWTD_SETONOFF_TYPE,   //59.开门狗设置开关
	eMCU_HWTD_FEED_TYPE,       //60.看门狗喂狗
	eMCU_HWTD_SETTIMEOUT_TYPE,    //61.设置看门狗喂狗时间
	eMCU_HWTD_GETTIMEOUT_TYPE,    //62.获取看门狗喂狗时间
	eMCU_RESET_COREBOARD_TYPE,  //63.复位核心板
	eMCU_RESET_LCD_TYPE,        //64.复位lcd 9211（复位引脚没有连通）
	eMCU_RESET_LFBOARD_TYPE,    //65.复位底板，单片机重启
	eMCU_MICCTRL_SETONOFF_TYPE,  //66.MICCTRL 引脚的控制,1.3版本改到3399控制了！！！
	eMCU_LEDS_FLASH_TYPE  ,//67.led闪烁控制
	eMCU_LSPK_SETONOFF_TYPE  , //68.LSPK,2022-11-11 1.3新版增加
	eMCU_V12_CTL_SETONOFF_TYPE ,  //69.V12_CTL,2022-11-14 1.3新版增加
	eMCU_GET_LCDTYPE_TYPE  ,   //70.上位机获得LCD类型的接口，之前是在3399，现在改为单片机实现，2022-12-12
	eMCU_SET_7INCHPWM_TYPE ,  //71.7inch lcd的pwm值调整,2022-12-13
	eMCU_5INLCD_SETONOFF_TYPE  ,  //72.5英寸背光使能引脚的控制,2022-12-13
	eMCU_GET_MCUVERSION_TYPE  ,    //73.获取单片机版本
	eMCU_UPDATE_MCUFIRM_TYPE       //74.单片机升级命令
}mcu_data_type;



//#define FRAME_LENGHT (8)    //数据帧的字节数


//typedef struct frame_buf
//{
//	uint8_t com_handle_buf[FRAME_LENGHT];   //接收缓存
//	uint8_t datalen;            //帧长-缓存中的数据长度，即下一次要读的字节数
//}frame_buf_t;


//消息处理的函数指针
//typedef void (*message_handle)(uint8_t* );

//中断处理函数
//void Com_Cpu_Rne_Int_Handle(void);
//帧数据处理函数
//void Com_Frame_Handle(frame_buf_t* buf, Queue_UART_STRUCT* Queue_buf,message_handle handle);
//中断处理函数
//void Com_Cpu_Idle_Int_Handle(void);




//void Send_Fan_Div_Status_ToCpu(bitstatus_t b_status,uint8_t fan_pwm,uint8_t lcd_pwm);
#if 0
//发送2个电压
void Send_Vol_ToCpu(data_type type,short vol1,short vol2);
//发送温度
void Send_Temp_ToCpu(data_type type,short cpu_temp,short board_temp);

#endif
//发送dvi视频被切换的数据到cpu
//source 1（本地）或者2（外部）
//void Send_Dvi_Change_ToCpu(int source);

//发送命令数据到cpu
//cmd请参考uart.h中宏定义
//param 参数。
//void Send_Cmd_ToCpu(int cmd,int param);

//应答cpu的获取信息的请求
void AnswerCpu_GetInfo(uint16_t ask);
//应答cpu的设置信息的请求 errcode为0表示成功，其他值为错误码 应小于0x7f
//void AnswerCpu_Status(uart_err_t errcode);


//缓存初始化
void Com_Cpu_Recive_Buff_Init(void);


//与cpu通信串口的接收任务
void Com_ToCPU_Recv_Task(void * parameter);

#endif
