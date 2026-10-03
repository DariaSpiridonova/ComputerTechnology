#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stdio.h>

#define SEM_KEY 2007   /* Ключ семафора (тот же, что у писателя) */
#define SHM_KEY 2007   /* Ключ разделяемой памяти (тот же, что у писателя) */

int main(int argc, char **argv)
{
    int shm_id, sem_id;     /* IPC дескрипторы памяти и семафора */
    char *shm_buf;          /* Указатель на разделяемую память */
    struct sembuf sb[1];    /* Операция над семафором */

    /* Ищем уже существующий сегмент памяти (без IPC_CREAT) */
    shm_id = shmget(SHM_KEY, 1, 0600);
    if (shm_id == -1)
    {
        perror("shmget");
        return 1;
    }

    /* Ищем уже существующий набор из 1 семафора */
    sem_id = semget(SEM_KEY, 1, 0600);
    if (sem_id == -1)
    {
        perror("semget");
        return 1;
    }

    /* Подключаем память к адресному пространству процесса */
    shm_buf = (char *) shmat(shm_id, NULL, 0);
    if (shm_buf == (char *) -1)
    {
        perror("shmat");
        return 1;
    }

    /* Печатаем сообщение, записанное писателем */
    printf("Message: %s", shm_buf);

    /* Операция над семафором 0: +1, чтобы разбудить писателя */
    sb[0].sem_num = 0;
    sb[0].sem_op  = 1;
    sb[0].sem_flg = 0;
    if (semop(sem_id, sb, 1) == -1)
    {
        perror("semop");
        return 1;
    }

    /* Отсоединяем память от процесса */
    shmdt(shm_buf);

    return 0;
}