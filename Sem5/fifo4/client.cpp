/* Клиент: читает число с терминала, шлёт серверу, печатает результат.
   Если введено не число, завершает работу (сервер завершится сам). */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#define REQ_FIFO "req.fifo"
#define RES_FIFO "res.fifo"

int main()
{
    int fd_req, fd_res;
    int number;
    long long square;

    signal(SIGPIPE, SIG_IGN);

    if ((fd_req = open(REQ_FIFO, O_WRONLY)) < 0)
    {
        perror("open req.fifo (запустите ./fifo_ctl create)");
        return 1;
    }
    if ((fd_res = open(RES_FIFO, O_RDONLY)) < 0)
    {
        perror("open res.fifo");
        return 1;
    }

    printf("Вводите целые числа. Что-то другое (или Ctrl+D) - выход.\n");

    while (1)
    {
        printf("Число: ");
        fflush(stdout);

        /* scanf вернёт 1 только если прочитано целое число */
        if (scanf("%d", &number) != 1)
        {
            printf("Введено не число, завершаю работу\n");
            break;
        }

        if (write(fd_req, &number, sizeof(number)) != sizeof(number))
        {
            printf("Не удалось отправить число (сервер недоступен?)\n");
            break;
        }

        /* Ждём ответ от сервера */
        ssize_t n = read(fd_res, &square, sizeof(square));
        if (n != sizeof(square))
        {
            printf("Сервер не ответил\n");
            break;
        }

        printf("Результат: %d^2 = %lld\n", number, square);
    }

    /* Закрытие req.fifo даёт серверу конец файла (read вернёт 0) */
    close(fd_req);
    close(fd_res);
    return 0;
}