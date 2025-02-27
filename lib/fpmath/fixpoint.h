#pragma once
#include <stdint.h>

typedef int32_t fp32;
typedef int16_t fp16;

fp32 fp32_28_mul(fp32 a, fp32 b);
fp16 fp16_12_mul(fp16 a, fp16 b);

// Approximate reciprocal
fp16 fp16_12_rc(fp16 a);

// Sine. Byte range = full circle.
fp16 fp16_12_sin(int8_t phi);
fp16 fp16_12_cos(int8_t phi);
