#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/select.h>

#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>         
#include <netdb.h>

#include <netinet/in.h>        /* IPv4 packet format */
#include <netinet/ip_icmp.h>   /* ICMP format */

#include "misc.h"

#define MAX_HOP 32 

const char* errfunc;
const char* errmsg;

#define on_error_exit(msg)          do{ errmsg = msg;  goto on_error_exit; }while(0);
#define on_function_exit(func)      do{ errmsg = NULL; errfunc = func; goto on_function_exit; }while(0);
#define on_function2_exit(func,msg) do{ errmsg = msg;  errfunc = func; goto on_function2_exit; }while(0);

int main(int argc,char* args[]){
    int sockfd;
    struct sockaddr_in src_ip= {0};
    struct sockaddr_in dst_ip = {0};
    setbuf(stdout,NULL);

    // Read arguments.
    if(argc != 3){
        fprintf(stderr,"[usage] %s <destination ipv4 address> <network interface name>\n",args[0]);
        exit(0);
    }
    if(inet_aton(args[1],&dst_ip.sin_addr) == 0)
        on_function2_exit("inet_aton","Failed to parse destination ip");
    if(get_ipv4_address(args[2],&src_ip.sin_addr) != 0)
        on_function2_exit("get_ipv4_address", "Unexpected condition during retrive network interface ip");

    printf("Traceroute from %s(%s)",inet_ntoa(src_ip.sin_addr),args[2]);
    printf(" to %s\n",inet_ntoa(dst_ip.sin_addr));
    
    // Open socket
    set_raw_capability(CAP_SET);
    if((sockfd = socket(PF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0)
        on_function_exit("socket");
    set_raw_capability(CAP_CLEAR);

    // traceroute
    for(int hop = 1, stop = 0;!stop && hop < MAX_HOP;hop++){
        struct icmp icmph = {0};
        icmph.icmp_type = ICMP_ECHO;
        icmph.icmp_seq = hop;
        icmph.icmp_cksum = checksum(&icmph,sizeof(struct icmp));

        if(setsockopt(sockfd,IPPROTO_IP,IP_TTL,&hop,sizeof(hop)) < 0)
            on_function_exit("setsockopt");

        printf("Hop %3d ",hop);

        for(int attempt = 0;attempt < 3;attempt++){
            sendto(sockfd, &icmph, sizeof(struct icmp), 0, (struct sockaddr*)&dst_ip, sizeof(dst_ip));

            fd_set read_set = {0};
            FD_SET(sockfd,&read_set);
            struct timeval tv = {0,1000000};
            int rc = select(sockfd+1,&read_set,NULL,NULL,&tv);
            int ms = 1000000 - tv.tv_sec * 1000000 - tv.tv_usec;

            if(rc > 0){
                #define MAX_IP_PACKET_SIZE sizeof(unsigned char)*65536
                unsigned char* buf = malloc(MAX_IP_PACKET_SIZE);
                if(buf == NULL)
                    on_function_exit("malloc");

                if(recv(sockfd,buf,MAX_IP_PACKET_SIZE,0) < 0)
                    on_function_exit("recv");
                
                // extrace packet
                struct ip* ipheader = (struct ip*)buf;
                struct icmp* icmpheader = (struct icmp*)(buf + (ipheader->ip_hl * 4));

                if(checksum(ipheader,(ipheader->ip_hl*4)) != 0)
                    on_error_exit("validation for ip packet checksum failed.");
                if(checksum(icmpheader,sizeof(struct icmp)) != 0)
                    on_error_exit("validation for icmp packet checksum failed.");
                
                printf("%15s (%5.1fms) ", inet_ntoa(ipheader->ip_src), ms/1000.0);

                if(icmpheader->icmp_type == ICMP_ECHOREPLY){
                    stop = 1;
                }

                free(buf);
                
            }else{
                ms = 1000000;
                printf("%14s (%5.1fms) ","Timeout",ms/1000.0);
            }
        }

        printf("\n");

    }

    close(sockfd);

    return 0;

on_function_exit:
on_function2_exit:
    perror(errfunc);
on_error_exit:
    if(errmsg != NULL) fprintf(stderr,"%s\n",errmsg);
}
