

/* Программа, осуществляющая двунаправленную связь через pipe
между процессом-родителем и процессом-ребенком */
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main()
{
    int fd0[2], fd1[2], result;
    size_t size;
    char resstring[80];

    /* Создаем два pip'а */
    if(pipe(fd0) < 0)
    {
    printf("Can\'t create pipe 0\n");
    exit(-1);
    }

    if(pipe(fd1) < 0)
    {
    printf("Can\'t create pipe 1\n");
    exit(-1);
    }

    /* Порождаем новый процесс */
    result = fork();

    if(result < 0)
    {
        /* Если создать процесс не удалось, сообщаем об этом и завершаем работу */
        printf("Can\'t fork child\n");
        exit(-1);
    } 
    else if (result > 0) 
    {
        /* Мы находимся в родительском процессе. Закрываем ненужные потоки данных */
        close(fd0[0]);
        close(fd1[1]);
        /* Пишем в первый pipe и читаем из второго */
        size = write(fd0[1], "Привет, дитя!", 80);
        if(size != 80)
        {
            printf("Parent: can\'t write all string\n");
            exit(-1);
        }
        size = read(fd1[0], resstring, 80);

        if(size < 0)
        {
            printf("Parent: can\'t read string\n");
            exit(-1);
        }

        size = read(fd1[0], resstring, sizeof(resstring) - 1);
        if(size < 0)
        {
            printf("Parent: can't read string\n");
            exit(-1);
        }
        resstring[size] = '\0'; 

        printf("Parent: %s\n",resstring); 
        close(fd0[1]);
        close(fd1[0]);
        printf("Parent exit\n");
    } 
    else 
    {
        /* Мы находимся в порожденном процессе. Закрываем ненужные потоки данных */
        close(fd0[1]);
        close(fd1[0]);
        /* Читаем из первого pip'а и пишем во второй */
        size = read(fd0[0], resstring, sizeof(resstring) - 1);
        if(size < 0)
        {
            printf("Child: can\'t read string\n");
            exit(-1);
        }
        resstring[size] = '\0';
        
        printf("Child: %s\n",resstring);

        const char *msg = "Ну привет, папик!";
        size_t bytes_to_write = strlen(msg) + 1; 

        size = write(fd1[1], msg, bytes_to_write);

        if(size != bytes_to_write)
        {
            printf("Child: can\'t write all string\n");
            exit(-1);
        }

        close(fd0[0]);
        close(fd1[1]);
    }

    return 0;
} 

