/* Программа, иллюстрирующая использование системных вызовов open(), read() и close() для чтения информации из файла */

#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <wait.h>

int main(int argc, char *argv[], char *envp[])
{
    int fd;
    int fd1;
    size_t size;
    char string[61];

    /* Попытаемся открыть файл с именем в первом параметре выззова только
    для операций чтения */

    if ((fd = open("/home/dasha/file1.txt", O_RDONLY)) < 0)
    {

        /* Если файл открыть не удалось, печатаем об этом сообщение и прекращаем работу */
        printf("Can\'t open file\n");
        exit(-1);
    }

    /* A file for writing a copy of a file with the fd descriptor */
    if ((fd1 = open("/home/dasha/file2.txt", O_RDWR)) < 0)
    {

        /* Если файл открыть не удалось, печатаем об этом сообщение и прекращаем работу */
        printf("Can\'t open file\n");
        exit(-1);
    }

    ssize_t bytes_written = 0;
    /* Читаем фаил пока не кончится и печатаем */
    while ((size = read(fd, string, 60)) > 0)
    {
        string[size] = '\0';
        printf("%s\n", string); /* Печатаем прочитанное*/

        bytes_written = write(fd1, string, strlen(string));
        if (bytes_written == -1) 
        {
            perror("Error writing to file");
            close(fd); // Ensure file is closed even if writing fails
            return 1;
        }
    };

    if (size < 0)
    {
        printf("An error occurred: %s (errno: %d)\n", strerror(errno), errno);
        exit(-2);
    }

    /* Закрываем файлы*/
    if (close(fd) < 0 || close(fd1) < 0)
    {
        printf("Failed to close both files successfully\n");
    }

    fflush(stdout);
    pid_t chpid = fork();

    if (chpid < 0)
    {
        printf("Error\n");
    } 

    else if (chpid == 0)
    {
        int result = execle("/bin/less", "/bin/less", "/home/dasha/file2.txt", NULL, envp);

        perror("Execle error");
        exit(-3);
    }

    else 
    {
        pid_t pid;
        int status;
        
        if ((pid = waitpid(chpid, &status, 0)) == -1)
        {
            printf("Error\n");
            exit(-4);
        }
    }

    return 0;
} 
