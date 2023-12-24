#include "fixpoint.h"

fp fooa, foob, foor;

void main(void) {
    foor = fp_mul(fooa, foob);
}

int getchar(void) { return 0; }
int putchar(int c) { return 0; }
