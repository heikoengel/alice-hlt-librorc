/**
 * Copyright (c) 2015, Heiko Engel <hengel@cern.ch>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of University Frankfurt, CERN nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL A COPYRIGHT HOLDER BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **/
#include <iostream>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <sstream>
#include <cstring>

#include <librorc/sysfs_handler.hh>
#include <librorc/error.hh>

namespace LIBRARY_NAME {

#define SYSFS_MOD_BASE "/sys/bus/pci/drivers/uio_pci_dma/"
#define SYSFS_PCI_BASE "/sys/bus/pci/devices/"
#define SYSFS_ATTR_NEW_ID "new_id"
#define SYSFS_ATTR_DMA_REQUEST "dma/request"
#define SYSFS_ATTR_DMA_FREE "dma/free"
#define SYSFS_ATTR_BUF_DIR "dma/"
#define SYSFS_ATTR_DMA_MAP "map"
#define SYSFS_ATTR_DMA_SG "sg"
#define SYSFS_ATTR_NUMA_NODE "numa_node"
#define SYSFS_ATTR_DEVICE_ID "device"
#define SYSFS_ATTR_VENDOR_ID "vendor"

#define LIBRORC_DEFAULT_SPINS 21

typedef struct {
  uint16_t vendor_id;
  uint16_t device_id;
} t_pci_id;

t_pci_id pci_ids[] = {
    {0x10dc, 0x01a0},
#ifdef MODELSIM
    {0x17aa, 0x1e22}, /* Heikos SMBus Controller for modelsim*/
    {0x8086, 0x1e22}, /* Heikos SMBus Controller for modelsim*/
#endif
};

/*************************** Helpers *********************************/
std::string idToStr(uint64_t id) {
  std::stringstream ss;
  ss << id;
  return ss.str();
}

std::string getBufferAttributeName(uint64_t id, std::string name) {
  return std::string(SYSFS_ATTR_BUF_DIR) + idToStr(id) + "/" + name;
}

int spinOpen(const char *path, int flags,
             uint64_t spins = LIBRORC_DEFAULT_SPINS) {
  int fd = -1;
  for (uint64_t i = 0; i < spins; i++) {
    fd = open(path, flags);
    if (fd != -1) {
      break;
    }
    usleep(1 << i);
  }
  return (fd);
}

int spinStat(const char *path, struct stat *fstat,
             uint64_t spins = LIBRORC_DEFAULT_SPINS) {
  int ret = -1;
  for (uint64_t i = 0; i < spins; i++) {
    ret = stat(path, fstat);
    if (ret != -1) {
      break;
    }
    usleep(1 << i);
  }
  return ret;
}

/*************************** Base *********************************/
sysfs_handler::sysfs_handler(uint32_t device_id, int scanmode) {
  // make sure kernel module is loaded
  if (!__attribute_exists(SYSFS_MOD_BASE)) {
    throw LIBRORC_DEVICE_ERROR_PDA_KMOD_MISMATCH;
  }
  if (scanmode == LIBRORC_SCANMODE_PCI) {
    // iterate over all PCI devices
    if (find_pci_device_by_id(device_id) != 0) {
      throw LIBRORC_DEVICE_ERROR_PDADEV_FAILED;
    }
    // bind PCI device to kernel module
    if (bind_pci_device() != 0) {
      throw LIBRORC_DEVICE_ERROR_PDADOP_FAILED;
    }
  } else {
    // scanmode == LIBRORC_SCANMODE_UIO
    // iterate over pre-bound devices only
    if (find_uio_device_by_id(device_id) != 0) {
      throw LIBRORC_DEVICE_ERROR_PDADEV_FAILED;
    }
  }
}

sysfs_handler::~sysfs_handler(){};

int sysfs_handler::get_bin_attr(const char *attr) {
  std::string fname = m_sysfs_device_base + attr;
  int fd = spinOpen(fname.c_str(), O_RDONLY);
  if (fd == -1) {
    perror(fname.c_str());
    return -1;
  }
  int32_t val;
  if (read(fd, &val, sizeof(int32_t)) != sizeof(val)) {
    perror(fname.c_str());
    close(fd);
    return -1;
  }
  close(fd);
  return val;
}

int64_t sysfs_handler::get_char_attr(const char *attr, int nbytes) {
  std::string fname = m_sysfs_device_base + attr;
  return __get_char_attr(fname, nbytes);
}

ssize_t sysfs_handler::get_attribute_size(const char *attr_name) {
  std::string fname = m_sysfs_device_base + attr_name;
  struct stat fstat;
  if (spinStat(fname.c_str(), &fstat) == -1) {
    std::cerr << "failed to stat " << fname << std::endl;
    perror(fname.c_str());
    return -1;
  }
  if (!S_ISREG(fstat.st_mode)) {
    return -1;
  }
  return fstat.st_size;
}

bool sysfs_handler::attribute_exists(const char *attr_name) {
  std::string fname = m_sysfs_device_base + attr_name;
  return __attribute_exists(fname);
}

int sysfs_handler::mmap_file(void **map, std::string attr, uint64_t size,
                             int open_flags, int prot) {
  std::string fname = m_sysfs_device_base + attr;
  int fd = spinOpen(fname.c_str(), open_flags);
  if (fd == -1) {
    std::cerr << "cannot open " << fname << ": " << std::endl;
    perror(fname.c_str());
    return -1;
  }

  *map = mmap(NULL, size, prot, MAP_SHARED, fd, 0);
  close(fd);
  if (*map == MAP_FAILED) {
    std::cerr << "mmap(" << fname << ") failed" << std::endl;
    return -1;
  }
  return 0;
}

/*************************** Buffer Handling *********************************/
int sysfs_handler::allocate_buffer(uint64_t id, uint64_t size) {
  if (size == 0) {
    errno = EINVAL;
    perror("allocate_buffer");
    return -1;
  }
  struct uio_pci_dma_private request;
  memset(&request, 0, sizeof(request));
  request.size = size;
  request.start = 0;
  request.numa_node = get_char_attr(SYSFS_ATTR_NUMA_NODE, 255);
  snprintf(request.name, 1024, "%ld", id);

  std::string fname = m_sysfs_device_base + SYSFS_ATTR_DMA_REQUEST;
  int fd = spinOpen(fname.c_str(), O_WRONLY);
  if (fd == -1) {
    std::cerr << "allocate_buffer: failed to open " << fname << std::endl;
    perror(fname.c_str());
    return -1;
  }

  ssize_t nbytes = write(fd, &request, sizeof(request));
  if (nbytes <= 0) {
    std::cerr << "allocate_buffer: failed to write to " << fname << std::endl;
    close(fd);
    return -1;
  }
  close(fd);
  return 0;
}

int sysfs_handler::deallocate_buffer(uint64_t id) {
  std::string fname = m_sysfs_device_base + SYSFS_ATTR_DMA_FREE;
  int fd = spinOpen(fname.c_str(), O_WRONLY);
  if (fd == -1) {
    std::cerr << "deallocate_buffer: failed to open " << fname << std::endl;
    perror(fname.c_str());
    return -1;
  }

  char buffer_name[1024];
  snprintf(buffer_name, sizeof(buffer_name), "%ld", id);

  ssize_t nbytes = write(fd, buffer_name, strlen(buffer_name) + 1);
  if (nbytes <= 0) {
    std::cerr << "deallocate_buffer: failed to write to" << fname << std::endl;
    perror(fname.c_str());
    close(fd);
    return -1;
  }
  close(fd);

  return 0;
}

ssize_t sysfs_handler::get_buffer_size(uint64_t id) {
  return get_attribute_size(
      getBufferAttributeName(id, SYSFS_ATTR_DMA_MAP).c_str());
}

bool sysfs_handler::buffer_exists(uint64_t id) {
  return attribute_exists(
      getBufferAttributeName(id, SYSFS_ATTR_DMA_MAP).c_str());
}

int sysfs_handler::mmap_buffer(uint32_t **map, uint64_t id, ssize_t size,
                               bool wrap_map) {
  ssize_t mapping_size = (wrap_map) ? (size * 2) : size;
  return mmap_file((void **)map, getBufferAttributeName(id, SYSFS_ATTR_DMA_MAP),
                   mapping_size, O_RDWR, PROT_READ | PROT_WRITE);
}

void sysfs_handler::munmap_buffer(uint32_t *map, ssize_t size, bool wrap_map) {
  ssize_t mapping_size = (wrap_map) ? (size * 2) : size;
  munmap(map, mapping_size);
}

std::vector<uint64_t> sysfs_handler::list_all_buffers() {
  std::vector<uint64_t> bufferlist;
  bufferlist.clear();
  std::string dname = std::string(m_sysfs_device_base) + SYSFS_ATTR_BUF_DIR;
  DIR *directory = NULL;
  struct dirent *directory_entry;
  directory = opendir(dname.c_str());
  if (directory == NULL) {
    perror(dname.c_str());
    return bufferlist;
  }

  while (NULL != (directory_entry = readdir(directory))) {
    struct stat file_status;
    std::string dirname = dname + directory_entry->d_name;
    if (stat(dirname.c_str(), &file_status) == -1) {
      continue;
    }
    if (S_ISDIR(file_status.st_mode)) {
      char *endptr;
      uint64_t bufid = strtoul(directory_entry->d_name, &endptr, 0);
      if ((const char *)directory_entry->d_name != '\0' and *endptr == '\0') {
        bufferlist.push_back(bufid);
      }
    }
  }
  closedir(directory);
  return bufferlist;
}

/***************** Scatter-Gather-List Handling ******************************/
int sysfs_handler::mmap_sglist(void **map, uint64_t id, ssize_t size) {
  return mmap_file(map, getBufferAttributeName(id, SYSFS_ATTR_DMA_SG), size,
                   O_RDONLY, PROT_READ);
}

void sysfs_handler::munmap_sglist(void *map, ssize_t size) {
  munmap(map, size);
}

ssize_t sysfs_handler::get_sglist_size(uint64_t id) {
  return get_attribute_size(
      getBufferAttributeName(id, SYSFS_ATTR_DMA_SG).c_str());
}

/***************** Protected / Internal Methods *******************************/
int sysfs_handler::find_pci_device_by_id(uint32_t device_id) {
  DIR *directory = NULL;
  struct dirent *directory_entry;
  uint64_t index = 0;

  m_device_id_found = false;
  directory = opendir(SYSFS_PCI_BASE);
  if (directory == NULL) {
    return -1;
  }

  // iterate over PCI devices and check for matching device/vendor IDs
  while (NULL != (directory_entry = readdir(directory)) && !m_device_id_found) {
    struct stat file_status;
    std::string dirname = std::string(SYSFS_PCI_BASE) + directory_entry->d_name;
    std::string vendorfile = dirname + "/vendor";
    std::string devicefile = dirname + "/device";
    if (lstat(dirname.c_str(), &file_status) == -1 ||
        !S_ISLNK(file_status.st_mode) ||
        !__attribute_exists(vendorfile) ||
        !__attribute_exists(devicefile)) {
      continue;
    }

    uint16_t vendor = __get_char_attr(vendorfile, 6);
    uint16_t device = __get_char_attr(devicefile, 6);
    for (uint64_t i = 0; i < (sizeof(pci_ids) / sizeof(t_pci_id)); i++) {
      if (vendor == pci_ids[i].vendor_id && device == pci_ids[i].device_id) {
        if (index == device_id) {
          m_sysfs_pci_slot = directory_entry->d_name;
          m_device_id_found = true;
          break;
        }
        index++;
      }
    }
  }
  closedir(directory);
  m_sysfs_device_base = std::string(SYSFS_MOD_BASE) + m_sysfs_pci_slot + "/";

  return (m_device_id_found) ? 0 : -1;
}

int sysfs_handler::find_uio_device_by_id(uint32_t device_id) {
  DIR *directory = NULL;
  struct dirent *directory_entry;
  uint64_t index = 0;

  m_device_id_found = false;
  directory = opendir(SYSFS_MOD_BASE);
  if (directory == NULL) {
    return -1;
  }

  // iterate over devices that are already bound to the kernel module
  while (NULL != (directory_entry = readdir(directory)) && !m_device_id_found) {
    struct stat file_status;
    std::string dirname = std::string(SYSFS_MOD_BASE) + directory_entry->d_name;
    std::string devicefile = dirname + "/device";
    if (lstat(dirname.c_str(), &file_status) == -1 ||
        !S_ISLNK(file_status.st_mode) ||
        !__attribute_exists(devicefile)) {
      continue;
    }
    if (index == device_id) {
      m_sysfs_pci_slot = directory_entry->d_name;
      m_device_id_found = true;
      break;
    }
    index++;
  }
  closedir(directory);
  m_sysfs_device_base = std::string(SYSFS_MOD_BASE) + m_sysfs_pci_slot + "/";

  return (m_device_id_found) ? 0 : -1;
}

int sysfs_handler::bind_pci_device() {
  if (!__attribute_exists(m_sysfs_device_base)) {
    // not bound -> bind all
    if (write_ids_to_kernel_module() != 0) {
      return -1;
    }
    // check again if bound now
    if (!__attribute_exists(m_sysfs_device_base)) {
      return -1;
    }
  }
  return 0;
}

bool sysfs_handler::__attribute_exists(std::string attr_path) {
  struct stat fstat;
  if (stat(attr_path.c_str(), &fstat) == -1) {
    return false;
  }
  return true;
}

int64_t sysfs_handler::__get_char_attr(std::string attr_path, int nbytes) {
  int fd = spinOpen(attr_path.c_str(), O_RDONLY);
  if (fd == -1) {
    perror(attr_path.c_str());
    return -1;
  }
  char str[nbytes+1];
  memset(str, 0, nbytes+1);
  ssize_t readsize = read(fd, str, nbytes);
  close(fd);
  // note: the actual number of chars in the attr may be lower than nbytes
  if (readsize == -1) {
    perror(attr_path.c_str());
    return -1;
  }
  return strtoll(str, NULL, 0);
}

int sysfs_handler::write_ids_to_kernel_module() {
  std::string fname = std::string(SYSFS_MOD_BASE) + SYSFS_ATTR_NEW_ID;
  FILE *fp = fopen(fname.c_str(), "w");
  if (fp == NULL) {
    perror(fname.c_str());
    return -1;
  }
  // write device IDs to new_id attribute
  for (uint64_t i = 0; i < (sizeof(pci_ids) / sizeof(t_pci_id)); i++) {
    rewind(fp);
    if (fprintf(fp, "%04x %04x", pci_ids[i].vendor_id, pci_ids[i].device_id) <
        0) {
      fclose(fp);
      perror(fname.c_str());
      return -1;
    }
  }
  fclose(fp);
  return 0;
}
}
