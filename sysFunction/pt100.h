#ifndef __PT100_H
#define __PT100_H

#include "HeaderFiles.h"

#define PT100_ALGO_TABLE       0U
#define PT100_ALGO_QUADRATIC   1U
#define PT100_ALGO_PIECEWISE_QUADRATIC 2U

#ifndef PT100_TEMP_ALGO
#define PT100_TEMP_ALGO        PT100_ALGO_PIECEWISE_QUADRATIC
#endif

#define PT100_RES_CAL_LINEAR   0U
#define PT100_RES_CAL_TABLE    1U

#ifndef PT100_RES_CAL_MODE
#define PT100_RES_CAL_MODE     PT100_RES_CAL_TABLE
#endif

#define PT100_FILTER_AVERAGE       0U
#define PT100_FILTER_TRIMMED_MEAN  1U
#define PT100_FILTER_MEDIAN        2U
#define PT100_FILTER_MID_AVERAGE   3U

#ifndef PT100_FILTER_MODE
#define PT100_FILTER_MODE          PT100_FILTER_MID_AVERAGE
#endif

#define PT100_DEFAULT_MUX      AD3344_MUX_DIFF_0_1
#define PT100_DEFAULT_PGA      AD3344_PGA_4_096V
#define PT100_DEFAULT_DR       AD3344_DR_1000SPS
#define PT100_DEFAULT_MODE     AD3344_MODE_CONTINUOUS
#define PT100_SAMPLE_COUNT     5U
#define PT100_DISCARD_COUNT    1U
#define PT100_SAMPLE_GAP_MS    1U

#define PT100_VOLT_TO_RES_K    789.62198f
#define PT100_VOLT_TO_RES_B    0.15362f
#define PT100_RES_SNAP_OHM     0.12f

typedef struct {
    float resistance;
    float temperature;
} pt100_temp_point_t;

typedef struct {
    float voltage;
    float resistance;
} pt100_res_point_t;

uint8_t PT100_Read(float *res_ohm, float *temp_c);
uint8_t PT100_ReadVoltage(float *voltage);
float PT100_VoltageToResistance(float voltage);
float PT100_ResistanceToTemp(float res_ohm);
const char *PT100_GetAlgoName(void);

#endif
