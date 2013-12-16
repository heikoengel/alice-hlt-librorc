#!/bin/bash

PCI_IDS=("0000\:02\:04.0" "0000\:05\:04.0" "0000\:09\:04.0" "0000\:0c\:04.0" "0000\:0f\:04.0")

for ID in ${PCI_IDS[@]}
do
  if [ -f /sys/bus/pci/devices/${ID}/remove ]
  then
    echo 1 > /sys/bus/pci/devices/${ID}/remove
  fi
done

# rescan PCIe bus
echo 1 > /sys/bus/pci/rescan

# make sure uio_pci_dma is loaded
if [ ! -f /sys/bus/pci/drivers/uio_pci_dma/new_id ]
then
  modprobe uio_pci_dma
fi

# bind all C-RORCs to uio_pci_dma
echo "10dc 01a0" >> /sys/bus/pci/drivers/uio_pci_dma/new_id

# chmod a+rw for all detected devices
find -L /sys/module/uio_pci_dma/drivers/pci\:uio_pci_dma/ -maxdepth 2 -iname "*resource1*" -exec chmod a+rw {} \; >/dev/null 2>&1

chmod a+rw /dev/i2c-*
