/* Сервер: читает число из req.fifo, возводит в квадрат, отправляет в res.fifo */
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
    int number;             /* полученное число */
    long long square;       /* результат */

    /* Если клиент пропал, write вернёт ошибку, а не убьёт программу сигналом */
    signal(SIGPIPE, SIG_IGN);

    printf("Сервер: жду клиента...\n");

    if ((fd_req = open(REQ_FIFO, O_RDONLY)) < 0)
    {
        perror("open req.fifo (запустите ./fifo_ctl create)");
        return 1;
    }
    if ((fd_res = open(RES_FIFO, O_WRONLY)) < 0)
    {
        perror("open res.fifo");
        return 1;
    }

    printf("Сервер: клиент подключился\n");

    while (1)
    {
        ssize_t n = read(fd_req, &number, sizeof(number));

        if (n == 0)                    /* клиент закрыл трубу: конец работы */
        {
            printf("Сервер: клиент отключился, завершаю работу\n");
            break;
        }
        if (n < 0)
        {
            perror("read");
            break;
        }
        if (n != sizeof(number))       /* пришло меньше байт, чем нужно */
        {
            printf("Сервер: неполные данные\n");
            break;
        }

        square = (long long)number * number;
        printf("Сервер: %d^2 = %lld\n", number, square);

        /* Возвращаем результат клиенту через вторую трубу */
        if (write(fd_res, &square, sizeof(square)) != sizeof(square))
        {
            perror("write");
            break;
        }
    }

    close(fd_req);
    close(fd_res);
    return 0;
}