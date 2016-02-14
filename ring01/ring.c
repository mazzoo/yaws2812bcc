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
uint16_t string1[N_LED];
uint16_t string2[N_LED];
uint16_t string3[N_LED];

/* speeds for strings */
#define PRIME1  57
#define PRIME2   7
#define PRIME3  29

#define RNDMAX  11

void setup(void)
{
    PORTB &= ~0x02; // in case led pin left from the bl
    PORTB &= ~0x01; // neo_data_pin

    DDRB |= 0x03; // onboard LED and neo

    asm("cli");   // left on from bootloader

    string1[N_LED-4] = RGB565(44, 88,44);
    string1[N_LED-3] = RGB565( 6,111, 6);
    string1[N_LED-2] = RGB565(11, 88,11);

    // Rakete
    // (aus Lego City Undercover,
    //  Bright Lights Plaza,
    //  ester Stock unter dem Disco Dude)

    memset(string2, 0, (N_LED - 4)*2);

    string2[1] = RGB565(120, 0, 0);
    string2[2] = RGB565(120, 120, 0);
    string2[3] = RGB565(120, 120, 0);
    string2[4] = RGB565(120, 120, 0);
    string2[5] = RGB565(0, 0, 0);
    string2[6] = RGB565(0, 0, 30);
    string2[7] = RGB565(0, 0, 50);
    string2[8] = RGB565(0, 0, 10);

    // gruener Wurm

    memset(string3, 0, N_LED*2);

    string3[10] = RGB565(0, 5, 0);
    string3[11] = RGB565(0, 15, 0);
    string3[12] = RGB565(0, 20, 0);
    string3[13] = RGB565(0, 30, 40);
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

void send_byte(uint8_t by)
{
    uint8_t b;
    for (b=0; b<8; b++)
    {
        if (by & 0x80)
            send_1();
        else
            send_0();
        by <<= 1;
    }
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

void loop(void)
{
    static uint32_t frame = 0;
    frame++;

#define FLASH
#ifdef FLASH
    int f = frame % 9997;
    if (
            ( (f > 80) && (f < 100) ) ||
            ( (f > 130) && (f < 150) )
       )
    {
        memset(neo, 0xff, N_LED*2);
    }
#endif

    int l;

    for (l=0; l<N_LED; l++)
    {
        send_rgb565(neo[l]);
    }

    // commented as the code runtime should do it...
    //_delay_us(40);

    static int s1 = 0;
    s1++;
    if ( (s1%PRIME1) == 0)
    {
        s1 = 0;
        move_left(string1);
        //move_right(string1);
    }

    static int s2 = 0;
    s2++;
    if ( (s2%PRIME2) == 0)
    {
        s2 = 0;
        move_left(string2);
        //move_right(string2);
    }

    static int s3 = 0;
    s3++;
    if ( (s3%PRIME3) == 0)
    {
        s3 = 0;
        move_right(string3);
        //move_left(string3);
    }

    for (l=0; l<N_LED; l++)
    {
        neo[l] = rgb565_add(rgb565_add(string1[l], string2[l]), string3[l]);
    }
}

int main(void)
{
    setup();
    while (1)
        loop();
}
