#include "Key.h"

/*---------------------------------------------变量区------------------------------------------------*/


uint8_t g_key_stable_level[KEY_COUNT] = {KEY_UNPRESSED};
uint8_t Key_flag[KEY_COUNT];
int16_t CalcCount=0;
int16_t CalcMode=0;
uint8_t LED_Mode =0;
int16_t Count[3]={0};
uint8_t prev_real_level[KEY_COUNT] = {KEY_UNPRESSED}; // 存储上一次原始采样电平

/*---------------------------------------按键部分（全功能非阻塞）------------------------------------------------*/
void Key_Init(void)
{
	/* 使能GPIOE时钟 */
	rcu_periph_clock_enable(RCU_GPIOE);
	//上拉输入
	gpio_mode_set(GPIOE, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, GPIO_PIN_15 | GPIO_PIN_11 | GPIO_PIN_9 | GPIO_PIN_7);
}

void Key_Debounce_Tick(void)
{
	static uint8_t conti_cnt[KEY_COUNT] = {0}; // 连续稳定计数
	static const uint32_t key_pins[KEY_COUNT] = {GPIO_PIN_15, GPIO_PIN_11, GPIO_PIN_9, GPIO_PIN_7};
	uint8_t real_level[KEY_COUNT];

	for (uint8_t i = 0; i < KEY_COUNT; i++)
	{
		// 双重采样
		uint8_t t1 = gpio_input_bit_get(GPIOE, key_pins[i]);
		uint8_t t2 = gpio_input_bit_get(GPIOE, key_pins[i]);

		real_level[i] = (t1 == t2 && t1 == 0) ? KEY_PRESSED : KEY_UNPRESSED;

		if (real_level[i] == prev_real_level[i])
		{
			if (conti_cnt[i] < DEBOUNCE_CNT)
			{
				conti_cnt[i]++;
			}
		}
		else
		{
			conti_cnt[i] = 0;
		}

		if (conti_cnt[i] >= DEBOUNCE_CNT)
		{
			g_key_stable_level[i] = real_level[i];
			conti_cnt[i] = DEBOUNCE_CNT;
		}
		prev_real_level[i] = real_level[i];
	}
}

uint8_t Key_GetState(uint8_t n)
{
	return g_key_stable_level[n];
}

//外部调用核心函数
uint8_t Key_Check(uint8_t n,uint8_t Flag)
{
	if(Key_flag[n] & Flag)
	{
		if(Flag!=KEY_HOLD)
		{
			Key_flag[n] &= ~Flag;
		}

		return 1;
	}
	return 0;
}


void key_Check_Tick(void)//按键核心控制
{
	static uint8_t count=0,i;
	static uint8_t CurrState[KEY_COUNT],PrevState[KEY_COUNT];
	static uint8_t s[KEY_COUNT];
	static uint16_t Time[KEY_COUNT];
	static uint8_t Last_Scan_StableLevel[KEY_COUNT] = {KEY_UNPRESSED};	
	// 上一次扫描的稳定电平”（确保边沿检测基于两次30ms扫描的稳定值）
	count++;
	for(i=0;i<KEY_COUNT;i++)
	{
		if(Time[i]>0)
			Time[i]--;
	}
	if(count>=30) // 每30ms扫描一次
	{
		count = 0;
		for(i=0;i<KEY_COUNT;i++)
		{
				Key_flag[i] &= ~(KEY_DOWN | KEY_UP);	//清零单次边沿标志
				PrevState[i] = Last_Scan_StableLevel[i];// CurrState当前稳定电平，PrevState上一次扫描的稳定电平
				CurrState[i] = Key_GetState(i); 		// 防抖后的稳定电平
				Last_Scan_StableLevel[i] = CurrState[i]; // 更新“上一次扫描的稳定电平”

			// 持续按下
			if(CurrState[i]==KEY_PRESSED)
			{
				Key_flag[i]|=KEY_HOLD;
			}
			else
			{
				Key_flag[i]&=~KEY_HOLD;
			}
			
			// 边沿检测必须满足“两次30ms扫描的稳定电平一致”
			// 下降沿：上一次扫描稳定松开 → 当前扫描稳定按下
			if(CurrState[i]==KEY_PRESSED && PrevState[i]==KEY_UNPRESSED)
			{
				Key_flag[i]|=KEY_DOWN;
			}
			
			// 上升沿：上一次扫描稳定按下 → 当前扫描稳定松开
			if(CurrState[i]==KEY_UNPRESSED && PrevState[i]==KEY_PRESSED)
			{
				Key_flag[i]|=KEY_UP;
			}			
			switch(s[i])
			{
				case 0:
					if(CurrState[i]==KEY_PRESSED)
					{
						Time[i] =KEY_TIME_LONG;
						s[i]=1;
					}
				break;
				case 1:
					if(CurrState[i]==KEY_UNPRESSED)
					{
						Time[i] =KEY_TIME_DOUBLE;
						s[i]=2;
					}
					else if(Time[i]==0)
					{
						Time[i]=KEY_TIME_REPEAT;
						s[i]=4;
						Key_flag[i]|=KEY_LONG;
					}
				break;
				case 2:
					if(CurrState[i]==KEY_PRESSED)
					{
						s[i]=3;
						Key_flag[i]|=KEY_DOUBLE;
					}
					else if(Time[i] ==0)
					{
						s[i]=0;
						Key_flag[i]|=KEY_SINGLE;
					}
				break;
				case 3:
					if(CurrState[i]==KEY_UNPRESSED)
					{
						s[i]=0;
					}
				break;
				case 4:
					if(CurrState[i]==KEY_UNPRESSED)
					{
						s[i]=0;	
					}
					else if(Time[i]==0)
					{
						Time[i]=KEY_TIME_REPEAT;
						Key_flag[i]|=KEY_REPEAT;
						s[i]=4;
					}
				
			}
		}
	}
}

void Key_Tick(void)
{
	Key_Debounce_Tick();
	key_Check_Tick();
}




