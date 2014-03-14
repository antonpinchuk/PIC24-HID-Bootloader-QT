#include "Compiler.h"
#define FOSC    (7370000ULL)
#define FCY     (FOSC/2)
#include <p24Fxxxx.h>
#include <stdio.h>
#include <stdlib.h>
#include <timer.h>
#include <libpic30.h>
//#include "HardwareProfile.h"


int main(void)
{
    //init LED's
    LATG &= 0xFC3F; TRISG &= 0xFC3F; LATF &= 0xFFCF; TRISF &= 0xFFCF; //G6,7,8,9 and F4,5
    TRISF |= 0x0030;
    TRISG |= 0x0300;
    TRISG &= 0x00C0;

    while(1)
    {
        __delay_ms(500);
    TRISF ^= 0x0030;
    //TRISG ^= 0x0300;
    TRISG ^= 0x00C0;
    }


    return 0;
}
