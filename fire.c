#include <string.h>
#include <stdlib.h>
#include <libsys/vga.h>

#define COLOR_FIRE  175
#define FIRE_RANGE 16

#define WIDTH (VGA_COLS / 2)
#define HEIGHT (VGA_ROWS + 1)

#define VPIXELS(col, row) (vpixels[(row) * WIDTH + (col)])

const uint8_t vpixels_rows = HEIGHT;
const size_t vpixels_size = vpixels_rows * WIDTH;
uint8_t vpixels[WIDTH * HEIGHT];

void step(void) {
    int i = 0;
    uint16_t sum = vpixels[WIDTH] + vpixels[WIDTH - 1]; // sum of middle and right from last iteration
    for (; i != WIDTH * (HEIGHT - 1); i++) {
        const uint8_t right = vpixels[i + WIDTH + 1];
        const uint8_t down = vpixels[i + WIDTH + WIDTH];
        const uint8_t left = vpixels[i + WIDTH - 1];
        sum += right;
        vpixels[i] = (uint8_t)(sum + down) >> 2;
        sum -= left;
    }
    for (; i != WIDTH * HEIGHT; i++) {
        if (rand() % 8 <= 2)
            vpixels[i] = 63;
        else
            vpixels[i] = 0;
    }
}

void    vpixels_render(void)
{
    uint8_t *vpixel = vpixels;
    const int pixel_offset = VGA_OFFSET(0,1) - VGA_OFFSET(VGA_COLS,0);
    uint8_t *pixel = VGA_CHAR_SEG;
    for (int row = 0; row != VGA_ROWS; row += 1)
    {
        for (int col = 0; col < WIDTH; col++)
        {
            const uint8_t color = *vpixel >> 4;
            *pixel = color + COLOR_FIRE;
            pixel++;
            *pixel = color + COLOR_FIRE;
            pixel++;
            vpixel++;
        }
        pixel += pixel_offset;
    }
}

void main(void) {
    vga_clear(COLOR(COLOR_YELLOW, COLOR_BLACK));
    memset(vpixels, 1, vpixels_size);
    vpixels_render();
    while (1)
    {
        step();
        vpixels_render();
    }
}

int getchar(void) { return 0; }
int putchar(int c) { return 0; }
