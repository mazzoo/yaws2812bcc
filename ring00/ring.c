#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <avr/io.h>
#include <util/delay.h>

#define G 0
#define R 1
#define B 2

#define N_LED 24

uint8_t neo[N_LED][3];
uint8_t string1[N_LED][3];
uint8_t string2[N_LED][3];
uint8_t string3[N_LED][3];

/* speeds for strings */
#define PRIME1  107
#define PRIME2  47
#define PRIME3  197

#define RNDMAX  11

void setup(void)
{
    PORTB &= ~0x02; // in case led pin left from the bl
    PORTB &= ~0x01; // neo_data_pin

    DDRB |= 0x03; // onboard LED and neo

    asm("cli");   // bootloader OFF

    int l, c;
    for (l=N_LED-4; l<N_LED; l++)
    {
        for (c=0; c<3; c++)
        {
            string1[l][R] = random()%RNDMAX;
            string1[l][G] = random()%RNDMAX;
            string1[l][B] = random()%RNDMAX;
        }
    }

    // Rakete
    // (aus Lego City Undercover,
    //  Bright Lights Plaza,
    //  ester Stock unter dem Disco Dude)

    memset(string2, 0, (N_LED - 4)*3);

    string2[1][R] = 120;
    string2[1][G] = 0;
    string2[1][B] = 0;

    string2[2][R] = 120;
    string2[2][G] = 120;
    string2[2][B] = 0;

    string2[3][R] = 120;
    string2[3][G] = 120;
    string2[3][B] = 0;

    string2[4][0] = 120;
    string2[4][1] = 120;
    string2[4][2] = 0;

    string2[5][0] = 0;
    string2[5][1] = 0;
    string2[5][2] = 0;

    string2[6][0] = 0;
    string2[6][1] = 0;
    string2[6][2] = 30;

    string2[7][0] = 0;
    string2[7][1] = 0;
    string2[7][2] = 50;

    string2[8][0] = 0;
    string2[8][1] = 0;
    string2[8][2] = 10;

    // gruener Wurm

    memset(string3, 0, N_LED*3);

    string3[10][R] = 0;
    string3[10][G] = 5;
    string3[10][B] = 0;

    string3[11][R] = 0;
    string3[11][G] = 15;
    string3[11][B] = 0;

    string3[12][R] = 0;
    string3[12][G] = 20;
    string3[12][B] = 0;

    string3[13][R] = 0;
    string3[13][G] = 30;
    string3[13][B] = 40;
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

void move_left(uint8_t string[N_LED][3])
{
    string[N_LED-1][0] = string[0][0];
    string[N_LED-1][1] = string[0][1];
    string[N_LED-1][2] = string[0][2];
    uint8_t l;
    for (l=0; l<N_LED-1; l++)
    {
        string[l][0] = string[l+1][0];
        string[l][1] = string[l+1][1];
        string[l][2] = string[l+1][2];
    }
}

void move_right(uint8_t string[N_LED][3])
{
    string[0][0] = string[N_LED-1][0];
    string[0][1] = string[N_LED-1][1];
    string[0][2] = string[N_LED-1][2];
    uint8_t l;
    for (l=N_LED-1; l>0; l--)
    {
        string[l][0] = string[l-1][0];
        string[l][1] = string[l-1][1];
        string[l][2] = string[l-1][2];
    }
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
        memset(neo, 0xff, N_LED*3);
    }
#endif

    int l;

    for (l=0; l<N_LED; l++)
    {
        // G
        send_byte(neo[l][G]);

        // R
        send_byte(neo[l][R]);

        // B
        send_byte(neo[l][B]);
    }

    // commented as the code runtime should do it...
    //_delay_us(40);

    static int s1 = 0;
    s1++;
    if ( (s1%PRIME1) == 0)
    {
        s1 = 0;
        move_left(string1);
    }

    static int s2 = 0;
    s2++;
    if ( (s2%PRIME2) == 0)
    {
        s2 = 0;
        move_left(string2);
    }

    static int s3 = 0;
    s3++;
    if ( (s3%PRIME3) == 0)
    {
        s3 = 0;
        move_right(string3);
    }

    for (l=0; l<N_LED; l++)
    {
        neo[l][0] = string1[l][0] + string2[l][0] + string3[l][0];
        neo[l][1] = string1[l][1] + string2[l][1] + string3[l][1];
        neo[l][2] = string1[l][2] + string2[l][2] + string3[l][2];
    }
}

int main(void)
{
    setup();
    while (1)
        loop();
}
