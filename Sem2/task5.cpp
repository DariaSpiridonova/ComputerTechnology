

/* Эта программа позволяет обмениваться сообщениями между терминалами.
  Произвести изменения. 
  1 расширить работу на N терминалов, добавив имя пользователя в параметры программы
  и имя получателя при вводе строки.
  2 Отметить какие моменты сильно влияют на надёжность работы программы*/
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

const size_t SHM_KEY_BASE = 0xDEADBABE;
const size_t SHM_MAXSIZE  = 1 << 20;
const size_t DELAY        = 10;
const size_t OFFSET       = 100;

const size_t MAX_CHANNELS = 16;
const size_t NAME_SIZE    = 32;
const size_t TEXT_SIZE    = 1024;

struct ShmMessage 
{
    char sender[NAME_SIZE];    // Имя того, КТО написал (передаем через argv[1])
    char recipient[NAME_SIZE]; // Имя того, КОМУ написали (вводится при отправке)
    char text[TEXT_SIZE];      // Сам текст сообщения
    int is_read;               // Флаг: 0 - новое, 1 - прочитано получателем
};

const size_t total_shm_size = sizeof(ShmMessage) * MAX_CHANNELS;

bool IsStringEmpty(char* str)
{
    while (*str == ' ') str++;
    if (*str == '\0' || *str == '\n') 
        return true;
    return false;
}

int main(int argc, char *argv[])
{
	if (argc < 2)
    {
        printf("User name is not specified.\n");
        printf("Required usage: %s <your_name>", argv[0]);

        return 1;
    }

	char* my_name = argv[1];

	printf(
		"--------------------\n"
		"SHM-Chat 0.1\n"
		"--------------------\n"
		"To send message just type it and press Enter. Another instance of SHM-Chat receives and displays it.\n"
		"--------------------\n"
		"stdin: %s\n"
		"stdout: %s\n"
		"--------------------\n"
		"Enter any number (chat is among instances with the same number):"
	, ttyname(fileno(stdin)), ttyname(fileno(stdout))); // get terminals names

	key_t shm_key;
	scanf("%d", &shm_key);
	shm_key += SHM_KEY_BASE;

	printf("--------------------\n\n");
	
	// check if more than one instance already running
	int shmid = shmget(shm_key, SHM_MAXSIZE, IPC_CREAT | 0777);		
	
	struct ShmMessage* shm_array = (struct ShmMessage*)shmat(shmid, NULL, 0);
	pid_t childpid;
	if ((childpid = fork()) == -1)
	{
		perror("fork error");
	}
	
	if (childpid == 0)
	{
		// child process: prints incoming messages
		// 1. Небольшая пауза при старте, чтобы Родитель гарантированно успел создать память через IPC_CREAT
        usleep(DELAY);
       
        // Отображаем разделяемую память как массив наших структур
        if (shm_array == (struct ShmMessage*)(-1)) {
            perror("Ошибка shmat в дочернем процессе");
            exit(1);
        }

		while (1)
		{
			for (size_t i = 0; i < MAX_CHANNELS; i++)
			{
				// Если сообщение адресовано нам И оно еще не прочитано (is_read == 0)
				if (strcmp(shm_array[i].recipient, my_name) == 0 && shm_array[i].is_read == 0)
				{
					// Выводим сообщение на экран
					printf("\n[Входящее от %s]: %s\n", shm_array[i].sender, shm_array[i].text);
					fflush(stdout);
	
					// Помечаем сообщение как прочитанное получателем
					shm_array[i].is_read = 1;
				}
			}
		}
            usleep(DELAY);

		shmdt (shm_array); // открепляем сегмент от процесса
		// удаляем сегмент из системы освобождаем память в ОС
	} 
	else
    {
        if (shm_array == (struct ShmMessage*)(-1)) {
            perror("Ошибка shmat в родительском процессе");
            return 1;
        }

        char input_buffer[TEXT_SIZE + NAME_SIZE + 2];

        for(;;)
        {
            fgets(input_buffer, sizeof(input_buffer), stdin);
            
            // Отрезаем перенос строки '\n' на конце
            input_buffer[strcspn(input_buffer, "\n")] = '\0';

            char *char_id_pointer = input_buffer;

            // Пропускаем пробелы
            while (*char_id_pointer == ' ') char_id_pointer++;
            if (*char_id_pointer == '\0') continue;

            // Выделяем имя получателя (до первого пробела)
            char *message_text = strchr(char_id_pointer, ' ');
            if (message_text == NULL) {
                // Если пробела нет, проверяем на команду выхода
                if (strcmp(char_id_pointer, "exit") == 0) {
                    printf("Выход из чата. Отключаемся...\n");
                    kill(childpid, SIGKILL); // Корректно завершаем ребенка
                    break;
                }
                printf("Ошибка: Используйте формат <имя_получателя> <сообщение>\n");
                continue;
            }
            
            // Разделяем строку на две независимые
            *(message_text++) = '\0';

			while (*message_text == ' ') message_text++;
            if (IsStringEmpty(message_text)) continue;

            // Ищем свободную ячейку в массиве разделяемой памяти для отправки
            int target_index = -1;
            for (size_t i = 0; i < MAX_CHANNELS; i++)
            {
                // Свободной считается ячейка, которая пуста или сообщение в ней уже прочитано
                if (shm_array[i].is_read == 1 || shm_array[i].recipient[0] == '\0')
                {
                    target_index = i;
                    break;
                }
            }

            if (target_index == -1)
            {
                printf("[Система]: Буфер чата переполнен! Подождите, пока сообщения прочитают.\n");
                continue;
            }

            // Формируем структуру и отправляем ее прямо в ОЗУ ячейки target_index
            strncpy(shm_array[target_index].sender, my_name, NAME_SIZE - 1);
            shm_array[target_index].sender[NAME_SIZE - 1] = '\0';

            strncpy(shm_array[target_index].recipient, char_id_pointer, NAME_SIZE - 1);
            shm_array[target_index].recipient[NAME_SIZE - 1] = '\0';

            strncpy(shm_array[target_index].text, message_text, TEXT_SIZE - 1);
            shm_array[target_index].text[TEXT_SIZE - 1] = '\0';

            // Сбрасываем флаг в 0 (сообщение не прочитано), зажигая светофор получателю
            shm_array[target_index].is_read = 0;

            printf("[Система]: Отправлено пользователю %s\n", char_id_pointer);
        }

        shmdt(shm_array);
	}

	return 0;
}

