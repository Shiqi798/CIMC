#include "pt100.h"

#if (PT100_SAMPLE_COUNT < 1U)
#error "PT100_SAMPLE_COUNT must be at least 1"
#endif

static const pt100_temp_point_t g_pt100_temp_table[] = {
    {80.60f, -49.27f},
    {82.50f, -44.49f},
    {100.00f, 0.00f},
    {107.00f, 18.00f},
    {113.00f, 33.44f},
    {115.00f, 38.61f},
    {124.00f, 62.00f},
    {130.00f, 78.00f},
    {137.00f, 96.00f},
    {150.00f, 130.54f},
    {154.00f, 141.11f},
};

static const pt100_res_point_t g_pt100_res_table[] = {
    {0.1018880f, 80.60f},
    {0.1040408f, 82.50f},
    {0.1267180f, 100.00f},
    {0.1353776f, 107.00f},
    {0.1429614f, 113.00f},
    {0.1454320f, 115.00f},
    {0.1569290f, 124.00f},
    {0.1642680f, 130.00f},
    {0.1733200f, 137.00f},
    {0.1897100f, 150.00f},
    {0.1948470f, 154.00f},
};

static float pt100_temp_table_interp(float res_ohm)
{
    uint16_t i;
    const uint16_t count = (uint16_t)(sizeof(g_pt100_temp_table) / sizeof(g_pt100_temp_table[0]));

    if (res_ohm <= g_pt100_temp_table[0].resistance) {
        return g_pt100_temp_table[0].temperature;
    }

    if (res_ohm >= g_pt100_temp_table[count - 1U].resistance) {
        return g_pt100_temp_table[count - 1U].temperature;
    }

    for (i = 0U; i < (uint16_t)(count - 1U); i++) {
        const pt100_temp_point_t *left = &g_pt100_temp_table[i];
        const pt100_temp_point_t *right = &g_pt100_temp_table[i + 1U];

        if ((res_ohm >= left->resistance) && (res_ohm <= right->resistance)) {
            float ratio = (res_ohm - left->resistance) / (right->resistance - left->resistance);
            return left->temperature + ratio * (right->temperature - left->temperature);
        }
    }

    return g_pt100_temp_table[count - 1U].temperature;
}

static float pt100_res_table_interp(float voltage)
{
    uint16_t i;
    const uint16_t count = (uint16_t)(sizeof(g_pt100_res_table) / sizeof(g_pt100_res_table[0]));

    if (voltage <= g_pt100_res_table[0].voltage) {
        return g_pt100_res_table[0].resistance;
    }

    if (voltage >= g_pt100_res_table[count - 1U].voltage) {
        return g_pt100_res_table[count - 1U].resistance;
    }

    for (i = 0U; i < (uint16_t)(count - 1U); i++) {
        const pt100_res_point_t *left = &g_pt100_res_table[i];
        const pt100_res_point_t *right = &g_pt100_res_table[i + 1U];

        if ((voltage >= left->voltage) && (voltage <= right->voltage)) {
            float ratio = (voltage - left->voltage) / (right->voltage - left->voltage);
            return left->resistance + ratio * (right->resistance - left->resistance);
        }
    }

    return g_pt100_res_table[count - 1U].resistance;
}

static float pt100_snap_resistance(float res_ohm)
{
    uint16_t i;
    const uint16_t count = (uint16_t)(sizeof(g_pt100_temp_table) / sizeof(g_pt100_temp_table[0]));

    for (i = 0U; i < count; i++) {
        float diff = res_ohm - g_pt100_temp_table[i].resistance;

        if (diff < 0.0f) {
            diff = -diff;
        }

        if (diff <= PT100_RES_SNAP_OHM) {
            return g_pt100_temp_table[i].resistance;
        }
    }

    return res_ohm;
}

static float pt100_quadratic_fit(float res_ohm)
{
    const float a = 7.80690197e-4f;
    const float b = 2.41190243f;
    const float c = -248.912194f;

    return (a * res_ohm * res_ohm) + (b * res_ohm) + c;
}

static float pt100_piecewise_quadratic_fit(float res_ohm)
{
    float a;
    float b;
    float c;

    if (res_ohm <= 100.00f) {
        a = 1.36578560e-3f;
        b = 2.29302984f;
        c = -242.960840f;
    } else if (res_ohm <= 113.00f) {
        a = 1.46520147e-4f;
        b = 2.54109890f;
        c = -255.575092f;
    } else if (res_ohm <= 124.00f) {
        a = 1.26262626e-3f;
        b = 2.29712121f;
        c = -242.257172f;
    } else if (res_ohm <= 137.00f) {
        a = -7.32600733e-3f;
        b = 4.52747253f;
        c = -386.761905f;
    } else {
        a = -8.48416290e-4f;
        b = 2.90041855f;
        c = -285.433416f;
    }

    return (a * res_ohm * res_ohm) + (b * res_ohm) + c;
}

