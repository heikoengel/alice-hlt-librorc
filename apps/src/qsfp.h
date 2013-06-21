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

int hextobin(unsigned char data)
{
  int data_bin = 0;
  int i;
  for(i=0;i<8;i++) {
    if(data & (1<<i))
      data_bin|=(1<<(4*i));
  }
  return data_bin;
}


void qsfp_set_page0(struct rorcfs_sysmon *sm)
{
  uint8_t data_r;
  // check page
  if ( sm->i2c_read_mem(slvaddr, 127, &data_r)<0 )
    printf("failed to read from i2c: %02x (%08x)\n",
        data_r, hextobin(data_r));
  else
    if ( data_r!=0 ) //page0 not selected
      sm->i2c_write_mem(slvaddr, 127, 0);
}


void qsfp_print_vendor_name(struct rorcfs_sysmon *sm)
{
  uint8_t data_r, i;
  // get vendor name
  printf("\tVendor Name:\t");
  for (i=148;i<=163;i++) {
    if ( sm->i2c_read_mem(slvaddr, i, &data_r)<0 )
      printf("failed to read from i2c: %02x (%08x)\n",
          data_r, hextobin(data_r));
    else
      printf("%c", data_r);
  }
  printf("\n");
}


void qsfp_print_part_number(struct rorcfs_sysmon *sm)
{
  uint8_t data_r, i;
  // get PartNumber
  printf("\tPart Number:\t");
  for (i=168;i<=183;i++) {
    if ( sm->i2c_read_mem(slvaddr, i, &data_r)<0 )
      printf("failed to read from i2c: %02x (%08x)\n",
          data_r, hextobin(data_r));
    else
      printf("%c", data_r);
  }
  printf("\n");
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
