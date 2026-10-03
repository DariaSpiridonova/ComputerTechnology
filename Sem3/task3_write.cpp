#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stdio.h>
#include <string.h>

#define SHMEM_SIZE  4096              /* Размер разделяемой памяти */
#define SH_MESSAGE  "Hello World!\n"  /* Сообщение для читателя */

#define SEM_KEY     2007              /* Ключ семафора */
#define SHM_KEY     2007              /* Ключ разделяемой памяти */

/* Объединение для передачи параметров в semctl (в Linux нужно объявлять самому) */
union semnum
{
    int val;
    struct semid_ds *buf;
    unsigned short *array;
} sem_arg;

int main(void)
{
    int shm_id, sem_id;            /* shm_id - IPC дескриптор разделяемой памяти,
                                  sem_id - IPC дескриптор набора семафоров
                                  (числа, которые вернут shmget и semget) */
    char *shm_buf;                 /* Указатель на разделяемую память в нашем процессе
                                    (результат shmat), через него пишем сообщение */
    int shm_size;                  /* Реальный размер сегмента в байтах,
                                    узнаём через shmctl(IPC_STAT) */
    struct shmid_ds ds;            /* Структура с информацией о сегменте памяти
                                    (размер, владелец, права и т.д.),
                                    её заполняет shmctl(IPC_STAT) */
    struct sembuf sb[1];           /* Массив из одной операции над семафором:
                                    какой семафор, что сделать (-1 / +1), флаги */
    unsigned short sem_vals[1];    /* Массив начальных значений семафоров набора
                                    (у нас один семафор), передаётся в semctl(SETALL) */
    
                                    /* Создаём разделяемую память; IPC_EXCL - ошибка, если она уже существует */
    shm_id = shmget(SHM_KEY, SHMEM_SIZE, IPC_CREAT | IPC_EXCL | 0600);
    if (shm_id == -1)
    {
        perror("shmget");
        return 1;
    }

    /* Создаём набор из 1 семафора, также только если его ещё нет */
    sem_id = semget(SEM_KEY, 1, 0600 | IPC_CREAT | IPC_EXCL);
    if (sem_id == -1)
    {
        perror("semget");
        return 1;
    }

    printf("Semaphore: %d\n", sem_id);

    /* Устанавливаем начальное значение семафора, равное 1 */
    sem_vals[0] = 1;
    sem_arg.array = sem_vals;
    if (semctl(sem_id, 0, SETALL, sem_arg) == -1) // для SETALL игнорируется, т.к. команда затрагивает все семафоры набора (хотя у нас всё равно один)
    {
        perror("semctl SETALL");
        return 1;
    }

    /* Подключаем память к адресному пространству процесса */
    shm_buf = (char *) shmat(shm_id, NULL, 0);
    if (shm_buf == (char *) -1)
    {
        perror("shmat");
        return 1;
    }

    /* Узнаём реальный размер сегмента и проверяем, что сообщение поместится
       (+1 байт под завершающий '\0') */
    shmctl(shm_id, IPC_STAT, &ds);
    shm_size = ds.shm_segsz;
    if (shm_size < (int)strlen(SH_MESSAGE) + 1)
    {
        fprintf(stderr, "error: segsize=%d\n", shm_size);
        return 1;
    }

    /* Копируем сообщение В разделяемую память (куда, откуда) */
    strcpy(shm_buf, SH_MESSAGE);

    printf("ID: %d\n", shm_id);

    /* Операции над семафором 0; SEM_UNDO - откат при аварийном завершении */
    sb[0].sem_num = 0;
    sb[0].sem_flg = SEM_UNDO;

    /* Первое -1: семафор 1 -> 0, проходим без ожидания */
    sb[0].sem_op = -1;
    if (semop(sem_id, sb, 1) == -1)
    {
        perror("semop 1");
        return 1;
    }

    /* Второе -1: семафор равен 0, засыпаем, пока читатель не сделает +1 */
    sb[0].sem_op = -1;
    if (semop(sem_id, sb, 1) == -1)
    {
        perror("semop 2");
        return 1;
    }

    /* Читатель закончил: удаляем семафор, отсоединяем и удаляем память */
    semctl(sem_id, 0, IPC_RMID, sem_arg);
    shmdt(shm_buf);
    shmctl(shm_id, IPC_RMID, NULL);

    return 0;
}