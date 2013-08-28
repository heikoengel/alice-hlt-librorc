#ifndef LIBRORC_DEFINES_H
#define LIBRORC_DEFINES_H

    enum librorc_verbosity_enum
    {
        LIBRORC_VERBOSE_OFF = 0,
        LIBRORC_VERBOSE_ON  = 1
    };

    typedef enum librorc_verbosity_enum librorc_verbosity;

    #define PAGE_MASK ~(sysconf(_SC_PAGESIZE) - 1)
    #define PAGE_SIZE sysconf(_SC_PAGESIZE)

#endif /** LIBRORC_DEFINES_H */
