#!/bin/bash

PCI_IDS[0]=0000\:01\:00.0
PCI_IDS[1]=0000\:02\:00.0
PCI_IDS[2]=0000\:04\:00.0
PCI_IDS[3]=0000\:05\:00.0
PCI_IDS[4]=0000\:06\:00.0
PCI_IDS[5]=0000\:08\:00.0

for ID in ${PCI_IDS[@]}
do
  if [ -f /sys/bus/pci/devices/${ID}/remove ]
  then
    echo "Removing ${ID}"
    echo 1 > /sys/bus/pci/devices/${ID}/remove
  fi
done

# rescan PCIe bus
echo "Rescanning..."
echo 1 > /sys/bus/pci/rescan

# make sure uio_pci_dma is loaded
if [ ! -f /sys/bus/pci/drivers/uio_pci_dma/new_id ]
then
  /sbin/modprobe uio_pci_dma
fi

# bind all C-RORCs to uio_pci_dma
echo "10dc 01a0" >> /sys/bus/pci/drivers/uio_pci_dma/new_id

# chmod a+rw for all detected devices
find -L /sys/module/uio_pci_dma/drivers/pci\:uio_pci_dma/ -maxdepth 2 -iname "*resource1*" -exec chmod a+rw {} \; 2>&1 | sed -n 's/.*\([0-9:\.]\{12\}\+\).*/Found device at \1/p'

# make sure i2c-dev is loaded
if [ ! -d /sys/module/i2c_dev ]
then
  /sbin/modprobe i2c_dev
fi
chmod a+rw /dev/i2c-*