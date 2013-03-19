#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#ifndef NOT_SET
    #define NOT_SET 0xFFFFFFFFFFFFFFFF
#endif

#ifndef PAGE_MASK
    #define PAGE_MASK ~(sysconf(_SC_PAGESIZE) - 1)
#endif

#ifndef PAGE_SIZE
    #define PAGE_SIZE sysconf(_SC_PAGESIZE)
#endif


#endif
