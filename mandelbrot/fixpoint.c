#include "fixpoint.h"
#include <stdbool.h>

fp32 fp32_28_umul(fp32 a, fp32 b);

fp32 fp32_28_mul(fp32 a, fp32 b) {
    bool negate_result = false;
    if (a < 0) {
        a = -a;
        negate_result = true;
    }
    if (b < 0) {
        b = -b;
        negate_result = !negate_result;
    }
    fp32 r = fp32_28_umul(a, b);
    if (negate_result) {
        r = -r;
    }
    return r;
}

fp16 fp16_12_umul(fp16 a, fp16 b);

fp16 fp16_12_mul(fp16 a, fp16 b) {
    bool negate_result = false;
    if (a < 0) {
        a = -a;
        negate_result = true;
    }
    if (b < 0) {
        b = -b;
        negate_result = !negate_result;
    }
    fp16 r = fp16_12_umul(a, b);
    if (negate_result) {
        r = -r;
    }
    return r;
}

