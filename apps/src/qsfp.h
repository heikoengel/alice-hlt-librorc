/**
 * @file qsfp.h
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2013-05-10
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
#ifndef QSFP_H
#define QSFP_H

using namespace std;

#define slvaddr 0x50

//TODO : thorw errors!

void qsfp_set_page0(struct rorcfs_sysmon *sm)
{
    uint8_t data_r;

    try
    {
        data_r = sm->i2c_read_mem(slvaddr, 127);
    }
    catch(...)
    {
        cout << "Failed to read from i2c!" << endl;
        return;
    }

    if( data_r!=0 )
    {
        sm->i2c_write_mem(slvaddr, 127, 0);
    }
}


void qsfp_print_vendor_name(struct rorcfs_sysmon *sm)
{
    cout << "Vendor Name: ";

    uint8_t data_r;
    //TODO this is redundant
    for(uint8_t i=148; i<=163; i++)
    {
        try
        { data_r = sm->i2c_read_mem(slvaddr, i); }
        catch(...)
        {
            cout << "Failed to read from i2c!" << endl;
            return;
        }
        cout << (char)data_r;
    }
    cout << endl;
}


void qsfp_print_part_number(struct rorcfs_sysmon *sm)
{
    cout << "Part Number: ";

    uint8_t data_r = 0;
    for(uint8_t i=168; i<=183; i++)
    {
        try
        { data_r = sm->i2c_read_mem(slvaddr, i); }
        catch(...)
        {
            cout << "Failed to read from i2c!" << endl;
            return;
        }
        cout << (char)data_r;
    }
    cout << endl;
}



void qsfp_print_temp(struct rorcfs_sysmon *sm)
{
    uint8_t data_r;
    try
    { data_r = sm->i2c_read_mem(slvaddr, 23); }
    catch(...)
    {
        cout << "Failed to read from i2c!" << endl;
        return;
    }

    uint32_t temp = data_r;
    try
    { data_r = sm->i2c_read_mem(slvaddr, 22); }
    catch(...)
    {
        cout << "Failed to read from i2c!" << endl;
        return;
    }

    temp += ((uint32_t)data_r<<8);
    printf("\tTemperature:\t%.2f °C\n", ((float)temp/256));
}


#endif
