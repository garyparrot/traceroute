#ifndef MISC_HEADER_GUARD
#define MISC_HEADER_GUARD

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/capability.h>

extern const char* ICMP_TYPE_STR[]; 

int set_raw_capability(int);
int get_ipv4_address(const char*,struct in_addr*);
uint16_t checksum(const void* daat,size_t size);

#endif
