#include "oled.h"
#include "oledfont.h"

u8 OLED_GRAM[144][4]; //128*32?????


/************************************************************ 
 * Function :       OLED_ColorTurn
 * Comment  :       ?��???????????
 * Parameter:       null
 * Return   :       null
 * Author   :       
 * Date     :       
************************************************************/
void OLED_ColorTurn(u8 i)
{
	if(i==0)
		{
			OLED_WR_Byte(0xA6,OLED_CMD);//???????
		}
	if(i==1)
		{
			OLED_WR_Byte(0xA7,OLED_CMD);//??????
		}
}


/************************************************************ 
 * Function :       OLED_DisplayTurn
 * Comment  :       ??????180
 * Parameter:       null
 * Return   :       null
 * Author   :       
 * Date     :       
************************************************************/
void OLED_DisplayTurn(u8 i)
{
	if(i==0)
		{
			OLED_WR_Byte(0xC8,OLED_CMD);//???????
			OLED_WR_Byte(0xA1,OLED_CMD);
		}
	if(i==1)
		{
			OLED_WR_Byte(0xC0,OLED_CMD);//??????
			OLED_WR_Byte(0xA0,OLED_CMD);
		}
}


/************************************************************ 
 * Function :       IIC_delay
 * Comment  :       IIC???
 * Parameter:       null
 * Return   :       null
 * Author   :       Lingyu Meng
 * Date     :       
************************************************************/
void IIC_delay(void)
{
	uint16_t t = 120;
	while(t--);
	//delay_1ms();   //??????????
}

/************************************************************ 
 * Function :       I2C_Start
 * Comment  :       IIC??????
 * Parameter:       null
 * Return   :       null
 * Author   :       
 * Date     :       
************************************************************/
void I2C_Start(void)
{
	
	// ??SDA????????????
    gpio_mode_set(OLED_SDA_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, OLED_SDA_PIN);
    gpio_output_options_set(OLED_SDA_PORT, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, OLED_SDA_PIN);
	
	//???????IIC ????
	OLED_SDIN_Set();
	OLED_SCLK_Set();
	IIC_delay();
	OLED_SDIN_Clr();
	IIC_delay();
	OLED_SCLK_Clr();
	IIC_delay();
}

/************************************************************ 
 * Function :       I2C_Stop
 * Comment  :       IIC???????
 * Parameter:       null
 * Return   :       null
 * Author   :       
 * Date     :       
************************************************************/
void I2C_Stop(void)
{
	
	// ??SDA????????????
    gpio_mode_set(OLED_SDA_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, OLED_SDA_PIN);
    gpio_output_options_set(OLED_SDA_PORT, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, OLED_SDA_PIN);
	
	OLED_SDIN_Clr();
	OLED_SCLK_Set();
	IIC_delay();
	OLED_SDIN_Set();
}

//?????????
void I2C_WaitAck(void) //????????????
{
	u8 t;

	OLED_SDIN_Set();
	IIC_delay();
	OLED_SCLK_Set();
	IIC_delay();
	 // ??SDA?????????
    gpio_mode_set(OLED_SDA_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, OLED_SDA_PIN);
	while(IIC_READ_SDA)
	{
		t++;
		//printf("%d times:i2c wait out time!\r\n",t);  //??????
		if(t>254)
		{
			//printf("=========================iic TIMEOUT STOP!=====================\r\n"); //???????IIC?????
			I2C_Stop();
			
		}
	}
	OLED_SCLK_Clr();
	
	IIC_delay();
	
	// ??SDA????????????
    gpio_mode_set(OLED_SDA_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, OLED_SDA_PIN);
    gpio_output_options_set(OLED_SDA_PORT, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, OLED_SDA_PIN);
    
}

