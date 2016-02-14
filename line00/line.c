#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <avr/io.h>
#include <util/delay.h>

#define G 0
#define R 1
#define B 2

#define N_LED 60

/* save memory, make 1m 60 LED stripe feasible */
/* 16 bit 565 rgb a.k.a. hi-color */
#define RGB565(r,g,b) \
  (((r<<8) & 0b1111100000000000) | \
   ((g<<2) & 0b0000011111100000) | \
   ((b>>3) & 0b0000000000011111))

#define RGB565_R(rgb) \
    ((rgb & 0b1111100000000000) >> 8)
#define RGB565_G(rgb) \
    ((rgb & 0b0000011111100000) >> 2)
#define RGB565_B(rgb) \
    ((rgb & 0b0000000000011111) << 3)

uint16_t neo[N_LED];
uint16_t line[N_LED];

/* speeds for strings */
#define PERIOD  3

#define RNDMAX  11

void setup(void)
{
    PORTB &= ~0x02; // in case led pin left from the bl
    PORTB &= ~0x01; // neo_data_pin

    DDRB |= 0x03; // onboard LED and neo

    asm("cli");   // left on from bootloader

    // oranje

    memset(line, 0, (N_LED - 4)*2);

    line[0] = RGB565(0xaa, 0x33, 0x00);
    line[1] = RGB565(0xff, 0x88, 0x88);
    line[2] = RGB565(0xff, 0x66, 0x00);
    line[3] = RGB565(0xff, 0x66, 0x00);
    line[4] = RGB565(0xff, 0x66, 0x00);
    line[5] = RGB565(0xff, 0x66, 0x00);
    line[6] = RGB565(0xff, 0x66, 0x00);
    line[7] = RGB565(0xff, 0x88, 0x88);
    line[8] = RGB565(0xaa, 0x33, 0x00);
}

static inline void send_0(void)
{

    asm (

            // 0.40 us high

            "sbi 0x18,0\n"
            "nop\n"

            // 0.85 us low

            "cbi 0x18,0\n"
        );
}

static inline void send_1(void)
{
    asm (
            // 0.80 us high

            "sbi 0x18,0\n"
            "nop\n"
            "nop\n"
            "nop\n"
            "nop\n"
            "nop\n"

            // 0.45 us low
            "cbi 0x18,0\n"
        );
}

void send_rgb565(uint16_t c)
{
    uint8_t c8;

    // sequence is G R B

    c8 = RGB565_G(c);
    if (c8 & 0x80) send_1();
    else           send_0();
    if (c8 & 0x40) send_1();
    else           send_0();
    if (c8 & 0x20) send_1();
    else           send_0();
    if (c8 & 0x10) send_1();
    else           send_0();
    if (c8 & 0x08) send_1();
    else           send_0();
    if (c8 & 0x04) send_1();
    else           send_0();
    send_0();
    send_0();

    c8 = RGB565_R(c);
    if (c8 & 0x80) send_1();
    else           send_0();
    if (c8 & 0x40) send_1();
    else           send_0();
    if (c8 & 0x20) send_1();
    else           send_0();
    if (c8 & 0x10) send_1();
    else           send_0();
    if (c8 & 0x08) send_1();
    else           send_0();
    send_0();
    send_0();
    send_0();

    c8 = RGB565_B(c);
    if (c8 & 0x80) send_1();
    else           send_0();
    if (c8 & 0x40) send_1();
    else           send_0();
    if (c8 & 0x20) send_1();
    else           send_0();
    if (c8 & 0x10) send_1();
    else           send_0();
    if (c8 & 0x08) send_1();
    else           send_0();
    send_0();
    send_0();
    send_0();
}

void move_left(uint16_t string[N_LED])
{
    string[N_LED-1] = string[0];
    uint8_t l;
    for (l=0; l<N_LED-1; l++)
    {
        string[l] = string[l+1];
    }
}

void move_right(uint16_t string[N_LED])
{
    string[0] = string[N_LED-1];
    uint8_t l;
    for (l=N_LED-1; l>0; l--)
    {
        string[l] = string[l-1];
    }
}

uint16_t rgb565_add(uint16_t x, uint16_t y)
{
    uint16_t r, g, b;
    r = RGB565_R(x) + RGB565_R(y);
    g = RGB565_G(x) + RGB565_G(y);
    b = RGB565_B(x) + RGB565_B(y);
    if (r>0xff) r = 0xff;
    if (g>0xff) g = 0xff;
    if (b>0xff) b = 0xff;
    uint8_t r8, g8, b8;
    r8 = r;
    g8 = g;
    b8 = b;
    return RGB565(r8, g8, b8);
}


