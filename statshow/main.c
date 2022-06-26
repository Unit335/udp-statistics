#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <mqueue.h>
#include <fcntl.h>
#include <string.h>


#define SERVER_QUEUE_NAME   "/udp-server"
#define QUEUE_PERMISSIONS 0660
#define MAX_MESSAGES 10
#define MAX_MSG_SIZE 256
typedef struct msgt
{
    long numb;
    long w;
} msgt;

int main()
{
    char client_queue_name [64];
    mqd_t qd_server, qd_client;   

    sprintf (client_queue_name, "/udp-client-%d", getpid ());

    struct mq_attr attr;

    attr.mq_flags = 0;
    attr.mq_maxmsg = MAX_MESSAGES;
    attr.mq_msgsize = MAX_MSG_SIZE;
    attr.mq_curmsgs = 0;

    if ((qd_client = mq_open (client_queue_name, O_RDONLY | O_CREAT, QUEUE_PERMISSIONS, &attr)) == -1) {
        perror ("Client: mq_open (client)");
        exit (1);
    }

    if ((qd_server = mq_open (SERVER_QUEUE_NAME, O_WRONLY)) == -1) {
        perror ("Client: mq_open (server)");
        exit (1);
    }

    msgt in_buffer;
    printf("Packets received | Total size\n");
    while (1) {
        if (mq_send (qd_server, client_queue_name, strlen (client_queue_name) + 1, 0) == -1) {
            perror ("Client: Not able to send message to server");
            continue;
        }

        if (mq_receive (qd_client, (char *) &in_buffer, 256, NULL) == -1) {
            perror ("Client: mq_receive");
            exit (1);
        }
        printf("%-16ld | %-5ld \n", in_buffer.numb, in_buffer.w);
        sleep(2);
    }

    if (mq_close (qd_client) == -1) {
        perror ("Client: mq_close");
        exit (1);
    }

    if (mq_unlink (client_queue_name) == -1) {
        perror ("Client: mq_unlink");
        exit (1);
    }
    exit (0);
}