//��????????
void Send_Byte(u8 dat)
{
	u8 i;
	for(i=0;i<8;i++)
	{
		OLED_SCLK_Clr();//?????????????????
		if(dat&0x80)//??dat??8��?????��????��??
		{
			OLED_SDIN_Set();
    }
		else
		{
			OLED_SDIN_Clr();
    }
		IIC_delay();
		OLED_SCLK_Set();
		IIC_delay();
		OLED_SCLK_Clr();
		dat<<=1;
  }
}

//??????????
//??SSD1306��?????????
//mode:????/?????? 0,???????;1,???????;
void OLED_WR_Byte(u8 dat,u8 mode)
{
	I2C_Start();
	Send_Byte(0x78);
	I2C_WaitAck();
	if(mode){Send_Byte(0x40);}
  else{Send_Byte(0x00);}
	I2C_WaitAck();
	Send_Byte(dat);
	I2C_WaitAck();
	I2C_Stop();
}


//????OLED??? 
void OLED_DisPlay_On(void)
{
	OLED_WR_Byte(0x8D,OLED_CMD);//???????
	OLED_WR_Byte(0x14,OLED_CMD);//????????
	OLED_WR_Byte(0xAF,OLED_CMD);//???????
}

//???OLED??? 
void OLED_DisPlay_Off(void)
{
	OLED_WR_Byte(0x8D,OLED_CMD);//???????
	OLED_WR_Byte(0x10,OLED_CMD);//??????
	OLED_WR_Byte(0xAF,OLED_CMD);//??????
}

//??????��OLED	
void OLED_Refresh(void)
{
	u8 i,n;
	for(i=0;i<4;i++)
	{
	   OLED_WR_Byte(0xb0+i,OLED_CMD); //????????????
	   OLED_WR_Byte(0x00,OLED_CMD);   //?????????????
	   OLED_WR_Byte(0x10,OLED_CMD);   //?????????????
	   for(n=0;n<128;n++)
		 OLED_WR_Byte(OLED_GRAM[n][i],OLED_DATA);
  }
}
//????????
void OLED_Clear(void)
{
	u8 i,n;
	for(i=0;i<4;i++)
	{
	   for(n=0;n<128;n++)
			{
			 OLED_GRAM[n][i]=0;//???????????
			}
  }
	OLED_Refresh();//???????
}

//???? 
//x:0~127
//y:0~31
void OLED_DrawPoint(u8 x,u8 y)
{
	u8 i,m,n;
	i=y/8;
	m=y%8;
	n=1<<m;
	OLED_GRAM[x][i]|=n;
}

//????????
//x:0~127
//y:0~31
void OLED_ClearPoint(u8 x,u8 y)
{
	u8 i,m,n;
	i=y/8;
	m=y%8;
	n=1<<m;
	OLED_GRAM[x][i]=~OLED_GRAM[x][i];
	OLED_GRAM[x][i]|=n;
	OLED_GRAM[x][i]=~OLED_GRAM[x][i];
}

//????
//x:0~128
//y:0~31
void OLED_DrawLine(u8 x1,u8 y1,u8 x2,u8 y2)
{
	u8 i,k,k1,k2;
	if(x1==x2)    //??????
	{
			for(i=0;i<(y2-y1);i++)
			{
				OLED_DrawPoint(x1,y1+i);
			}
  }
	else if(y1==y2)   //??????
	{
			for(i=0;i<(x2-x1);i++)
			{
				OLED_DrawPoint(x1+i,y1);
			}
  }
	else      //??��??
	{
		k1=y2-y1;
		k2=x2-x1;
		k=k1*10/k2;
		for(i=0;i<(x2-x1);i++)
			{
			  OLED_DrawPoint(x1+i,y1+i*k/10);
			}
	}
}
//x,y:???????
//r:????
void OLED_DrawCircle(u8 x,u8 y,u8 r)
{
	int a, b,num;
    a = 0;
    b = r;
    while(2 * b * b >= r * r)      
    {
        OLED_DrawPoint(x + a, y - b);
        OLED_DrawPoint(x - a, y - b);
        OLED_DrawPoint(x - a, y + b);
        OLED_DrawPoint(x + a, y + b);
 
        OLED_DrawPoint(x + b, y + a);
        OLED_DrawPoint(x + b, y - a);
        OLED_DrawPoint(x - b, y - a);
        OLED_DrawPoint(x - b, y + a);
        
        a++;
        num = (a * a + b * b) - r*r;//????????????????
        if(num > 0)
        {
            b--;
            a--;
        }
    }
}



