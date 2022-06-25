//
// Created by root on 21.06.22.
//

#ifndef UNTITLED_MAIN_H
#define UNTITLED_MAIN_H

void *netw();

void *stat_collector();

FILE *logfile;
struct sockaddr_in source, dest;
int udp = 0, i, j;
long total_size;
int fsource_port = 0, fdest_port = 0
        ;
int data_size;
_Bool toRead = 0;
_Bool conf = 0;
pthread_mutex_t lock;

_Bool s_check = 0, d_check = 0, sp_check = 0, dp_check = 0;
struct sockaddr_in fsource;
struct sockaddr_in fdest;
struct ipc_msg {
    long mtype; /* must be positive */
    struct msg_data {
        int number;
        long total_size;
    } datats;
};

#endif //UNTITLED_MAIN_H