typedef struct biquad_data_s
{
    float a[2];
    float b[3];
    int16_t z[3];
} biquad_data_t;

int16_t biquad(biquad_data_t * bd, int16_t x)
{
    int16_t y;

    y =
        bd->b[2] * bd->z[1] +
        bd->b[1] * bd->z[0] +
        bd->b[0] * (
                x -
                bd->a[0] * bd->z[0] -
                bd->a[1] * bd->z[1]
                );

    bd->z[1] = bd->z[0];
    bd->z[0] = x;
    return y;
}

biquad_data_t bd_r;
biquad_data_t bd_g;
biquad_data_t bd_b;

void random_coefficients(void)
{
    bd_r.a[0] = (rand() % (2*256)-128.0) / 256.0;
    bd_r.a[1] = (rand() % (2*256)-128.0) / 256.0;
    bd_r.b[0] = (rand() % (2*256)-128.0) / 256.0;
    bd_r.b[1] = (rand() % (2*256)-128.0) / 256.0;
    bd_r.b[2] = (rand() % (2*256)-128.0) / 256.0;

    bd_g.a[0] = (rand() % (2*256)-128.0) / 256.0;
    bd_g.a[1] = (rand() % (2*256)-128.0) / 256.0;
    bd_g.b[0] = (rand() % (2*256)-128.0) / 256.0;
    bd_g.b[1] = (rand() % (2*256)-128.0) / 256.0;
    bd_g.b[2] = (rand() % (2*256)-128.0) / 256.0;

    bd_b.a[0] = (rand() % (2*256)-128.0) / 256.0;
    bd_b.a[1] = (rand() % (2*256)-128.0) / 256.0;
    bd_b.b[0] = (rand() % (2*256)-128.0) / 256.0;
    bd_b.b[1] = (rand() % (2*256)-128.0) / 256.0;
    bd_b.b[2] = (rand() % (2*256)-128.0) / 256.0;
}

void random_change_coefficients(void)
{
    bd_r.a[0] += .001 * (rand() % (2*256)-128);
    bd_r.a[1] += .001 * (rand() % (2*256)-128);
    bd_r.b[0] += .001 * (rand() % (2*256)-128);
    bd_r.b[1] += .001 * (rand() % (2*256)-128);
    bd_r.b[2] += .001 * (rand() % (2*256)-128);

    bd_g.a[0] += .001 * (rand() % (2*256)-128);
    bd_g.a[1] += .001 * (rand() % (2*256)-128);
    bd_g.b[0] += .001 * (rand() % (2*256)-128);
    bd_g.b[1] += .001 * (rand() % (2*256)-128);
    bd_g.b[2] += .001 * (rand() % (2*256)-128);

    bd_b.a[0] += .001 * (rand() % (2*256)-128);
    bd_b.a[1] += .001 * (rand() % (2*256)-128);
    bd_b.b[0] += .001 * (rand() % (2*256)-128);
    bd_b.b[1] += .001 * (rand() % (2*256)-128);
    bd_b.b[2] += .001 * (rand() % (2*256)-128);
}

void loop(void)
{
    static uint32_t frame = 0;
    frame++;
    int l;

    int16_t r;
    int16_t g;
    int16_t b;
    for (l=0; l<N_LED; l++)
    {
        r = RGB565_R(neo[l]);
        g = RGB565_G(neo[l]);
        b = RGB565_B(neo[l]);
        r = biquad(&bd_r, r);
        g = biquad(&bd_g, g);
        b = biquad(&bd_b, b);
        neo[l] = RGB565(r, g, b);
    }

    if ( (frame % 5) == 0)
    {
        random_change_coefficients();
    }

    for (l=0; l<N_LED; l++)
    {
        send_rgb565(neo[l]);
    }

    // commented as the code runtime should do it...
    //_delay_us(40);

    static int shift = 0;
    shift++;
    if ( (shift%PERIOD) == 0)
    {
        shift = 0;
        move_left(line);
        //move_right(line);
    }


    for (l=0; l<N_LED; l++)
    {
        neo[l] = line[l];
    }
}

int main(void)
{
    setup();

    int i;
    for (i=0; i<3; i++)
    {
        bd_r.z[i] = 0;
        bd_g.z[i] = 0;
        bd_b.z[i] = 0;
    }
    random_coefficients();

    while (1)
        loop();
}