//?????��???????????,???????????
//x:0~127
//y:0~31
//size:??????? 12/16/24
void OLED_ShowChar(u8 x,u8 y,u8 chr,u8 size1)
{
	u8 i,m,temp,size2,chr1;
	u8 y0=y;
	size2=(size1/8+((size1%8)?1:0))*(size1/2);  //??????????????????????????????
	chr1=chr-' ';  //??????????
	for(i=0;i<size2;i++)
	{
		if(size1==12)
        {temp=asc2_1206[chr1][i];} //????1206????
		else if(size1==16)
        {temp=asc2_1608[chr1][i];} //????1608????
		else if(size1==24)
        {temp=asc2_2412[chr1][i];} //????2412????
		else return;
				for(m=0;m<8;m++)           //��??????
				{
					if(temp&0x80)OLED_DrawPoint(x,y);
					else OLED_ClearPoint(x,y);
					temp<<=1;
					y++;
					if((y-y0)==size1)
					{
						y=y0;
						x++;
						break;
          }
				}
  }
}


//????????
//x,y:???????  
//size1:?????�� 
//*chr:??????????? 
void OLED_ShowString(u8 x,u8 y,u8 *chr,u8 size1)
{
	while((*chr>=' ')&&(*chr<='~'))//?��???????????!
	{
		OLED_ShowChar(x,y,*chr,size1);
		x+=size1/2;
		if(x>128-size1)  //????
		{
			x=0;
			y+=2;
    }
		chr++;
  }
}

//m^n
u32 OLED_Pow(u8 m,u8 n)
{
	u32 result=1;
	while(n--)
	{
	  result*=m;
	}
	return result;
}

////???2??????
////x,y :???????	 
////len :?????��??
////size:?????��
void OLED_ShowNum(u8 x,u8 y,u32 num,u8 len,u8 size1)
{
	u8 t,temp;
	for(t=0;t<len;t++)
	{
		temp=(num/OLED_Pow(10,len-t-1))%10;
			if(temp==0)
			{
				OLED_ShowChar(x+(size1/2)*t,y,'0',size1);
      }
			else 
			{
			  OLED_ShowChar(x+(size1/2)*t,y,temp+'0',size1);
			}
  }
}

//???????
//x,y:???????
//num:???????????
void OLED_ShowChinese(u8 x,u8 y,u8 num,u8 size1)
{
	u8 i,m,n=0,temp,chr1;
	u8 x0=x,y0=y;
	u8 size3=size1/8;
	while(size3--)
	{
		chr1=num*size1/8+n;
		n++;
			for(i=0;i<size1;i++)
			{
				if(size1==16)
						{temp=Hzk1[chr1][i];}//????16*16????
				else if(size1==24)
						{temp=Hzk2[chr1][i];}//????24*24????
				else if(size1==32)       
						{temp=Hzk3[chr1][i];}//????32*32????
				else if(size1==64)
						{temp=Hzk4[chr1][i];}//????64*64????
				else return;
							
						for(m=0;m<8;m++)
							{
								if(temp&0x01)OLED_DrawPoint(x,y);
								else OLED_ClearPoint(x,y);
								temp>>=1;
								y++;
							}
							x++;
							if((x-x0)==size1)
							{x=x0;y0=y0+8;}
							y=y0;
			 }
	}
}

