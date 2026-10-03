#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stdio.h>
#include <poll.h>
#include <stdlib.h>
#include <unistd.h>

/* Один ход.
   wait_num   - номер семафора, на котором я жду свою очередь
   signal_num - номер семафора соперника, которому отдаю ход
   Возвращает: 0 - игра продолжается,
               1 - я довёл число до 0 (убирать не мне),
               2 - соперник довёл до 0 (убирать мне),
              -1 - ошибка */
int ChangeValue(int sem_id, char *shm_buf, int *number, int wait_num, int signal_num)
{
    struct sembuf op;               /* одна заявка, массив не нужен */

    op.sem_num = wait_num;          /* к какому семафору относится заявка */
    op.sem_op  = -1;                /* ждать свою очередь */
    op.sem_flg = 0;
    if (semop(sem_id, &op, 1) < 0) { perror("semop wait"); return -1; }

    *number = atoi(shm_buf);        /* читаем число из памяти */
    if (*number <= 0) return 2;     /* соперник уже дошёл до 0 */

    (*number)--;                    /* наш ход */
    printf("Процесс %d: %d\n", (int)getpid(), *number);
    fflush(stdout);
    sleep(1);                       /* пауза для наглядности */
    sprintf(shm_buf, "%d", *number);

    op.sem_num = signal_num;        /* теперь заявка про семафор соперника */
    op.sem_op  = 1;                 /* отдаём ему ход */
    op.sem_flg = 0;
    if (semop(sem_id, &op, 1) < 0) { perror("semop signal"); return -1; }

    return (*number == 0) ? 1 : 0;
}

int main(void)
{
    int shm_id, sem_id;
    char *shm_buf;
    char pathname[] = "task5.cpp";
    key_t key;
    struct sembuf op;
    struct pollfd pfd;
    int number = 0;
    bool was_get = false;           /* true: я второй, число ввели не у меня */

    if ((key = ftok(pathname, 0)) < 0) { perror("ftok"); return 1; }

    /* 2 семафора: [0] - ход инициатора, [1] - ход второго */
    if ((sem_id = semget(key, 2, 0666 | IPC_CREAT)) < 0) { perror("semget"); return 1; }
    if ((shm_id = shmget(key, 16, 0666 | IPC_CREAT)) < 0) { perror("shmget"); return 1; }

    shm_buf = (char *) shmat(shm_id, NULL, 0);
    if (shm_buf == (char *) -1) { perror("shmat"); return 1; }

    pfd.fd = 0;
    pfd.events = POLLIN;

    printf("Введите число (или ждите, пока его введут в другом терминале)\n");

    while (1)
    {
        int event = poll(&pfd, 1, 100);
        if (event > 0)
        {
            if (scanf("%d", &number) == 1 && number > 0)
            {
                sprintf(shm_buf, "%d", number);   /* кладём число в память */

                /* отдаём ход ВТОРОМУ, то есть семафору №1 */
                op.sem_num = 1;
                op.sem_op  = 1;
                op.sem_flg = 0;
                if (semop(sem_id, &op, 1) < 0) { perror("semop"); return 1; }
                break;                            /* was_get остаётся false */
            }
            /* ввели не число - сбрасываем строку */
            int c;
            while ((c = getchar()) != '\n' && c != EOF) ;
            if (c == EOF) return 1;
        }
        else if (event == 0)
        {
            /* Смотрим значение семафора №1, не изменяя его:
               если там 1, значит, число уже ввели в другом терминале */
            if (semctl(sem_id, 1, GETVAL) > 0)
            {
                was_get = true;                   /* я второй */
                break;
            }
        }
        else { perror("poll"); return 1; }
    }

    /* Игра: инициатор ждёт семафор 0 и отдаёт семафору 1, второй наоборот */
    int wait_num   = was_get ? 1 : 0;
    int signal_num = was_get ? 0 : 1;
    int res;
    do {
        res = ChangeValue(sem_id, shm_buf, &number, wait_num, signal_num);
    } while (res == 0);

    shmdt(shm_buf);
    if (res == 2)                                 /* убирает только тот, кто проснулся последним */
    {
        semctl(sem_id, 0, IPC_RMID);
        shmctl(shm_id, IPC_RMID, NULL);
    }
    return 0;
}