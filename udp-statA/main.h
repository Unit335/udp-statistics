#ifndef UNTITLED_MAIN_H
#define UNTITLED_MAIN_H

void *netw();

void *stat_collector();

struct sockaddr_in source, dest;
int udp = 0;
int fsource_port = 0, fdest_port = 0;
int data_size;
_Bool conf = 0;
_Bool snd_switch = 0;

_Bool s_check = 0, d_check = 0, sp_check = 0, dp_check = 0;
struct sockaddr_in fsource;
struct sockaddr_in fdest;

typedef struct msgt
{
    long numb;
    long w;
} msgt;

#endif //UNTITLED_MAIN_H
