

/* Проверить совместную работу с 4.
   Написать комментарии, В ТОМ ЧИСЛЕ К ПАРАМЕТРАМ!*/
#include <stdio.h>
#include <string.h>
#include <sys/shm.h>

#define SHMEM_SIZE	4096
#define SH_MESSAGE	"Poglad Kota!\n"

int main (int argc, char** argv)
{
  int shm_id {}; // IPC дескриптор для области разделяемой памяти 
  char* shm_buf = nullptr; // Указатель на разделяемую память
  int shm_size {}; // размер сегмента разделяемой памяти
  struct shmid_ds ds {}; // указатель на структуру shmid_ds, содержащую параметры сегмента
  
  // Пытаемся создать разделяемую память для сгенерированного ключа
  shm_id = shmget (IPC_PRIVATE, // выделяется уникальный, абсолютно новый кусок памяти, к которому никто не знает ключ 
                   SHMEM_SIZE,
  		           IPC_CREAT | IPC_EXCL | 0600);
  
  if (shm_id == -1) 
  {
    fprintf (stderr, "shmget() error\n");
    return 1;
  }
  
  // Пытаемся отобразить разделяемую память в адресное пространство текущего процесса
  shm_buf = (char *) shmat (shm_id,
                            NULL,
                            0);
  if (shm_buf == nullptr)
  {
    fprintf(stderr, "shmat() error\n");
  	return 1;
  }
  
  // запрашиваем у Linux технический паспорт разделяемой памяти
  shmctl (shm_id,
          IPC_STAT, // копирует информацию из структуры ядра в ds
          &ds);
  
  shm_size = ds.shm_segsz; // получаем размер сегмента разделяемой памяти
  // и сравниваем его с длиной сообщения
  if (shm_size < strlen (SH_MESSAGE)) 
  {
  	fprintf (stderr, "error: segsize=%d\n", shm_size);
  	return 1;
  }
  
  // Программа копирует строчку "Poglad Kota\n" (значение SH_MESSAGE) в общую оперативную память
  strcpy (shm_buf,
          SH_MESSAGE);
  
  // печатаем IPC дескриптор для области разделяемой памяти 
  printf ("ID:     %d\n", shm_id);
  printf ("String: %s\n", shm_buf);

  // для завершения программа ждёт нажатия <Enter>
  printf ("Press <Enter> to exit...");	
  fgetc (stdin);
  
  shmdt (shm_buf); // открепляем сегмент от процесса
  // удаляем сегмент из системы освобождаем память в ОС
  shmctl(shm_id,
         IPC_RMID,
         NULL);
  
  return 0;
}

