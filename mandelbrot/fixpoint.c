#include "fixpoint.h"
#include <stdbool.h>

fp fp_umul(fp a, fp b);

fp fp_mul(fp a, fp b) {
    bool negate_result = false;
    if (a < 0) {
        a = -a;
        negate_result = true;
    }
    if (b < 0) {
        b = -b;
        negate_result = !negate_result;
    }
    fp r = fp_umul(a, b);
    if (negate_result) {
        r = -r;
    }
    return r;
}
