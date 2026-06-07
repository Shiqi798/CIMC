#include "AD3344.h"
#include "systick.h"

#define AD3344_USE_SPI4 0

#if AD3344_USE_SPI4
#define AD3344_SPI SPI4
#define AD3344_SPI_CLK RCU_SPI4
#define AD3344_SPI_AF GPIO_AF_6
#else
#define AD3344_SPI SPI3
#define AD3344_SPI_CLK RCU_SPI3
#define AD3344_SPI_AF GPIO_AF_5
#endif

#define AD3344_GPIO_CLK RCU_GPIOE
#define AD3344_GPIO GPIOE
#define AD3344_CS_PIN GPIO_PIN_9
#define AD3344_SCK_PIN GPIO_PIN_12
#define AD3344_MISO_PIN GPIO_PIN_13
#define AD3344_MOSI_PIN GPIO_PIN_14
#define AD3344_SPI_TIMEOUT 0x20000U
#define AD3344_ADC_FULL_SCALE 32768.0f
#define AD3344_SPI_PRESCALE SPI_PSC_32
#define AD3344_CS_SETUP_US 1000U
#define AD3344_CS_HOLD_US 1000U
#define AD3344_ENABLE_AIN3_EXT_REF 1
#define AD3344_REFERENCE_BASE 2.048f
#define AD3344_DEFAULT_REFERENCE_VOLTAGE 2.004f

#define AD3344_CS_LOW() gpio_bit_reset(AD3344_GPIO, AD3344_CS_PIN)
#define AD3344_CS_HIGH() gpio_bit_set(AD3344_GPIO, AD3344_CS_PIN)
// 初始化参数
static ad3344_config_t g_ad3344_config = {
    AD3344_MUX_DIFF_0_1,
    AD3344_PGA_1_024V,
    AD3344_DR_1000SPS,
    AD3344_MODE_SINGLE};

static uint16_t g_ad3344_config_word = AD3344_DEFAULT_CONFIG;
static float g_ad3344_reference_voltage = AD3344_DEFAULT_REFERENCE_VOLTAGE;

/*********************************私有函数*************************************** */
static void ad3344_delay_us(uint32_t us)
{
    uint16_t i;

    while (us-- > 0U)
    {
        i = 10U;
        while (i-- > 0U)
        {
        }
    }
}

// 拼配置字并更新缓存
static uint16_t ad3344_make_config(ad3344_mux_t mux, ad3344_pga_t pga,
                                   ad3344_data_rate_t data_rate, ad3344_mode_t mode, uint16_t os)
{
    g_ad3344_config_word = (uint16_t)(os | (uint16_t)mux | (uint16_t)pga | (uint16_t)mode | (uint16_t)data_rate |
                                      AD3344_CONFIG_PULL_UP_EN | AD3344_CONFIG_NOP_VALID | AD3344_CONFIG_RESERVED);
    return g_ad3344_config_word;
}

static uint16_t ad3344_transfer16(uint16_t data)
{
    uint32_t timeout = AD3344_SPI_TIMEOUT;

    while (RESET == spi_i2s_flag_get(AD3344_SPI, SPI_FLAG_TBE))
    {
        if (timeout-- == 0U)
        {
            return 0xFFFFU;
        }
    }
    spi_i2s_data_transmit(AD3344_SPI, data);

    timeout = AD3344_SPI_TIMEOUT;
    while (RESET == spi_i2s_flag_get(AD3344_SPI, SPI_FLAG_RBNE))
    {
        if (timeout-- == 0U)
        {
            return 0xFFFFU;
        }
    }
    return (uint16_t)spi_i2s_data_receive(AD3344_SPI);
}

// 等待spi发送完成
static void ad3344_wait_spi_idle(void)
{
    uint32_t timeout = AD3344_SPI_TIMEOUT;

    while (SET == spi_i2s_flag_get(AD3344_SPI, SPI_FLAG_TRANS))
    {
        if (timeout-- == 0U)
        {
            break;
        }
    }
}
//

