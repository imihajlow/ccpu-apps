#include "fpmath/fixpoint.h"

fp32 fooa, foob, foor;

void main(void) {
    foor = fp32_28_mul(fooa, foob);
}

int getchar(void) { return 0; }
int putchar(int c) { return 0; }
