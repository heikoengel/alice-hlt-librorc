# librorc
`librorc` is the user space driver library for the HLT C-RORC. It uses the
kernel module from the [libpda](https://gitlab.cern.ch/ALICE-HLT/libpda) project
to manage DMA buffers and talk to the hardware. `librorc` is used among others
by [crorc-utils](https://gitlab.cern.ch/ALICE-HLT/crorc-utils) and the [HLT data
transport framework](https://gitlab.cern.ch/ALICE-HLT/core) to operate the
C-RORCs.

## Build requirements
* libpci
* libpthread

## Run time requirements
* `uio_pci_dma` kernel module from the libpda repository loaded
* a C-RORC

## Installation
```bash
mkdir -p build
mkdir -p dist
cd build

# Prepare with CMake
cmake ../ -DCMAKE_INSTALL_PREFIX=../dist -DCMAKE_BUILD_TYPE=RelWithDebInfo
# Optional additional CMake parameters:
# -DMODELSIM=ON : Build for HDL Simulation instead of hardware
# -DSHM=ON : allocate event_stream status structs in shared memory

# Compile and install, pass "-j [#cores]" to speed it up
make install

# Optional: build RPM
make package
```