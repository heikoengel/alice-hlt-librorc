/**
 * @file rorcfs_device.cpp
 * @author Heiko Engel <hengel@cern.ch>
 * @date 2011-08-16
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
 **/

#include <cstdlib>
#include <stdio.h>
#include <dirent.h>
#include <cstring>
#include <errno.h>
#include <sys/stat.h>

#include "rorcfs_device.hh"


rorcfs_device::rorcfs_device()
{ 
	dname = NULL;
	dname_size = 0;
}

rorcfs_device::~rorcfs_device()
{ }

int rorcfs_device::init(int n) {
	int n_dev;
	char basedir[] = "/sys/module/rorcfs/drivers/pci:rorcfs/";
	struct dirent **namelist;
	char *pEnd;

	if (dname)
		free(dname);
	
	n_dev = find_rorc(basedir, &namelist);
	if ( n_dev <= 0 || n<0 || n > n_dev )
		return -1;

	bus = (uint8) strtoul(namelist[n]->d_name+5, &pEnd, 16);
	slot = (uint8) strtoul(pEnd+1, &pEnd, 16);
	func= (uint8) strtoul(pEnd+1, &pEnd, 16);

	dname_size = snprintf(NULL, 0, "%s0000:%02x:%02x.%x/mmap/", 
			basedir, bus, slot, func);
	dname_size++;

	dname = (char *) malloc(dname_size);
	if ( !dname )
		return -1;

	snprintf(dname, dname_size, "%s0000:%02x:%02x.%x/mmap/", 
			basedir, bus, slot, func);

	return 0;
}


int rorcfs_device::getDName ( char **ptr )
{
	*ptr = dname;
	return dname_size;
}


/**
 * scandir_filter:
 * return nonzero if directory name starts with "0000:"
 * This is a filter for PCI-IDs, e.g. "0000:03:00.0"
 **/
int scandir_filter(const struct dirent* entry) {
	if( strncmp(entry->d_name, "0000:", 5)== 0)
		return 1;
	else
		return 0;
}

/**
 * find PCIe devices bound with rorcfs
 *
 * the directory names can be found in namelist[n]->d_name
 * @param basedir sysfs directory to be listed
 * @param namelist struct dirent*** to contain the filtered list
 * @return number of devices found bound with rorcfs
 **/
int rorcfs_device::find_rorc(char *basedir, struct dirent ***namelist) {
	int err, n;
	struct stat filestat;
	struct dirent **namelist_i;

	err = stat(basedir, &filestat);
	if (err) {
		return -ENOTDIR;
	}

	n = scandir(basedir, &namelist_i, scandir_filter, alphasort);
	*namelist = namelist_i;

	return n;
}