//num ???????????
//space ???????????
void OLED_ScrollDisplay(u8 num,u8 space)
{
	u8 i,n,t=0,m=0,r;
	while(1)
	{
		if(m==0)
		{
	    OLED_ShowChinese(128,8,t,16); //��??????????????OLED_GRAM[][]??????
			t++;
		}
		if(t==num)
			{
				for(r=0;r<16*space;r++)      //??????
				 {
					for(i=0;i<144;i++)
						{
							for(n=0;n<4;n++)
							{
								OLED_GRAM[i-1][n]=OLED_GRAM[i][n];
							}
						}
           OLED_Refresh();
				 }
        t=0;
      }
		m++;
		if(m==16){m=0;}
		for(i=0;i<144;i++)   //???????
		{
			for(n=0;n<4;n++)
			{
				OLED_GRAM[i-1][n]=OLED_GRAM[i][n];
			}
		}
		OLED_Refresh();
	}
}

//????��??????????��??
void OLED_WR_BP(u8 x,u8 y)
{
	OLED_WR_Byte(0xb0+y,OLED_CMD);//????????????
	OLED_WR_Byte(((x&0xf0)>>4)|0x10,OLED_CMD);
	OLED_WR_Byte((x&0x0f)|0x01,OLED_CMD);
}

//x0,y0?????????
//x1,y1?????????
//BMP[]???��?????????
void OLED_ShowPicture(u8 x0,u8 y0,u8 x1,u8 y1,u8 BMP[])
{
	u32 j=0;
	u8 x=0,y=0;
	for(y=y0;y<y1;y++)
	 {
		 OLED_WR_BP(x0,y);
		 for(x=x0;x<x1;x++)
		 {
			 OLED_WR_Byte(BMP[j],OLED_DATA);
			 j++;
     }
	 }
}

void OLED_Init(void)
{
    /* ???GPIOB??? */
    rcu_periph_clock_enable(RCU_GPIOB);
    
    /* ????SCL??SDA???? */
    gpio_mode_set(OLED_SCL_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, OLED_SCL_PIN);
    gpio_output_options_set(OLED_SCL_PORT, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, OLED_SCL_PIN);
    OLED_SCLK_Set();
    gpio_mode_set(OLED_SDA_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, OLED_SDA_PIN);
    gpio_output_options_set(OLED_SDA_PORT, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, OLED_SDA_PIN);
    OLED_SDIN_Set();
   
	
	delay_1ms(500);
	
	OLED_WR_Byte(0xAE,OLED_CMD);//--turn off oled panel
	OLED_WR_Byte(0x00,OLED_CMD);//---set low column address
	OLED_WR_Byte(0x10,OLED_CMD);//---set high column address
	OLED_WR_Byte(0x40,OLED_CMD);//--set start line address  Set Mapping RAM Display Start Line (0x00~0x3F)
	OLED_WR_Byte(0x81,OLED_CMD);//--set contrast control register
	OLED_WR_Byte(0xCF,OLED_CMD);// Set SEG Output Current Brightness
	OLED_WR_Byte(0xA1,OLED_CMD);//--Set SEG/Column Mapping     0xa0??????? 0xa1????
	OLED_WR_Byte(0xC8,OLED_CMD);//Set COM/Row Scan Direction   0xc0???��??? 0xc8????
	OLED_WR_Byte(0xA6,OLED_CMD);//--set normal display
	OLED_WR_Byte(0xA8,OLED_CMD);//--set multiplex ratio(1 to 64)
	OLED_WR_Byte(0x1f,OLED_CMD);//--1/64 duty
	OLED_WR_Byte(0xD3,OLED_CMD);//-set display offset	Shift Mapping RAM Counter (0x00~0x3F)
	OLED_WR_Byte(0x00,OLED_CMD);//-not offset
	OLED_WR_Byte(0xd5,OLED_CMD);//--set display clock divide ratio/oscillator frequency
	OLED_WR_Byte(0x80,OLED_CMD);//--set divide ratio, Set Clock as 100 Frames/Sec
	OLED_WR_Byte(0xD9,OLED_CMD);//--set pre-charge period
	OLED_WR_Byte(0xF1,OLED_CMD);//Set Pre-Charge as 15 Clocks & Discharge as 1 Clock
	OLED_WR_Byte(0xDA,OLED_CMD);//--set com pins hardware configuration
	OLED_WR_Byte(0x00,OLED_CMD);
	OLED_WR_Byte(0xDB,OLED_CMD);//--set vcomh
	OLED_WR_Byte(0x40,OLED_CMD);//Set VCOM Deselect Level
	OLED_WR_Byte(0x20,OLED_CMD);//-Set Page Addressing Mode (0x00/0x01/0x02)
	OLED_WR_Byte(0x02,OLED_CMD);//
	OLED_WR_Byte(0x8D,OLED_CMD);//--set Charge Pump enable/disable
	OLED_WR_Byte(0x14,OLED_CMD);//--set(0x10) disable
	OLED_WR_Byte(0xA4,OLED_CMD);// Disable Entire Display On (0xa4/0xa5)
	OLED_WR_Byte(0xA6,OLED_CMD);// Disable Inverse Display On (0xa6/a7) 
	OLED_WR_Byte(0xAF,OLED_CMD);
	OLED_Clear();
}

