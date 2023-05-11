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
*    aarch64-linux-gnu-gcc *.c -o xyzmodem_send_hj
*********************************************************************************************************
*/
#include <stdio.h>
#include <stdlib.h>
#include "stdint.h"
#include "uart_to_mcu.h"
#include "xyzmodem.h"
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
 /* Ô´ÎÄ¼þÂ·¾¶ */
//char SourceFile[] = "./app.bin";    
static const char* my_opt = "Yyf:";

//70.需要server关闭串口，防止升级失败
//val: 0 表示临时关闭，1表示开启
int drvControlttyS0(int val);  //因为要升级单片机程序，需要临时关闭串口。








//ps -ef | grep drv722_22134_server | grep -v grep | wc -l
//尝试启动server进程
//返回0表示没有该进程，1表示存在进程了,-1表示出错
static int is_server_process_start(char * cmd_name)
{
	FILE *ptr = NULL;
	char cmd[256] = "ps -ef | grep %s | grep -v grep | wc -l";
//	int status = 0;
	char buf[64];
	int count;

//	printf("server DEBUG:cmd_name = %s\n",cmd_name);
	snprintf(cmd,sizeof cmd,"ps -ef | grep %s | grep -v grep | wc -l",cmd_name);
	printf("ServerDEBUG: api check serverProcess is running, cmd = %s\n",cmd);

	if((ptr = popen(cmd, "r")) == NULL)
	{
		printf("is_server_process_start popen err\n");
		return -1;
	}
	memset(buf, 0, sizeof(buf));
	if((fgets(buf, sizeof(buf),ptr))!= NULL)//获取进程和子进程的总数
	{
//		printf("server: buf = %s\n",buf);
		count = atoi(buf);
		if(count < 1)//当进程数小于等于2时，说明进程不存在, 1表示有一个，是grep 进程的
		{
			pclose(ptr);
			printf("ServerDEBUG: check serverProcess: no server process!!\n");
			return 0;  //系统中没有该进程	
		}		
		printf("ServerDEBUG: check serverProcess: server process is running!!\n");
	}
	pclose(ptr);
	return 1;
}






int main(int argc,char* argv[]) 
{
	char* filename = "./app.bin";
	int get_name = 0,c;
	int serverflag = 0;//,ret
	int go_ahead = 1;    //继续吗？0要键入字符y才能继续


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
	            case 'y':
	            case 'Y':
	            	go_ahead = 0;	       
	       	 	default:	       
	                break;
	                //return 0;
	        }
	        // if(get_name)  //Ìø³ö´óÑ­»·
	        // 	break;
	    }
	}

	if(go_ahead)
	{
		printf("注意事项：\n");
		printf("由于本次升级使用通信串口，为了避免升级过程中的干扰，本程序会关闭drv722_22134_server服务程序\n");
		printf("可能将导致应用程序异常退出，敬请关注\n");
		printf("如果升级失败，请关闭相关使用串口的程序，重新尝试，感谢您的使用\n");
		printf("如继续升级，请键入y或者Y，回车，其他输入则退出升级\n");
		c = getchar();
		if(c == 'y' || c == 'Y')
		{
			printf("继续升级流程\n");
		}
		else
		{
			printf("升级流程中止，感谢您的使用\n");
			return 1;
		}
	}

	if(1 == is_server_process_start("drv722_22134_server"))  //存在server进程
	{
		system("killall drv722_22134_server");
	}

	uart_init(argc, argv);

    if(0 == xymodem_send(filename))
    	printf("%s is done!\n",argv[0]);


    uart_exit() ;   //关闭打开的串口
    // if(serverflag)  //服务存在，则通知服务程序
    // 	drvControlttyS0(1);  //drv722_22134_server开启串口

    return 0;
}


