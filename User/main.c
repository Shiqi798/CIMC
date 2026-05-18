#include "HeaderFiles.h"

#define FILE_MAX_RECORDS     10U
#define FILE_PATH_LEN        64U
#define app_buf_size         1024U//app.bin文件的缓冲区大小，单位字节
#define BOOTLOADER_ADDR 0x08000000
#define APP_SIZE 4 // 4*16KB = 64KB

int main(void)
{	

	sysFunction_Init();
	sysFunction_loop();
	
//		clear_all_logs();
	
}



// 函数指针：指向App复位中断
typedef void (*pFunction)(void);
void backto_bootloader(void);

void backto_bootloader(void)
{
	led1_off();
	//关闭所有中断，防止跳转时干扰
    __disable_irq();
    //关闭系统定时器中断
    SysTick->CTRL = 0;
	//定义跳转函数指针
	pFunction JumpToBootloader;
	//设置栈指针为App栈顶
	__set_MSP(*(uint32_t*)BOOTLOADER_ADDR);
	//获取App复位中断地址
	JumpToBootloader = (pFunction)*(uint32_t*)(BOOTLOADER_ADDR + 4);
	JumpToBootloader();
}

/*
int main(void)
{
//    spi_flash_erase();



    sysFunction_Init();
	led1_on();
	while(1)
	{
		led1_on();
		led2_off();
		delay_1ms(100);
		if(Key_Check(0,KEY_DOWN)==1)
		{
			backto_bootloader();//
		}
	}
//    
	
	
}
*/
