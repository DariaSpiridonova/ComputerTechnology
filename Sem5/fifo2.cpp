

/* Программа для чтения из  FIFO */
/* Для отладки использовать утилиту strace: strace -e trace=open,read ./имя программы */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main()
{
int fd;
size_t size;
char resstring[64];
char name[]="aaa.fifo";

/* Открываем FIFO на чтение.*/
if((fd = open(name, O_RDONLY)) < 0)
{
  /* Если открыть FIFO не удалось, 
  печатаем об этом сообщение и прекращаем работу */
  printf("Can\'t open FIFO for reading\n");
  exit(-1);
}

/* Пробуем прочитать из FIFO 64 байта в массив,
т.е. заполнить весь доступный буфер */
size = read(fd, resstring, 64);
if(size < 0)
{
  /* Если прочитать не смогли, сообщаем об ошибке и завершаем работу */
  printf("Can\'t read string\n");
  exit(-1);
}

/* Печатаем прочитанную строку */
printf("Resstring: %s\n",resstring);
// Уведомление о том что сессия связи окончена
close(fd);
return 0;
} 

