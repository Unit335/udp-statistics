#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <netinet/udp.h>
#include <netinet/ip.h>
#include <net/ethernet.h>
#include <sys/socket.h>
#include <unistd.h>
#include "main.h"
#include <arpa/inet.h>
#include <getopt.h>
#include <mqueue.h>
#include <fcntl.h>

#define SERVER_QUEUE_NAME   "/udp-server"
#define QUEUE_PERMISSIONS 0660
#define MAX_MESSAGES 10
#define MAX_MSG_SIZE 256
#define MSG_BUFFER_SIZE (MAX_MSG_SIZE + 10)

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
                    break;
                case 'C':
                    conf = 1;
                    sp_check = 1;
                    if ( fsource_port = atoi(optarg) ) {
                        perror("Invalid port number in source_port argument");
                        return 0;
                    };
                    break;
                case 'D':
                    conf = 1;
                    dp_check = 1;
                    if ( fdest_port = atoi(optarg) ) {
                        perror("Invalid port number in dest_port argument");
                        return 0;
                    };
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
    printf("Starting...\n");

    pthread_t sock_read, stat;
    int iret1, iret2;
    if ( (iret1 = pthread_create(&sock_read, NULL, netw, NULL)) != 0 ) {
        fprintf(stderr, "pthread error: socket, error code %d", iret1);
        exit(1);
    }
    if ( (iret2 = pthread_create(&stat, NULL, stat_collector, NULL)) != 0 ) {
        fprintf(stderr, "pthread error: statistics, error code %d", iret2);
        exit(1);
    }

    pthread_join(sock_read, NULL);
    pthread_join(stat, NULL);

    printf("Finished");
    return 0;
}


void *netw()
{
    int saddr_size;
    struct sockaddr saddr;
    int data_size;

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
            perror("Recvfrom error, failed to get packets\n");
            exit(1);
        }

        struct iphdr *iph = (struct iphdr *) (buffer + sizeof(struct ethhdr));
        if (iph->protocol == 17) {
            if ( conf == 0 ) {
                ++udp;
                total_size += data_size;
            }
            else {
                unsigned short iphdrlen = iph->ihl * 4;
                struct udphdr *udph = (struct udphdr *)(buffer + iphdrlen + sizeof(struct ethhdr));
                memset(&source, 0, sizeof(source));
                source.sin_addr.s_addr = iph->saddr;
                memset(&source, 0, sizeof(source));
                dest.sin_addr.s_addr = iph->daddr;

                if ( !( s_check == 1 && (fsource.sin_addr.s_addr != source.sin_addr.s_addr))  &&
                     !( d_check == 1 && (fdest.sin_addr.s_addr != dest.sin_addr.s_addr) ) &&
                     !( sp_check == 1 && (fsource_port != ntohs(udph->source)) ) &&
                     !( dp_check == 1 && (fdest_port != ntohs(udph->dest)) )){
                    ++udp;
                    total_size += data_size;
                }
            }
        }
    }
    close(sock_raw);
}

void *stat_collector()
{
    mqd_t qd_server, qd_client;

    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = MAX_MESSAGES;
    attr.mq_msgsize = MAX_MSG_SIZE;
    attr.mq_curmsgs = 0;

    if ( (qd_server = mq_open (SERVER_QUEUE_NAME, O_RDONLY | O_CREAT, QUEUE_PERMISSIONS, &attr) ) == -1) {
        perror ("mq_open error with qd_server");
        exit (1);
    }
    char in_buffer[MSG_BUFFER_SIZE];

    while (1) {
        if (mq_receive (qd_server, in_buffer, MSG_BUFFER_SIZE, NULL) != -1) {
            if ((qd_client = mq_open (in_buffer, O_WRONLY)) == 1) {
                perror ("Unable to open client queue");
                continue;
            }
            msgt out_buffer = {udp, total_size};;
            if (mq_send (qd_client, (const char *) &out_buffer, sizeof(out_buffer), 0) == -1) {
                perror ("Unable to send message to client");
                continue;
            }
        }
    }
}