/*/spi发送核心*/
static uint16_t ad3344_write16(uint16_t data)
{
    uint16_t rx;

    AD3344_CS_LOW();
    ad3344_delay_us(AD3344_CS_SETUP_US);
    rx = ad3344_transfer16(data);
    ad3344_wait_spi_idle();
    AD3344_CS_HIGH();
    ad3344_delay_us(AD3344_CS_HOLD_US);

    return rx;
}

// 写ad3344寄存器
static void ad3344_write_private_register(uint16_t addr, uint16_t value)
{
    AD3344_CS_LOW();
    ad3344_delay_us(AD3344_CS_SETUP_US);
    (void)ad3344_transfer16(0x8100U);
    (void)ad3344_transfer16(addr);
    (void)ad3344_transfer16(value);
    ad3344_wait_spi_idle();
    AD3344_CS_HIGH();
    ad3344_delay_us(AD3344_CS_HOLD_US);
}
static uint16_t ad3344_read_private_register(uint16_t addr)
{
    uint16_t value;

    AD3344_CS_LOW();
    ad3344_delay_us(AD3344_CS_SETUP_US);
    (void)ad3344_transfer16(0x8106U);
    ad3344_delay_us(AD3344_CS_SETUP_US);
    (void)ad3344_transfer16(addr);
    ad3344_delay_us(AD3344_CS_SETUP_US);
    value = ad3344_transfer16(0x0000U);
    ad3344_wait_spi_idle();
    AD3344_CS_HIGH();
    ad3344_delay_us(AD3344_CS_HOLD_US);

    return value;
}

// 初始化芯片内部私有寄存器
static void ad3344_init_private_registers(void)
{
    uint16_t value;

    ad3344_write_private_register(0x0012U, 0xACCAU);
    value = ad3344_read_private_register(0x0014U);
    value = (uint16_t)(value | 0x0040U);
    ad3344_write_private_register(0x0014U, value);
}
// 根据数据速率给出转换等待时间，速率越低等待越长
static uint16_t ad3344_conversion_delay_ms(ad3344_data_rate_t data_rate)
{
    switch (data_rate)
    {
    case AD3344_DR_6_25SPS:
        return 162U;
    case AD3344_DR_12_5SPS:
        return 82U;
    case AD3344_DR_25SPS:
        return 42U;
    case AD3344_DR_50SPS:
        return 22U;
    case AD3344_DR_250SPS:
        return 6U;
    case AD3344_DR_500SPS:
        return 4U;
    case AD3344_DR_1000SPS:
        return 3U;
    case AD3344_DR_100SPS:
    default:
        return 12U;
    }
}

// 量程增益*参考电压=满量程电压。
static float ad3344_effective_full_scale(ad3344_pga_t pga)
{
    float full_scale;

    switch (pga)
    {
    case AD3344_PGA_6_144V:
        full_scale = 6.144f;
        break;

    case AD3344_PGA_4_096V:
        full_scale = 4.096f;
        break;
    case AD3344_PGA_1_024V:
        full_scale = 1.024f;
        break;
    case AD3344_PGA_0_512V:
        full_scale = 0.512f;
        break;
    case AD3344_PGA_0_256V:
        full_scale = 0.256f;
        break;
    case AD3344_PGA_0_064V:
        full_scale = 0.064f;
        break;
    case AD3344_PGA_2_048V:
    default:
        full_scale = 2.048f;
        break;
    }

    return (full_scale * g_ad3344_reference_voltage) / AD3344_REFERENCE_BASE;
}

static void ad3344_wait_ready(ad3344_data_rate_t data_rate)
{
    uint16_t timeout = (uint16_t)(ad3344_conversion_delay_ms(data_rate) + 5U);

    while (timeout-- > 0U)
    {
        if (RESET == gpio_input_bit_get(AD3344_GPIO, AD3344_MISO_PIN))
        {
            return;
        }
        delay_1ms(1U);
    }
}

