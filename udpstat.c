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
#include <arpa/inet.h>
#include <getopt.h>
#include <mqueue.h>
#include <linux/if_packet.h>
#include <fcntl.h>
#include <linux/if.h>
#include <signal.h>
#include <sys/ioctl.h>
#include "udpstat.h"

void closing_handler() 
{
    printf("\nClosing...\n");
    pthread_cancel(sock_read);
    pthread_cancel(stat);
}

int main(int argc, char *argv[] )
{
    signal(SIGINT, closing_handler);

    if ( parse_cmdline(argc, argv) == 1)
        return 1;
    printf("Starting...\n");

    int retcode, retcode_stat;
    if ((retcode = pthread_create(&sock_read, NULL, start_function, NULL)) != 0 ) {
        fprintf(stderr, "pthread_create (sock_read): (%d)%s\n", retcode, strerror(retcode));
        return 1;
    }
    if ((retcode = pthread_create(&stat, NULL, stat_function, NULL)) != 0 ) {
        fprintf(stderr, "pthread_create (stat): (%d)%s\n", retcode, strerror(retcode));
        return 1;
    }
    int main_retcode = 0;
    void *socket_retcode;
    void *stat_retcode;
    pthread_join(sock_read, &socket_retcode);
    pthread_join(stat, &stat_retcode);
    if ( (intptr_t)socket_retcode == 1 )
        main_retcode = 1;
    if ( (intptr_t)stat_retcode == 1 )
        main_retcode = 1;
    return main_retcode;
}


void *packet_filtering_threadA()
{
    int saddr_size;
    struct sockaddr saddr;
    unsigned char *buffer = (unsigned char *) malloc(PACKET_SIZE);
    pthread_cleanup_push(free, buffer);

    sock_raw = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if ( sock_raw < 0 ) {
        perror("Socket Error\n");
        pthread_exit((void *)1);
    }
    void *pointer = &sock_raw;
    pthread_cleanup_push(socket_close, NULL);

    if ( interface[0] != '\0' ) {
        struct ifreq ifr;
        setsockopt(sock_raw, SOL_SOCKET, SO_BINDTODEVICE, "", 0);
        memset(&ifr, 0, sizeof(ifr));
        snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), interface);
        if (setsockopt(sock_raw, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr)) == -1) {
            perror("Failure to bind socket to interface");
            pthread_exit((void *)1);
        }
    }
    while (run_switch) {
        saddr_size = sizeof saddr;

        data_size = recvfrom(sock_raw, buffer, PACKET_SIZE, 0, &saddr,
                             (socklen_t *) &saddr_size);
        if (data_size < 0) {
            perror("Recvfrom error, failed to get packets\n");
            pthread_exit((void *)1);
        }

        struct iphdr *iph = (struct iphdr *) (buffer + sizeof(struct ethhdr));
        if ( iph->protocol == 17 ) {
            if ( conf == 0 ) {
                snd_switch = 1;
            }
            else {
                unsigned short iphdrlen = iph->ihl * 4;
                struct udphdr *udph = (struct udphdr *)(buffer + iphdrlen + sizeof(struct ethhdr));
                memset(&source, 0, sizeof(source));
                source.sin_addr.s_addr = iph->saddr;
                memset(&dest, 0, sizeof(dest));
                dest.sin_addr.s_addr = iph->daddr;
                uint16_t a = ntohs(udph->source);
                if ( ntohs(udph->source) == (uint16_t)5566) {
                    printf("1");
                }
                if ( !( s_check == 1 && (fsource.sin_addr.s_addr != source.sin_addr.s_addr))  &&
                     !( d_check == 1 && (fdest.sin_addr.s_addr != dest.sin_addr.s_addr) ) &&
                     !( sp_check == 1 && (fsource_port != ntohs(udph->source)) ) &&
                     !( dp_check == 1 && (fdest_port != ntohs(udph->dest)) )){
                    snd_switch = 1;
                }
            }
        }
    }
    pthread_cleanup_pop(1);
    pthread_cleanup_pop(1);
    pthread_exit((void *)0);
}

void *stat_collectorA()
{
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = MAX_MESSAGES;
    attr.mq_msgsize = MAX_QUEUE_NAME;
    attr.mq_curmsgs = 0;

    if ( (qd_server = mq_open (SERVER_QUEUE_NAME, O_RDONLY | O_CREAT | O_NONBLOCK, QUEUE_PERMISSIONS, &attr) ) == -1) {
        perror ("mq_open error with qd_server");
        pthread_exit((void *)1);
    }
    pthread_cleanup_push(mqueues_close, NULL);

    long total_size = 0;
    while (run_switch) {
        if (mq_receive (qd_server, in_buffer, MAX_QUEUE_NAME, NULL) != -1) {
            if ((qd_client = mq_open(in_buffer, O_WRONLY)) == 1) {
                perror("Unable to open client queue");
                continue;
            }
            msgt out_buffer = {udp, total_size};
            if (mq_send (qd_client, (const char *) &out_buffer, sizeof(out_buffer), 0) == -1) {
                perror ("Unable to send message to client");
                continue;
            }
        }

        if ( snd_switch == 1 ) {
            ++udp;
            total_size += data_size;
            snd_switch = 0;
        }
    }
    pthread_cleanup_pop(1);
    pthread_exit((void *)0);
}

