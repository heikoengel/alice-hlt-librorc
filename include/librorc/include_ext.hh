#ifndef INCLUDE_EXT_H
#define INCLUDE_EXT_H

#include <assert.h>
#include <dirent.h>
#include <fcntl.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <unistd.h>
#include <stdint.h>

#include <pthread.h>

#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <pda.h>

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <vector>

#ifndef MODELSIM_SERVER
    #define MODELSIM_SERVER "localhost"
#endif


using namespace std;

#endif /** INCLUDE_EXT_H */
