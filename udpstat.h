#ifndef UNTITLED_MAIN_H
#define UNTITLED_MAIN_H

#define PACKET_SIZE 65536 //maximum packet size

#define SERVER_QUEUE_NAME   "/udp-server"
#define QUEUE_PERMISSIONS 0660
#define MAX_MESSAGES 1
#define MAX_MSG_SIZE 16 //maximum size of sent data - two unsigned long variables
#define MAX_QUEUE_NAME 256
#define MSG_BUFFER_SIZE (MAX_MSG_SIZE + 10)

void closing_handler();
void parse_cmdline(int argc, char *argv[]);

void *packet_filtering_threadA();
void *stat_collectorA();
void *packet_filtering_threadB();
void *stat_collectorB();

void (*start_function) = &packet_filtering_threadA;
void (*stat_function) = &stat_collectorA;

_Bool snd_switch = 0;
int data_size;

_Bool conf = 0;
_Bool s_check = 0, d_check = 0, sp_check = 0, dp_check = 0;
int fsource_port = 0, fdest_port = 0;
struct sockaddr_in fsource;
struct sockaddr_in fdest;
char interface[16];

struct sockaddr_in source, dest;
int udp = 0;
long total_size = 0;

mqd_t qd_server, qd_client;
char in_buffer[MAX_QUEUE_NAME];
typedef struct msgt
{
    long message_counter;
    long current_size;
} msgt;

#endif //UNTITLED_MAIN_H
