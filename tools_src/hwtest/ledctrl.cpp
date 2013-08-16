/**
 * @file ledctrl.cpp
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2013-08-08
 *
 * @section LICENSE
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * */
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iomanip>

#include "librorc.h"

#define HELP_TEXT "ledctrl usage: \n\
        ledctrl [paramters] \n\
paramters: \n\
        -n [0...255]    Target device ID \n\
        -q [0...0x3f]   Set QSFP LEDs bit mask \n\
        -b [0...0x03]   Set Bracket LED bit mask \n\
        -g [0...0x03]   Set GPIO LED bit mask \n\
        -s              Print current LED settings \n\
for all bit masks: 1 is ON, 0 is off. \n\
QSFP map MSB to LSB: QSFP2_LED1, QSFP2_LED0, QSFP1_LED1, \
QSFP1_LED0,QSFP0_LED1, QSFP0_LED0, \n\
Bracket LED map MSB to LSB: BRACKET_LED_RED, BRACKET_LED_GREEN \n\
GPIO LED map MSB to LSB: GPIO_LED1, GPIO_LED0 \n\
"

using namespace std;

string 
test_bit
(
    uint32_t status,
    uint32_t bit,
    int polarity
)
{
    if ( polarity )
    {
        if ( status & (1<<bit) )
        {
            return "ON";
        }
        else
        {
            return "OFF";
        }
    }
    else
    {
        if ( status & (1<<bit) )
        {
            return "OFF";
        }
        else
        {
            return "ON";
        }
    }
}



int
main
(
    int argc,
    char *argv[]
)
{
    int32_t device_number = -1;

    int set_qsfp = 0;
    int set_bracket = 0;
    int set_gpio = 0;
    int print_status = 0;

    uint32_t qsfp_map, gpio_map, bracket_map;

    int arg;

    while ( (arg = getopt(argc, argv, "hn:b:g:q:s")) != -1 )
    {
        switch (arg)
        {
            case 'h':
                cout << HELP_TEXT;
                return 0;
                break;
            case 'n':
                device_number = atoi(optarg);
                break;
            case 'q':
                qsfp_map = strtol(optarg, NULL, 0) & 0x3f;
                set_qsfp = 1;
                break;
            case 'g':
                gpio_map = strtol(optarg, NULL, 0) & 0x3;
                set_gpio = 1;
                break;
            case 'b':
                bracket_map = strtol(optarg, NULL, 0) & 0x3;
                set_bracket = 1;
                break;
            case 's':
                print_status = 1;
                break;
            default:
                cout << HELP_TEXT;
                return -1;
        }
    }

    /** make sure all required parameters are provided and valid **/
    if ( device_number == -1 )
    {
        cout << "ERROR: device ID was not provided." << endl;
        cout << HELP_TEXT;
        return -1;
    }
    else if ( device_number > 255 || device_number < 0 )
    {
        cout << "ERROR: invalid device ID selected: " 
             << device_number << endl;
        cout << HELP_TEXT;
        return -1;
    }

    /** Instantiate device **/
    librorc::device *dev = NULL;
    try{ 
        dev = new librorc::device(device_number);
    }
    catch(...)
    { 
        cout << "Failed to intialize device " << device_number
             << endl;
        return -1;
    }

    /** Instantiate a new bar */
    librorc::bar *bar = NULL;
    try
    {
    #ifdef SIM
        bar = new librorc::sim_bar(dev, 1);
    #else
        bar = new librorc::rorc_bar(dev, 1);
    #endif
    }
    catch(...)
    {
        cout << "ERROR: failed to initialize BAR." << endl;
        abort();
    }

    /** get current settings
     * 17: QSFP2_LED1
     * 16: QSFP2_LED0
     *  9: QSFP1_LED1
     *  8: QSFP1_LED0
     *  1: QSFP0_LED1
     *  0: QSFP0_LED0*/
    uint32_t qsfpctrl = bar->get32(RORC_REG_QSFP_CTRL);
    if ( set_qsfp )
    {
        /** clear current LED settings */
        qsfpctrl |= (3<<16)|(3<<8)|(3<<0);

        /** set new values */
        qsfpctrl &= ~( ((qsfp_map>>4)&3)<<16 );
        qsfpctrl &= ~( ((qsfp_map>>2)&3)<<8 );
        qsfpctrl &= ~( ((qsfp_map>>0)&3)<<0 );
    }

    if ( set_gpio )
    {
        /** Clear current LED settings: bits [27:26] */
        qsfpctrl |= (3<<26);

        /* set new values */
        qsfpctrl &= ~( (gpio_map & 3)<<26 );
    }

    if ( set_bracket )
    {
	/** NOTE: bracket LEDs are active high! */
        /** Clear current LED settings: bits [25:24] */
        qsfpctrl &= ~(3<<24);

        /* set new values */
        qsfpctrl |= ( (bracket_map & 3)<<24 );
    }

    if ( set_qsfp || set_gpio || set_bracket )
    {
        /** write back new values */
        bar->set32(RORC_REG_QSFP_CTRL, qsfpctrl);
    }

    if ( print_status )
    {
        cout << "QSFP0 LED0: " << test_bit(qsfpctrl, 0, 0) << endl;
        cout << "QSFP0 LED1: " << test_bit(qsfpctrl, 1, 0) << endl;
        cout << "QSFP1 LED0: " << test_bit(qsfpctrl, 8, 0) << endl;
        cout << "QSFP1 LED1: " << test_bit(qsfpctrl, 9, 0) << endl;
        cout << "QSFP2 LED0: " << test_bit(qsfpctrl, 16, 0) << endl;
        cout << "QSFP2 LED1: " << test_bit(qsfpctrl, 17, 0) << endl;
        cout << "GPIO LED0: " << test_bit(qsfpctrl, 26, 0) << endl;
        cout << "GPIO LED1: " << test_bit(qsfpctrl, 27, 0) << endl;
        cout << "Bracket GREEN: " << test_bit(qsfpctrl, 24, 1) << endl;
        cout << "Bracket RED  : " << test_bit(qsfpctrl, 25, 1) << endl;
    }

    delete bar;
    delete dev;

    return 0;
}
