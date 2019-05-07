#include "misc.h"

const char* ICMP_TYPE_STR[] = {
              "Echo Reply",			    
              "Unassigned",
              "Unassigned",
              "Destination Unreachable",	
              "Source Quench",		    
              "Redirect (change route)",	
              "Unassigned",
              "Unassigned",
              "Echo Request",			    
              "Unassigned",
              "Unassigned",
              "Time Exceeded",		    
              "Parameter Problem",		
              "Timestamp Request",		
              "Timestamp Reply",		    
              "Information Request",		
              "Information Reply",		
              "Address Mask Request",		
              "Address Mask Reply"		
};

uint16_t checksum(const void* data,size_t size){
    unsigned int chk = 0;
    const unsigned short* d = data;
    
    while(size>1){
        chk += *d;
        size-=2;
        d++;
    }
    if(size)
        chk += *(unsigned char*)d << 8;

    chk = (chk&0xffff) + (chk>>16);
    chk = (chk&0xffff) + (chk>>16);
    chk = (chk&0xffff) + (chk>>16);

    return ~chk;
}

// 設定CAP_NET_RAW能力
int set_raw_capability(int stat){
    cap_t caps;
    cap_value_t cap[1] = {CAP_NET_RAW};
    cap_flag_value_t nbit;

    // 得到當前Process的capblility handle
    if((caps = cap_get_proc()) == NULL)
        goto on_exit_1;

    // 檢查當前Process能不能夠啟用CAP_NET_RAW能力
    if(cap_get_flag(caps,CAP_NET_RAW,CAP_PERMITTED,&nbit) == -1)
        goto on_exit_2;

    if(nbit){
        // 啟用CAP_NET_RAW能力 
        if(cap_set_flag(caps,CAP_EFFECTIVE,1,cap,stat) == -1)
            goto on_exit_2;

        // 將設定套用至目前的Process
        if(cap_set_proc(caps) == -1)
            goto on_exit_2;
        
        //釋放
        if(cap_free(caps) == -1)
            goto on_exit_1;
    }

    return 0;

on_exit_2:
    cap_free(caps);
on_exit_1:
    return -1;
}

//取得網路卡的IPv4地址
int get_ipv4_address(const char* if_name, struct in_addr* addr){
    struct ifreq interface;
    int fd = socket(AF_INET,SOCK_DGRAM, 0);

    // 設定interface是IPv4 address family
    interface.ifr_addr.sa_family = AF_INET;
    // 把給定的interface name複製到interface裏面
    strncpy(interface.ifr_name,if_name,IFNAMSIZ-1);
    // 使用ioctl取得指定interface的 IPv4
    ioctl(fd,SIOCGIFADDR, &interface);

    close(fd);

    if(errno != 0) return -1;

    addr->s_addr = ((struct sockaddr_in*)&(interface.ifr_addr))->sin_addr.s_addr;
    return 0;
}
