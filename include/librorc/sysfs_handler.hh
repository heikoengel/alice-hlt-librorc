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
#ifndef LIBRORC_SYSFS_HANDLER_H
#define LIBRORC_SYSFS_HANDLER_H

#include <librorc/defines.hh>
#include <stdint.h>
#include <string>
#include <vector>

namespace LIBRARY_NAME {

#define LIBRORC_SCANMODE_PCI 0
#define LIBRORC_SCANMODE_UIO 1

struct __attribute__((__packed__)) uio_pci_dma_request {
  uint64_t id;
  uint64_t size;
  int64_t numa_node;
};

struct kobject {
  char name[1024];
};

struct __attribute__((__packed__)) uio_pci_dma_private {
  char name[1024];
  size_t size;
  int32_t numa_node;
  uint64_t start;
  struct kobject kobj;
  char *buffer;
  struct scatterlist *sg;
  struct scatter *scatter;
  uint64_t length;
  uint64_t pages;
  struct device *device;
  struct page **page_list;
};

struct __attribute__((__packed__)) scatter {
  uint64_t page_link;
  uint32_t offset;
  uint32_t length;
  uint64_t dma_address;
};

class sysfs_handler {
public:
  sysfs_handler(uint32_t device_id, int scanmode=LIBRORC_SCANMODE_PCI);
  ~sysfs_handler();

  int get_bin_attr(const char *fname);
  int64_t get_char_attr(const char *fname, int nbytes);
  ssize_t get_attribute_size(const char *attr_name);
  bool attribute_exists(const char *attr_name);
  std::string get_base() { return m_sysfs_device_base; }
  std::string get_pci_slot_str() { return m_sysfs_pci_slot; }
  int mmap_file(void **map, std::string attr, uint64_t size, int open_flags,
                int prot);

  int allocate_buffer(uint64_t id, uint64_t size);
  int deallocate_buffer(uint64_t id);
  ssize_t get_buffer_size(uint64_t id);
  int mmap_buffer(uint32_t **map, uint64_t id, ssize_t size, bool wrap_map);
  void munmap_buffer(uint32_t *map, ssize_t size, bool wrap_map);
  std::vector<uint64_t> list_all_buffers();
  bool buffer_exists(uint64_t id);

  ssize_t get_sglist_size(uint64_t id);
  int mmap_sglist(void **map, uint64_t id, ssize_t size);
  void munmap_sglist(void *map, ssize_t size);

protected:
  int find_pci_device_by_id(uint32_t device_id);
  int find_uio_device_by_id(uint32_t device_id);
  int64_t __get_char_attr(std::string attr_path, int nbytes);
  bool __attribute_exists(std::string attr_path);
  int bind_pci_device();
  int write_ids_to_kernel_module();

  bool m_device_id_found;
  std::string m_sysfs_device_base;
  std::string m_sysfs_pci_slot;
};
}

#endif
