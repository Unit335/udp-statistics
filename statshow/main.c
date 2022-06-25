#include <stdio.h>
#include <sys/msg.h>
#include <unistd.h>
#include <stdlib.h>

struct ipc_msg{
    long mtype;
    struct msg_data {
        int number;
        long total_size;
    } datats;
};

int main()
{
    struct ipc_msg rcvdt;
    int pmb = 1;
    key_t ipc_key;
    if ( (ipc_key = ftok("/tmp", 137)) == (key_t) -1 ) {
        perror("IPC error: ftok");
        exit(1);
    }
    int msqid;
    if (( msqid = msgget(ipc_key, 0644)) == -1 ) {
        perror("msgget");
        exit(1);
    }

    struct ipc_msg mssnd = {3, {}};
    for ( ; ; ) {
        msgsnd(msqid, &mssnd, sizeof(struct msg_data), 0);
        msgrcv(msqid, &rcvdt, sizeof(struct msg_data), 5, 0);
        printf("%d  ||| %ld \n", rcvdt.datats.number, rcvdt.datats.total_size);
        sleep(2);
    }
}