void *packet_filtering_threadB()
{

    int saddr_size;
    struct sockaddr saddr;
    unsigned char *buffer = (unsigned char *) malloc(PACKET_SIZE);
    pthread_cleanup_push(free, buffer);

    sock_raw = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if ( sock_raw < 0 ) {
        perror("Socket Error");
        pthread_exit((void *)1);
    }
    pthread_cleanup_push(socket_close, NULL);

    if ( interface[0] != '\0' ) {
        struct ifreq ifr;
        setsockopt(sock_raw, SOL_SOCKET, SO_BINDTODEVICE, "", 0);
        memset(&ifr, 0, sizeof(ifr));
        snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), interface);
        if (setsockopt(sock_raw, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr)) == -1) {
            perror("Failure to bind socket to interface");
            pthread_exit((void *)1);
        }
    }

    while (run_switch) {
        saddr_size = sizeof saddr;
        data_size = recvfrom(sock_raw, buffer, PACKET_SIZE, 0, &saddr,
                             (socklen_t *) &saddr_size);
        if (data_size < 0) {
            perror("Recvfrom error, failed to get packets");
            pthread_exit((void *)1);
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
                memset(&dest, 0, sizeof(dest));
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
    pthread_cleanup_pop(1);
    pthread_cleanup_pop(1);
    pthread_exit((void *)0);
}


void *stat_collectorB()
{
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = MAX_MESSAGES;
    attr.mq_msgsize = MAX_QUEUE_NAME;
    attr.mq_curmsgs = 0;

    if ( (qd_server = mq_open (SERVER_QUEUE_NAME, O_RDONLY | O_CREAT, QUEUE_PERMISSIONS, &attr) ) == -1) {
        perror ("mq_open error with qd_server");
        run_switch = 0;
        pthread_exit((void *)1);
    }
    pthread_cleanup_push(mqueues_close, NULL);
    while (run_switch) {
        if ( mq_receive (qd_server, in_buffer, MAX_QUEUE_NAME, NULL) != -1 ) {
            if ( (qd_client = mq_open (in_buffer, O_WRONLY) ) == 1) {
                perror ("Unable to open client queue");
                continue;
            }
            msgt out_buffer = {udp, total_size};
            if ( mq_send (qd_client, (const char *) &out_buffer, sizeof(out_buffer), 0) == -1 ) {
                perror ("Unable to send message to client");
                continue;
            }
        }
    }
    pthread_cleanup_pop(1);
    pthread_exit((void *)0);
}

int parse_cmdline(int argc, char *argv[])
{
    if( argc != 0 ) {
        const char* short_options = "ABR:T:Y:U:";
        const struct option long_options[] = {
                { "version-A", no_argument, NULL, 'A'},
                { "version-B", no_argument, NULL, 'B'},
                { "interface", required_argument, NULL, 'I'},
                { "source", required_argument, NULL, 'R' },
                { "dest", required_argument, NULL, 'T' },
                { "source_port", required_argument, NULL, 'Y' },
                { "dest_port", required_argument, NULL, 'U' },
                { NULL, 0, NULL, 0}
        };
        int rez = 0;
        while ((rez = getopt_long(argc, argv, short_options, long_options, NULL)) != -1) {
            switch (rez) {
                case 'A':
                    printf("Starting ver A\n");
                    start_function = &packet_filtering_threadA;
                    stat_function = &stat_collectorA;
                    break;
                case 'B':
                    printf("Starting ver B\n");
                    start_function = &packet_filtering_threadB;
                    stat_function = &stat_collectorB;
                    break;
                case 'I':
                    if ( (sizeof(interface) - 1) > 15 ) {
                        perror("Invalid interface name: string too long");
                        return 1;
                    }
                    strncpy(interface, optarg, 16);
                    break;
                case 'R':
                    conf = 1;
                    s_check = 1;
                    if ( inet_pton(AF_INET, optarg, &(fsource.sin_addr)) <= 0 ) {
                        perror("Invalid IP in source argument");
                        return 1;
                    }
                    break;
                case 'T':
                    conf = 1;
                    d_check = 1;
                    if ( inet_pton(AF_INET, optarg, &(fdest.sin_addr)) <= 0 ) {
                        perror("Invalid IP in dest argument");
                        return 1;
                    }
                    break;
                case 'Y':
                    conf = 1;
                    sp_check = 1;
                    if ( (fsource_port = atoi(optarg)) == 0 ) {
                        perror("Invalid port number in source_port argument");
                        return 1;
                    };
                    break;
                case 'U':
                    conf = 1;
                    dp_check = 1;
                    if ( (fdest_port = atoi(optarg)) == 0 ) {
                        perror("Invalid port number in dest_port argument");
                        return 1;
                    };
                    break;
                case '?':
                    printf("usage: udp-stat [ -A/B --interface INTERFACE--source SOURCE_IP --dest DESTINATION_IP --source_port SOURCE_PORT --dest_port DESTINATION_PORT ] \n"
                           "interface: name of network interface \n"
                           "source: IP address of source in format XXX.XXX.XXX.XXX \n"
                           "dest: IP address for destination format XXX.XXX.XXX.XXX  \n"
                           "source_port: source port\n"
                           "dest_port: destination port\n"
                           "Example: udp-stat -A --interface enp0s3 --source 192.150.3.1 -source_port 80\n");
                    return 1;
            }
        }
    }
}

void socket_close() { close(sock_raw); }
void mqueues_close()
{
    if ( mq_close(qd_server) == -1) {
        perror("mq_close (qd_close) error: ");
    }
    if ( mq_close(qd_client) == -1 )
        perror("mq_close (qd_client) error: ");
    if ( mq_unlink(SERVER_QUEUE_NAME) == -1 )
        perror("mq_unlink (qd_client) error: ");
}
