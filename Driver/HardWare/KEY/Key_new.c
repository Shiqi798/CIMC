#include "Key.h"

/*---------------------------------------------变量区------------------------------------------------*/

uint8_t Key_flag[KEY_COUNT];
uint8_t LED_Mode = 0;
int16_t Count[3] = {0};

/*---------------------------------------按键部分（全功能非阻塞）------------------------------------------------*/
void Key_Init(void)
{
	/* 使能GPIOE时钟 */
	rcu_periph_clock_enable(RCU_GPIOE);
	//上拉输入
	gpio_mode_set(GPIOE, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, GPIO_PIN_15 | GPIO_PIN_11 | GPIO_PIN_9 | GPIO_PIN_13);
}

uint8_t Key_GetState(uint8_t n)
{
	static const uint32_t key_pins[KEY_COUNT] = {GPIO_PIN_15, GPIO_PIN_13, GPIO_PIN_11, GPIO_PIN_9};
	
	// 直接读取GPIO状态（0=按下，1=松开）
	if (gpio_input_bit_get(GPIOE, key_pins[n]) == 0)
	{
		return KEY_PRESSED;
	}
	return KEY_UNPRESSED;
}

uint8_t Key_Check(uint8_t n, uint8_t Flag)
{
	if (Key_flag[n] & Flag)
	{
		if (Flag != KEY_HOLD)
		{
			Key_flag[n] &= ~Flag;
		}
		return 1;
	}
	return 0;
}

void Key_Tick(void)
{
	static uint8_t count = 0, i;
	static uint8_t CurrState[KEY_COUNT], PrevState[KEY_COUNT];
	static uint8_t s[KEY_COUNT];
	static uint16_t Time[KEY_COUNT];
	
	// 倒计时递减
	for (i = 0; i < KEY_COUNT; i++)
	{
		if (Time[i] > 0)
		{
			Time[i]--;
		}
	}
	
	// 每20ms采样一次（10ms调用×2）
	count++;
	if (count >= 2)
	{
		count = 0;
		
		for (i = 0; i < KEY_COUNT; i++)
		{
			PrevState[i] = CurrState[i];
			CurrState[i] = Key_GetState(i);
			
			// 持续按下
			if (CurrState[i] == KEY_PRESSED)
			{
				Key_flag[i] |= KEY_HOLD;
			}
			else
			{
				Key_flag[i] &= ~KEY_HOLD;
			}
			
			// 下降沿（松开->按下）
			if (CurrState[i] == KEY_PRESSED && PrevState[i] == KEY_UNPRESSED)
			{
				Key_flag[i] |= KEY_DOWN;
			}
			
			// 上升沿（按下->松开）
			if (CurrState[i] == KEY_UNPRESSED && PrevState[i] == KEY_PRESSED)
			{
				Key_flag[i] |= KEY_UP;
			}
			
			// 核心状态机
			if (s[i] == 0)
			{
				if (CurrState[i] == KEY_PRESSED)
				{
					Time[i] = KEY_TIME_LONG;
					s[i] = 1;
				}
			}
			else if (s[i] == 1)
			{
				if (CurrState[i] == KEY_UNPRESSED)
				{
					Time[i] = KEY_TIME_DOUBLE;
					s[i] = 2;
				}
				else if (Time[i] == 0)
				{
					Time[i] = KEY_TIME_REPEAT;
					Key_flag[i] |= KEY_LONG;
					s[i] = 4;
				}
			}
			else if (s[i] == 2)
			{
				if (CurrState[i] == KEY_PRESSED)
				{
					Key_flag[i] |= KEY_DOUBLE;
					s[i] = 3;
				}
				else if (Time[i] == 0)
				{
					Key_flag[i] |= KEY_SINGLE;
					s[i] = 0;
				}
			}
			else if (s[i] == 3)
			{
				if (CurrState[i] == KEY_UNPRESSED)
				{
					s[i] = 0;
				}
			}
			else if (s[i] == 4)
			{
				if (CurrState[i] == KEY_UNPRESSED)
				{
					s[i] = 0;
				}
				else if (Time[i] == 0)
				{
					Time[i] = KEY_TIME_REPEAT;
					Key_flag[i] |= KEY_REPEAT;
					s[i] = 4;
				}
			}
		}
	}
}
