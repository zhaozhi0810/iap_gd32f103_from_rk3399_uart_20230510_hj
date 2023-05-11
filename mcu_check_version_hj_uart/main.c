/*
*********************************************************************************************************
*
*	Ä£¿éÃû³Æ : Ö÷º¯Êý
*	ÎÄ¼þÃû³Æ : main.c
*	°æ    ±¾ : V1.0
*	Ëµ    Ã÷ : Ö÷¹¦ÄÜ
*
*	ÐÞ¸Ä¼ÇÂ¼ 
*		°æ±¾ºÅ    ÈÕÆÚ         ×÷Õß      ËµÃ÷
*		V1.0    2023-04-06     dazhi    Ê×·¢
*
*	Copyright (C), 2022-2030, htjc by dazhi
*
*
*    aarch64-linux-gnu-gcc *.c -o read_mcu_version_hj
*********************************************************************************************************
*/
#include <stdio.h>
#include <stdlib.h>
#include "stdint.h"
#include "uart_to_mcu.h"
//#include "xyzmodem.h"
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
 /* Ô´ÎÄ¼þÂ·¾¶ */
//char SourceFile[] = "./app.bin";    
static const char* my_opt = "f:";

//70.需要server关闭串口，防止升级失败
//val: 0 表示临时关闭，1表示开启
int drvControlttyS0(int val);  //因为要升级单片机程序，需要临时关闭串口。




uint8_t checksum(uint8_t *buf, uint8_t len)
{
	uint8_t sum;
	uint8_t i;

	for(i=0,sum=0; i<len; i++)
		sum += buf[i];

	return sum;
}

#if 0
void send_update_cmd_tomcu(uint8_t phase)
{
	uint8_t buf[] = {0xa5,74,0,0xef};   //Éý¼¶ÃüÁî

	if(phase)
	{
		buf[2] = 1;  //确认需要下载
		buf[3] = 0xf0;  //check sum
	}	

	UART_SendPacket(buf, 4);   //4¸ö×Ö½Ú·¢³öÈ¥
}

#else

int  send_update_cmd_tomcu(uint8_t*data,uint8_t phase)
{
	uint8_t buf[] = {0xa5,74,0,0xef};   //校验和不包括帧头
	int ret;
	int i = 0;
	

	if(phase == 2)
	{
		buf[2] = 2;  //确认需要下载
		buf[3] = 0xf1;	//check sum
		UART_SendPacket(buf, 4);   //4¸ö×Ö½Ú·¢³öÈ¥

		usleep(1000);

		ret = UART_ReceivePacket (data, 24, 1000);   //一次性读是正常的。
		if(ret == 0)
		{			
			return 0;
		}

	}	
	else
	{
		UART_SendPacket(buf, 4);   //4¸ö×Ö½Ú·¢³öÈ¥
		usleep(1000);

		ret = UART_ReceivePacket (data, 35, 1000);   //一次性读是正常的。
		if(ret == 0)
		{
			return 0;
		}		
	}

	return -1;
}

#endif

//·µ»ØÖµÎª0 ±íÊ¾ÒªÉý¼¶£¬ÆäËûÖµ²»Éý¼¶
static int read_mcu_md5_version(uint8_t phase)
{
	char data[40] = {0};
	int offset=0;
	uint8_t csum = 0,c,rsum = 0;
	int ret;
	uint8_t need_data = 0;
	uint8_t verion = 0;

	if(phase == 0)
		need_data = 34;  //md5 需要35个字节
	else if(phase == 2)
		need_data = 23;  //md5 需要24个字节
	else
	{
		return -1;
	}

	ret = send_update_cmd_tomcu(data,phase);

	if(ret == 0)
	{
		rsum = data[need_data];
		data[need_data] = 0;
		csum = checksum(data, need_data);
		if(csum == rsum)
		{
			//printf("checksum: ok\n");
			if(phase == 0)
			{
				printf("md5:%s\n",data+2);	
			}
			else if(phase == 2)
			{
				verion = data[need_data-1];  //获得版本
				data[need_data-1] = 0;	
				printf("time:%s\n",data+2);	
				printf("version:%d\n",verion);	
			}
		}
		else
		{
			printf("error: checksum\n");
			return -1;
		}
	}	
	return 0;
}







int main(int argc,char* argv[]) 
{
	char* filename = "./app.bin";
	int get_name = 0,c;
	int serverflag = 0;//,ret

    if(argc != 1)
	{	
	    while(1)
	    {
	        c = getopt(argc, argv, my_opt);
	        //printf("optind: %d\n", optind);
	        if (c < 0)
	        {
	            break;
	        }
	        //printf("option char: %x %c\n", c, c);
	        switch(c)
	        {
	        	case 'f':
	        		filename = optarg;
	        		get_name = 1;
	                //debug_level = atoi(optarg);
	                printf("filename = %s\n",filename);
	                break;	       
	       	 	default:	       
	                break;
	                //return 0;
	        }
	        if(get_name)  //Ìø³ö´óÑ­»·
	        	break;
	    }
	}

//	if(1 == is_server_process_start("drv722_22134_server"))  //存在server进程
	{
//		system("killall drv722_22134_server");
		// serverflag = 1;
		// drvControlttyS0(0);  //drv722_22134_server关闭串口
		// printf("drv722_22134_server is running!!\n");
	}	
	uart_init(argc, argv);
	//sleep(1);

	//sleep(1);

	printf("HJ mcu info:\n");
    read_mcu_md5_version(0);
    read_mcu_md5_version(2);

    uart_exit() ;   //关闭打开的串口
    // if(serverflag)  //服务存在，则通知服务程序
    // 	drvControlttyS0(1);  //drv722_22134_server开启串口

    return 0;
}


