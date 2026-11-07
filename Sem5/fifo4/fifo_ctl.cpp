/* Создаёт или удаляет трубы: ./fifo_ctl create | ./fifo_ctl remove */
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define REQ_FIFO "req.fifo"   /* клиент -> сервер (число) */
#define RES_FIFO "res.fifo"   /* сервер -> клиент (результат) */

/* Создаёт одну трубу; если она уже есть, это не ошибка */
static int make_fifo(const char *name)
{
    if (mknod(name, S_IFIFO | 0666, 0) < 0 && errno != EEXIST)
    {
        perror(name);
        return -1;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Использование: %s create | remove\n", argv[0]);
        return 1;
    }

    /* Обнуляем маску, чтобы права точно получились 0666 */
    (void)umask(0);

    if (strcmp(argv[1], "create") == 0)
    {
        if (make_fifo(REQ_FIFO) < 0 || make_fifo(RES_FIFO) < 0) return 1;
        printf("Трубы созданы\n");
    }
    else if (strcmp(argv[1], "remove") == 0)
    {
        /* unlink удаляет имя трубы из файловой системы */
        if (unlink(REQ_FIFO) < 0) perror(REQ_FIFO);
        if (unlink(RES_FIFO) < 0) perror(RES_FIFO);
        printf("Трубы удалены\n");
    }
    else
    {
        printf("Неизвестная команда: %s\n", argv[1]);
        return 1;
    }
    return 0;
}