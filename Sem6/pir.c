#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define MESSAGE_LENGTH 255
const size_t MESSAGE_BYTES  = MESSAGE_LENGTH + 2;  

struct mymsgbuf
{
    long mtype;
    char mtext[MESSAGE_LENGTH + 2]; // "+2" for '\0' and '\n'
};

int main(int args, char* argv[])
{
    if (args < 3)
    {
        printf("Usage: %s <num for receive messages> <number for send messages>\n", argv[0]);
        return 1;
    }

    char *endptr1, *endptr2;
    int rcp_num = (int)strtol(argv[1], &endptr1, 10);
    int snd_num = (int)strtol(argv[2], &endptr2, 10);

    if (*endptr1 != '\0' || *endptr2 != '\0')
    {
        printf("Usage: %s <num for receive messages> <number for send messages>\n", argv[0]);
        return 2;
    }

    struct mymsgbuf mybuf;

    int msqid = 0;
    char pathname[] = "key_generator.c";
    key_t key = 0;
    int len = 0, maxlen = 0;

    key = ftok(pathname, 0);
    if ((key = ftok(pathname, 0)) < 0) 
    {
        perror("ftok");
        exit(-1);
    }
    
    if ((msqid = msgget(key, 0666 | IPC_CREAT)) < 0)
    {
        printf("Can\'t get msqid\n");
        exit(-1);
    }

    pid_t pid = fork();

    if (pid < 0) 
    {
        // Error occurred
        perror("fork failed");
        exit(EXIT_FAILURE);
    } 
    else if (pid == 0) 
    {
        // Child process block
        printf("Child process: PID = %d, Parent PID = %d\n", getpid(), getppid());

        while (1) 
        {
            if ((len = msgrcv(msqid, (struct msgbuf *) &mybuf, MESSAGE_LENGTH + 3, rcp_num, 0)) < 0)
            {
                printf("Can\'t receive message from queue\n");
                exit(-1);
            }

            if (mybuf.mtype == 100)
            {
                msgctl(msqid, IPC_RMID, (struct msqid_ds *)NULL);
                exit(0);
            }
            printf("\nmessage type = %ld, info = %s\n", mybuf.mtype, mybuf.mtext);
        }    
    } 
    else 
    {
        // Parent process block
        printf("Parent process: PID = %d, Created Child PID = %d\n", getpid(), pid);
    
        char msg[MESSAGE_LENGTH];
        while(1)
        {
            printf("Please, enter a message not longer than %d symbols for send: ", MESSAGE_LENGTH);
            if (fgets(msg, sizeof(msg), stdin) == NULL || strcmp(msg, "exit\n") == 0) 
            {
                break; // Break out to clean up processes and queue
            }
            else
            {
                if (strchr(msg, '\n') != NULL)
                {
                    mybuf.mtype = snd_num;
                    strcpy(mybuf.mtext, msg);
                    len = strlen(mybuf.mtext) + 3;
                    
                    if (msgsnd(msqid, (struct msgbuf *) &mybuf, len, 0) < 0)
                    {
                        printf("Can\'t send message to queue\n");
                        msgctl(msqid, IPC_RMID, (struct msqid_ds *) NULL);
                        exit(-1);
                    }
                }
            }
        }
        printf("Cleaning up child process (PID %d)\n", pid);
        kill(pid, SIGTERM);      
        waitpid(pid, NULL, 0);
    }

    return 0;
}