void AD3344_Init(void)
{
    rcu_periph_clock_enable(AD3344_GPIO_CLK);
    rcu_periph_clock_enable(AD3344_SPI_CLK);
    // GPIO
    gpio_af_set(AD3344_GPIO, AD3344_SPI_AF, AD3344_SCK_PIN | AD3344_MISO_PIN | AD3344_MOSI_PIN);
    gpio_mode_set(AD3344_GPIO, GPIO_MODE_AF, GPIO_PUPD_NONE, AD3344_SCK_PIN | AD3344_MOSI_PIN);
    gpio_output_options_set(AD3344_GPIO, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, AD3344_SCK_PIN | AD3344_MOSI_PIN);
    gpio_mode_set(AD3344_GPIO, GPIO_MODE_AF, GPIO_PUPD_PULLUP, AD3344_MISO_PIN);
    gpio_output_options_set(AD3344_GPIO, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, AD3344_MISO_PIN);
    gpio_mode_set(AD3344_GPIO, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, AD3344_CS_PIN);
    gpio_output_options_set(AD3344_GPIO, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, AD3344_CS_PIN);
    AD3344_CS_HIGH();

    // SPI
    spi_parameter_struct spi_init_struct;
    spi_i2s_deinit(AD3344_SPI);
    spi_struct_para_init(&spi_init_struct);
    spi_init_struct.trans_mode = SPI_TRANSMODE_FULLDUPLEX;
    spi_init_struct.device_mode = SPI_MASTER;
    spi_init_struct.frame_size = SPI_FRAMESIZE_16BIT;
    spi_init_struct.clock_polarity_phase = SPI_CK_PL_LOW_PH_2EDGE;
    spi_init_struct.nss = SPI_NSS_SOFT;
    spi_init_struct.prescale = AD3344_SPI_PRESCALE;
    spi_init_struct.endian = SPI_ENDIAN_MSB;
    spi_init(AD3344_SPI, &spi_init_struct);
    spi_enable(AD3344_SPI);

#if AD3344_ENABLE_AIN3_EXT_REF
    ad3344_init_private_registers();
#endif
    //
    AD3344_Config(g_ad3344_config.mux, g_ad3344_config.pga,
                  g_ad3344_config.data_rate, g_ad3344_config.mode);
}

void AD3344_DeInit(void)
{
    AD3344_CS_HIGH();
    spi_disable(AD3344_SPI);
    spi_i2s_deinit(AD3344_SPI);
}
///////////////////////////配置参数设置/////////////////////////////////////
void AD3344_Config(ad3344_mux_t mux, ad3344_pga_t pga, ad3344_data_rate_t data_rate, ad3344_mode_t mode)
{
    uint16_t os = 0U;

    g_ad3344_config.mux = mux;
    g_ad3344_config.pga = pga;
    g_ad3344_config.data_rate = data_rate;
    g_ad3344_config.mode = mode;

    if (mode == AD3344_MODE_SINGLE)
    {
        os = AD3344_CONFIG_OS_SINGLE;
    }
    ad3344_make_config(g_ad3344_config.mux, g_ad3344_config.pga, g_ad3344_config.data_rate,
                       g_ad3344_config.mode, os);

    ad3344_write16(g_ad3344_config_word);
    //    delay_1ms(2);
    delay_1ms(ad3344_conversion_delay_ms(data_rate));

    if (mode == AD3344_MODE_CONTINUOUS)
    {
        AD3344_ReadRaw();
        delay_1ms(ad3344_conversion_delay_ms(data_rate));
        AD3344_ReadRaw();
        delay_1ms(ad3344_conversion_delay_ms(data_rate));
    }
}

void AD3344_SetMux(ad3344_mux_t mux)
{
    AD3344_Config(mux, g_ad3344_config.pga, g_ad3344_config.data_rate, g_ad3344_config.mode);
}

void AD3344_SetPga(ad3344_pga_t pga)
{
    AD3344_Config(g_ad3344_config.mux, pga, g_ad3344_config.data_rate, g_ad3344_config.mode);
}

void AD3344_SetDataRate(ad3344_data_rate_t data_rate)
{
    AD3344_Config(g_ad3344_config.mux, g_ad3344_config.pga, data_rate, g_ad3344_config.mode);
}

void AD3344_SetMode(ad3344_mode_t mode)
{
    AD3344_Config(g_ad3344_config.mux, g_ad3344_config.pga, g_ad3344_config.data_rate, mode);
}

// 配置参数获取
ad3344_config_t AD3344_GetState(void)
{
    return g_ad3344_config;
}

uint16_t AD3344_GetConfig(void)
{
    return g_ad3344_config_word;
}

