#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[], char *envp[])
{
    int semid;                  /* IPC дескриптор набора семафоров */
    char pathname[] = "task1_wait.cpp"; /* Файл для генерации ключа */
    key_t key;                  /* IPC ключ */
    struct sembuf mybuf;        /* Структура операции над семафором */

    /* Генерируем IPC ключ из имени файла и номера экземпляра 0 */
    if((key = ftok(pathname, 0)) < 0)
    {
        printf("Can't generate key\n");
        exit(-1);
    }

    /* Получаем набор из 1 семафора; если его нет - создаём
        (начальное значение семафора 0) */
    if((semid = semget(key, 1, 0666 | IPC_CREAT)) < 0)
    {
        printf("Can't create semaphore set\n");
        exit(-1);
    }

    /* Операция над семафором номер 0: уменьшить значение на 1.
        Если значение 0 - процесс блокируется до его увеличения */
    mybuf.sem_num = 0;
    mybuf.sem_op  = -1;
    mybuf.sem_flg = 0;

    if(semop(semid, &mybuf, 1) < 0)
    {
        printf("Can't wait for condition\n");
        exit(-1);
    }

    printf("The condition is present\n");
    return 0;
}