/**
  * 函    数：OLED显示浮点数字（十进制，小数）
  * 参    数：X 指定数字左上角的横坐标，范围：-32768~32767，屏幕区域：0~127
  * 参    数：Y 指定数字左上角的纵坐标，范围：-32768~32767，屏幕区域：0~63
  * 参    数：Number 指定要显示的数字，范围：-4294967295.0~4294967295.0
  * 参    数：IntLength 指定数字的整数位长度，范围：0~10
  * 参    数：FraLength 指定数字的小数位长度，范围：0~9，小数进行四舍五入显示
  * 参    数：FontSize 指定字体大小，范围：12/16
  * 返 回 值：无
  * 说    明：调用此函数后，要想真正地呈现在屏幕上，还需调用更新函数
  */
void OLED_ShowFloatNum(int16_t X, int16_t Y, double Number, uint8_t IntLength, uint8_t FraLength, uint8_t FontSize)
{
	uint32_t PowNum, IntNum, FraNum;
	
	if (Number < 0)						//数字大于等于0
	{
			OLED_ShowChar(X, Y, '-', FontSize);	//显示-号
			Number = -Number;					//Number取负
	}

	/*提取整数部分和小数部分*/
	IntNum = Number;						//直接赋值给整型变量，提取整数
	Number -= IntNum;						//将Number的整数减掉，防止之后将小数乘到整数时因数过大造成错误
	PowNum = OLED_Pow(10, FraLength);		//根据指定小数的位数，确定乘数
	FraNum = round(Number * PowNum);		//将小数乘到整数，同时四舍五入，避免显示误差
	IntNum += FraNum / PowNum;				//若四舍五入造成了进位，则需要再加给整数
	
	/*显示整数部分*/
	OLED_ShowNum(X, Y, IntNum, IntLength, FontSize);
	
	/*显示小数点*/
	OLED_ShowChar(X + (IntLength + 1) * FontSize, Y, '.', FontSize);
	
	/*显示小数部分*/
	OLED_ShowNum(X + (IntLength + 2) * FontSize, Y, FraNum, FraLength, FontSize);
}

void OLED_Printf(int16_t X, int16_t Y, uint8_t FontSize, char *format, ...)
{
	char String[256];						//定义字符数组
	va_list arg;							//定义可变参数列表数据类型的变量arg
	va_start(arg, format);					//从format开始，接收参数列表到arg变量
	vsprintf(String, format, arg);			//使用vsprintf打印格式化字符串和参数列表到字符数组中
	va_end(arg);							//结束变量arg
	OLED_ShowString(X, Y, (u8 *)String, FontSize);//OLED显示字符数组（字符串）
}
