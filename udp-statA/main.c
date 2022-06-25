#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <net/ethernet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <sys/msg.h>
#include "main.h"
#include <arpa/inet.h>
#include <getopt.h>
int main(int argc, char *argv[] )
{
    if( argc != 0 ) {
        const char* short_options = "A:B:C:D:";
        const struct option long_options[] = {
                { "source", required_argument, NULL, 'A' },
                { "dest", required_argument, NULL, 'B' },
                { "source_port", required_argument, NULL, 'C' },
                { "dest_port", required_argument, NULL, 'D' },
                { NULL, 0, NULL, 0}
        };

        int rez = 0;
        while ((rez = getopt_long(argc, argv, short_options, long_options, NULL)) != -1) {
            switch (rez) {
                case 'A':
                    conf = 1;
                    s_check = 1;
                    if ( inet_pton(AF_INET, optarg, &(fsource.sin_addr)) <= 0 ) {
                        perror("Invalid IP in source argument");
                        return 0;
                    }
                    break;
                case 'B':
                    conf = 1;
                    d_check = 1;
                    if ( inet_pton(AF_INET, optarg, &(fdest.sin_addr)) <= 0 ) {
                        perror("Invalid IP in dest argument");
                        return 0;
                    }
                    printf("found argument \"b = %s\".\n", optarg);
                    break;
                case 'C':
                    conf = 1;
                    sp_check = 1;
                    fsource_port = atoi(optarg);
                    if ( fsource_port == 0 ) {
                        perror("Invalid port number in source_port argument");
                        return 0;
                    };
                    printf("found argument \"C = %d\".\n", fsource_port);
                    break;
                case 'D':
                    conf = 1;
                    dp_check = 1;
                    fdest_port = atoi(optarg);
                    if ( fdest_port == 0 ) {
                        perror("Invalid port number in dest_port argument");
                        return 0;
                    };
                    printf("found argument \"D = %d\".\n", fdest_port);
                    break;
                case '?':
                    printf("usage: udp-stat [ --source SOURCE_IP --dest DESTINATION_IP --source_port SOURCE_PORT --dest_port DESTINATION_PORT ] \n"
                           "source: IP address of source in format XXX.XXX.XXX.XXX \n"
                           "dest: IP address for destination format XXX.XXX.XXX.XXX  \n"
                           "source_port: source port\n"
                           "dest_port: destination port\n"
                           "Example: udp-stat --source 192.150.3.1 -source_port 80\n");
                    return 0;
            } // switch
        } // while
    }
    logfile = fopen("log.txt", "w");
    if ( logfile == NULL ) {
        printf("Unable to create log.txt file.\n");
    }
    printf("Starting...\n");

    key_t ipc_key;
    if ( (ipc_key = ftok("/tmp", 137)) == (key_t) -1 ) {
        perror("IPC error: ftok\n");
        exit(1);
    }
    if (( msqid = msgget(ipc_key, 0644 | IPC_CREAT)) == -1) {
        perror("msgget error\n");
        exit(1);
    }

    pthread_t sock_read, stat;

    int iret1 = pthread_create(&sock_read, NULL, netw, NULL);
    int iret2 = pthread_create(&stat, NULL, stat_collector, NULL);

    pthread_join(sock_read, NULL);
    pthread_join(stat, NULL);

    printf("Finished");
    return 0;
}


void *netw()
{
    int saddr_size;
    struct sockaddr saddr;

    unsigned char *buffer = (unsigned char *) malloc(65536);

    int sock_raw = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

    if ( sock_raw < 0 ) {
        perror("Socket Error\n");
    }

    while (1) {
        saddr_size = sizeof saddr;
        data_size = recvfrom(sock_raw, buffer, 65536, 0, &saddr,
                                    (socklen_t *) &saddr_size);
        if (data_size < 0) {
            perror("Recvfrom error , failed to get packets\n");
            exit(1);
        }

        struct iphdr *iph = (struct iphdr *) (buffer + sizeof(struct ethhdr));
        if (iph->protocol == 17) {
            if ( conf == 0 ) {
                struct ipc_msg nmsg = {2, {1, data_size}};
                if (msgsnd(msqid, &nmsg, sizeof(struct msg_data), 0) == -1) {
                    perror("Failure to transfer data\n");
                }
            }
            else {
                unsigned short iphdrlen = iph->ihl * 4;
                struct udphdr *udph = (struct udphdr *)(buffer + iphdrlen + sizeof(struct ethhdr));
                memset(&source, 0, sizeof(source));
                source.sin_addr.s_addr = iph->saddr;
                memset(&source, 0, sizeof(source));
                dest.sin_addr.s_addr = iph->daddr;

                if ( !( s_check == 1 && !(fsource.sin_addr.s_addr == source.sin_addr.s_addr))  &&
                        !( d_check == 1 && !(fdest.sin_addr.s_addr == dest.sin_addr.s_addr) ) &&
                        !( sp_check == 1 && !(fsource_port == ntohs(udph->source)) ) &&
                        !( dp_check == 1 && !(fdest_port == ntohs(udph->dest)) )){
                    struct ipc_msg nmsg = {2, {0, data_size}};
                    if (msgsnd(msqid, &nmsg, sizeof(struct msg_data), 0) == -1) {
                        perror("Failure to transfer data\n");
                    }
                }
            }
        }
    }
    close(sock_raw);
}

void *stat_collector()
{
    long total_size = 0;
    struct ipc_msg mark;

    while (1) {
        if (msgrcv(msqid, &mark, sizeof(struct msg_data), -3, 0) != -1) {
            if ( mark.mtype == 2 ) {
                ++udp;
                total_size = total_size + mark.datats.total_size;
            }
            else if ( mark.mtype == 3) {
                struct ipc_msg nmsg = {5, {udp, total_size}};
                if (msgsnd(msqid, &nmsg, sizeof(struct msg_data), 0) == -1) {
                    perror("Failure to transfer data\n");
                }
            }
        }
    }
}