///////参考电压设置
void AD3344_SetReferenceVoltage(float reference_voltage)
{
    if (reference_voltage > 0.0f)
    {
        g_ad3344_reference_voltage = reference_voltage;
    }
}
float AD3344_GetReferenceVoltage(void)
{
    return g_ad3344_reference_voltage;
}

// 读取原始值/电压值
int16_t AD3344_ReadRaw(void)
{
    if (g_ad3344_config.mode == AD3344_MODE_SINGLE)
    {
        ad3344_make_config(g_ad3344_config.mux, g_ad3344_config.pga, g_ad3344_config.data_rate,
                           g_ad3344_config.mode, AD3344_CONFIG_OS_SINGLE);
        ad3344_write16(g_ad3344_config_word);
        ad3344_wait_ready(g_ad3344_config.data_rate);
    }

    return (int16_t)ad3344_write16(g_ad3344_config_word);
}

ad3344_sample_t AD3344_ReadSample(void)
{
    ad3344_sample_t sample;
    sample.raw = AD3344_ReadRaw();
    sample.voltage = AD3344_RawToVoltage(sample.raw, g_ad3344_config.pga);
    return sample;
}

uint8_t AD3344_ReadVoltage(float *voltage)
{
    int16_t raw;
    if (voltage == 0)
    {
        return 0U;
    }
    raw = AD3344_ReadRaw();
    *voltage = AD3344_RawToVoltage(raw, g_ad3344_config.pga);
    return 1U;
}

int16_t AD3344_ReadSingleRaw(ad3344_mux_t mux)
{
    AD3344_Config(mux, g_ad3344_config.pga, g_ad3344_config.data_rate, AD3344_MODE_SINGLE);
    return AD3344_ReadRaw();
}

uint8_t AD3344_ReadSingleVoltage(ad3344_mux_t mux, float *voltage)
{
    int16_t raw;

    if (voltage == 0)
    {
        return 0U;
    }

    raw = AD3344_ReadSingleRaw(mux);
    *voltage = AD3344_RawToVoltage(raw, g_ad3344_config.pga);
    return 1U;
}

float AD3344_RawToVoltage(int16_t raw, ad3344_pga_t pga)
{
    return ((float)raw * ad3344_effective_full_scale(pga)) / AD3344_ADC_FULL_SCALE;
}

// 连续采5次后求平均并输出
void AD3344_cmd(void)
{
    ad3344_sample_t ad3344_sample;
    uint8_t sample;
    uint16_t extref_reg;

    AD3344_Config(AD3344_MUX_DIFF_0_1, AD3344_PGA_4_096V, AD3344_DR_1000SPS,
                  AD3344_MODE_CONTINUOUS);
 //   AD3344_ReadSample();
//    AD3344_ReadSample();
    delay_1ms(5);
    extref_reg = ad3344_read_private_register(0x0014U);
    printf("AD3344 cfg=0x%04X ref=%.3fV extref_reg=0x%04X\r\n", AD3344_GetConfig(), AD3344_GetReferenceVoltage(), extref_reg);

    //    printf("AD3344 DIFF AIN0-AIN1 cfg=0x%04X ref=%.3fV continuous 500SPS\r\n",
    //           AD3344_GetConfig(), AD3344_GetReferenceVoltage());
/*    for (sample = 0U; sample < 5U; sample++)
    {
        ad3344_sample = AD3344_ReadSample();
        raw_sum += ad3344_sample.raw;
        voltage_sum += ad3344_sample.voltage;
    }
    raw_avg = (int16_t)(raw_sum / 5);
    voltage_avg = voltage_sum / 5.0f;
    printf("AD3344 avg(5) raw=%d hex=0x%04X voltage=%.6f V\r\n",
           raw_avg, (uint16_t)raw_avg, voltage_avg);
*/
    /*    printf("AD3344 voltage=%.6f V\r\n",voltage_avg);*/

    for (sample = 0U; sample < 5U; sample++)
    {
        ad3344_sample = AD3344_ReadSample();
        printf("AD3344 sample %u: raw=%d hex=0x%04X voltage=%.6f V\r\n", sample + 1, ad3344_sample.raw, (uint16_t)ad3344_sample.raw, ad3344_sample.voltage);
    }
}
