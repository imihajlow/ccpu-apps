#include "fixpoint.h"

fp16 fooa, foob, foor;

void main(void) {
    foor = fp16_12_mul(fooa, foob);
}

int getchar(void) { return 0; }
int putchar(int c) { return 0; }