static void pt100_sort_samples(float *samples, uint8_t count)
{
    uint8_t i;

    for (i = 1U; i < count; i++) {
        uint8_t j = i;
        float key = samples[i];

        while ((j > 0U) && (samples[j - 1U] > key)) {
            samples[j] = samples[j - 1U];
            j--;
        }

        samples[j] = key;
    }
}

static uint8_t pt100_read_voltage_filtered(float *voltage)
{
    uint8_t i;
    uint8_t discard;
    float sample;
    float sum = 0.0f;
    float samples[PT100_SAMPLE_COUNT];
    float filtered_voltage;

    if (voltage == NULL) {
        return 0U;
    }

    AD3344_Config(PT100_DEFAULT_MUX, PT100_DEFAULT_PGA, PT100_DEFAULT_DR, PT100_DEFAULT_MODE);

    for (discard = 0U; discard < PT100_DISCARD_COUNT; discard++) {
        if (AD3344_ReadVoltage(&sample) == 0U) {
            return 0U;
        }
#if (PT100_SAMPLE_GAP_MS > 0U)
        delay_1ms(PT100_SAMPLE_GAP_MS);
#endif
    }

    for (i = 0U; i < PT100_SAMPLE_COUNT; i++) {
        if (AD3344_ReadVoltage(&sample) == 0U) {
            return 0U;
        }

        samples[i] = sample;
        sum += sample;

#if (PT100_SAMPLE_GAP_MS > 0U)
        if (i < (uint8_t)(PT100_SAMPLE_COUNT - 1U)) {
            delay_1ms(PT100_SAMPLE_GAP_MS);
        }
#endif
    }

#if (PT100_FILTER_MODE == PT100_FILTER_MEDIAN)
    pt100_sort_samples(samples, PT100_SAMPLE_COUNT);
    filtered_voltage = samples[PT100_SAMPLE_COUNT / 2U];
#elif ((PT100_FILTER_MODE == PT100_FILTER_MID_AVERAGE) && (PT100_SAMPLE_COUNT >= 3U))
    pt100_sort_samples(samples, PT100_SAMPLE_COUNT);
    sum = 0.0f;
    for (i = 1U; i < (uint8_t)(PT100_SAMPLE_COUNT - 1U); i++) {
        sum += samples[i];
    }
    filtered_voltage = sum / (float)(PT100_SAMPLE_COUNT - 2U);
#elif ((PT100_FILTER_MODE == PT100_FILTER_TRIMMED_MEAN) && (PT100_SAMPLE_COUNT >= 3U))
    {
    float min_sample = samples[0];
    float max_sample = samples[0];

    for (i = 1U; i < PT100_SAMPLE_COUNT; i++) {
        if (samples[i] < min_sample) {
            min_sample = samples[i];
        }
        if (samples[i] > max_sample) {
            max_sample = samples[i];
        }
    }

    filtered_voltage = (sum - min_sample - max_sample) / (float)(PT100_SAMPLE_COUNT - 2U);
    }
#else
    filtered_voltage = sum / (float)PT100_SAMPLE_COUNT;
#endif

    *voltage = filtered_voltage;
    return 1U;
}

float PT100_VoltageToResistance(float voltage)
{
    float res_ohm;

    if (voltage < 0.0f) {
        voltage = 0.0f;
    }

#if (PT100_RES_CAL_MODE == PT100_RES_CAL_TABLE)
    res_ohm = pt100_res_table_interp(voltage);
#else
    res_ohm = (PT100_VOLT_TO_RES_K * voltage) + PT100_VOLT_TO_RES_B;
#endif

    return pt100_snap_resistance(res_ohm);
}

uint8_t PT100_ReadVoltage(float *voltage)
{
    return pt100_read_voltage_filtered(voltage);
}

float PT100_ResistanceToTemp(float res_ohm)
{
#if (PT100_TEMP_ALGO == PT100_ALGO_PIECEWISE_QUADRATIC)
    return pt100_piecewise_quadratic_fit(res_ohm);
#elif (PT100_TEMP_ALGO == PT100_ALGO_QUADRATIC)
    return pt100_quadratic_fit(res_ohm);
#else
    return pt100_temp_table_interp(res_ohm);
#endif
}

const char *PT100_GetAlgoName(void)
{
#if (PT100_TEMP_ALGO == PT100_ALGO_PIECEWISE_QUADRATIC)
    return "piecewise quadratic";
#elif (PT100_TEMP_ALGO == PT100_ALGO_QUADRATIC)
    return "quadratic";
#else
    return "table";
#endif
}

uint8_t PT100_Read(float *res_ohm, float *temp_c)
{
    float voltage;
    float resistance;

    if ((res_ohm == NULL) || (temp_c == NULL)) {
        return 0U;
    }

    if (pt100_read_voltage_filtered(&voltage) == 0U) {
        return 0U;
    }

    resistance = PT100_VoltageToResistance(voltage);
    *res_ohm = resistance;
    *temp_c = PT100_ResistanceToTemp(resistance);

    return 1U;
}
