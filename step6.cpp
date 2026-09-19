/* Создать программу, запускаемую на N терминалах, для обмена сообщениями между терминалами через файл.
Каждый экземпляр программы, запущенный на каждом терминале содержит два процесса:
один записывает файл, второй читает. Так на двух терминалах работает четыре процесса,
обеспечивая двусторонний обмен. 
Идентификатор пользователя (терминала), можно задавать параметром при запуске приложения.
Предусмотреть хранение информации для отсутствующего получателя.
Начать разработку со схемы данных. Предусмотреть поля для идентификаторов, отправителя и получателя,
а так же данных.
Разработать протокол обмена и структуру файла.
Продумать протокол очистки файла.
Исследовать вопрос с уникальностью идентификаторов.
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>  
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <wait.h>
#include <stddef.h>
#include <time.h>

const unsigned MAX_MESSAGES = 1000; // Критический лимит сообщений в файле
const unsigned MESSAGE_SIZE = 256;
const unsigned ID_SIZE      = 32;   // Максимальная длина имени пользователя
const unsigned FILE_NAME_SIZE = 256;
const char* CHAT_FILE = "chat.bin";
const char* TEMP_FILE = "chat_temp.bin";

struct Message 
{
    char sender[ID_SIZE];
    char recipient[ID_SIZE];
    char message[MESSAGE_SIZE];
    int is_read;
    int id; 
};

// Update the old file by moving the unread messages to the new chat_file
void UpdateChatFile() 
{
    printf("Запущена оптимизация файла...\n");
    
    // 1. Open a file only for reading
    int fd_old = open(CHAT_FILE, O_RDONLY);
    if (fd_old < 0) return; 

    // Exclusive Lock
    if (flock(fd_old, LOCK_EX) < 0) {
        close(fd_old);
        return;
    }

    // Create a new chat_file
    int fd_new = open(TEMP_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd_new < 0) {
        flock(fd_old, LOCK_UN);
        close(fd_old);
        return;
    }

    Message msg;
    int unread_count = 0;

    // 3. Move only unread messages
    while (read(fd_old, &msg, sizeof(Message)) == sizeof(Message)) 
    {
        if (msg.is_read == 0) 
        {
            msg.id = unread_count; 
            if (write(fd_new, &msg, sizeof(Message)) == sizeof(Message)) {
                unread_count++;
            }
        }
    }

    close(fd_new); 

    // Delete old chat.bin and rename chat.bin1 into chat.bin 
    rename(TEMP_FILE, CHAT_FILE);

    flock(fd_old, LOCK_UN);
    close(fd_old); 

    printf("Оптимизация завершена. Оставлено сообщений: %d\n", unread_count);
}

// A function for the parent to receive the current number of messages (= message_id) in the chat_buffer 
int get_next_id() 
{
    struct stat st;
    
    // Если файла еще нет (первый запуск), значит сообщений 0, и первый ID будет 0
    if (stat(CHAT_FILE, &st) < 0) {
        return 0; 
    }
    
    // Размер файла на диске делим на размер одной структуры
    int total_messages = st.st_size / sizeof(Message);

    if (total_messages == MAX_MESSAGES)
    {
        UpdateChatFile();
    }
    
    // Новое сообщение получит ID, равный текущему количеству
    // (например, если в файле 5 сообщений с ID от 0 до 4, то у шестого ID будет 5)
    return total_messages;
}

// In the case of chat_buffer overflow
void wait_for_free_space() 
{
    struct stat st;
    int first_time = 1;

    while (1) {
        // Если файла нет, значит он пуст — выходим из цикла ожидания
        if (stat(CHAT_FILE, &st) < 0) {
            break; 
        }

        // Считаем текущее количество сообщений
        int current_messages = st.st_size / sizeof(Message);

        if (current_messages < MAX_MESSAGES) {
            // Место есть! Выходим из цикла и разрешаем запись
            break; 
        }

        // Если мы здесь, значит файл переполнен
        if (first_time) {
            printf("\n[Система]: Превышен лимит сообщений. Запись временно недоступна.\n");
            printf("Ожидание освобождения места в файле...\n");
            fflush(stdout);
            first_time = 0; // Чтобы не спамить строкой в консоль каждую секунду
        }

        // Засыпаем на 2 секунды, чтобы не перегружать процессор постоянными проверками
        sleep(2); 
    }
}

bool IsStringEmpty(char** pointer)
{
    while (**pointer == ' ') (*pointer)++;
    if (**pointer == '\0' || **pointer == '\n') 
        return true;
    return false;
}

Message CreateStructure(const char *sender_str, const char *recipient_str, const char *text_str, int msg_id)
{
    Message msg; // Create a local structure

    // 1. Copy sender
    strncpy(msg.sender, sender_str, ID_SIZE - 1);
    msg.sender[ID_SIZE - 1] = '\0'; 

    // 2. Copy recipient
    strncpy(msg.recipient, recipient_str, ID_SIZE - 1);
    msg.recipient[ID_SIZE - 1] = '\0';

    // 3. Copy message
    strncpy(msg.message, text_str, MESSAGE_SIZE - 1);
    msg.message[MESSAGE_SIZE - 1] = '\0';

    // 4. Setting the values of the numeric flags
    msg.is_read = 0;
    msg.id = msg_id;

    return msg; 
}

void WriteMessageToFile(const char *filename, Message *msg) 
{
    int fd = open(filename, O_RDWR | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

    if (fd < 0) 
    {
        perror("Ошибка при открытии файла для записи");
        return;
    }

    // 2. LOCK_EX - Exclusive lock
    // waiting for permission to lock the file
    if (flock(fd, LOCK_EX) < 0) {
        perror("Ошибка блокировки файла");
        close(fd);
        return;
    }

    // 3. Write to a file
    size_t message_size = sizeof(Message);
    ssize_t bytes_written = write(fd, msg, sizeof(Message));
    
    if (bytes_written < 0) 
    {
        printf("Ошибка при записи структуры в файл");
        exit(1);
    } 
    else if (bytes_written < sizeof(Message))
    {
        printf("There was written %zd bytes in file instead of %zu bytes\n", bytes_written, sizeof(Message));
        exit(2);
    }
    else 
    {
        printf("Сообщение успешно отправлено!\n");
    }

    // 4. unlocking and closing a file.
    flock(fd, LOCK_UN);
    close(fd);
}

int main(int argc, char *argv[], char *envp[])
{
    if (argc < 2)
    {
        printf("Incorrect program launch\n");
        printf("Require: <program's name> <console id>\n");
    }

    char *console_id = argv[1];

    pid_t chpid = fork();

    char message_buffer[MESSAGE_SIZE] = {};

    if (chpid == -1)
    { 
        printf("Can\'t fork a child process\n");
        
    } 
    // parent
    else if (chpid > 0)
    {
        unsigned message_id = 0;

        // receive messages for other consoles until the user interrupts the input with the <exit> command
        while(1) 
        {
            // 1. Проверяем, не переполнен ли файл. Если переполнен — стоим тут и ждем
            wait_for_free_space();

            // 2. Если место освободилось (или и так было), запрашиваем ввод
            printf("Enter the message in the following format: <recipient console id> <message>\n");
            fgets(message_buffer, MESSAGE_SIZE, stdin);
            
            message_id = get_next_id();
            char *char_id_pointer = message_buffer;

            if (IsStringEmpty(&char_id_pointer)) continue;
            char *message = strchr(char_id_pointer, ' ');
            if (message == NULL) 
                continue;
            else
                *(message++) = '\0';

            if (IsStringEmpty(&message)) continue;
            
            // create a structure for current message
            Message msg = CreateStructure(argv[1], char_id_pointer, message, message_id);

            WriteMessageToFile(CHAT_FILE, &msg);
        }

    } 
    // child
    else 
    {
        while (1) 
        {
            int fd = open(CHAT_FILE, O_RDWR);
            // Ставим разделяемую блокировку (LOCK_SH) — читать файл могут несколько терминалов одновременно,
            // но пока мы читаем, родительские процессы не смогут в него писать, что защищает от ошибок.
            
            Message msg;
            off_t current_offset = 0;
            
            // Читаем файл от начала до конца блоками по sizeof(Message)
            // Функция read сама автоматически сдвигает указатель файла на размер структуры после каждого чтения!
            flock(fd, LOCK_SH);
            while (read(fd, &msg, sizeof(Message)) == sizeof(Message)) 
            {
                // Проверяем: адресовано ли сообщение этой консоли и не прочитано ли оно (is_read == 0)
                if (strcmp(msg.recipient, console_id) == 0 && msg.is_read == 0) 
                {
                    // 1. Выводим входящее сообщение на экран
                    printf("\n[Входящее от %s]: %s\n", msg.sender, msg.message);
                    fflush(stdout);

                    // 2. Теперь нужно изменить статус на 1.
                    // Для этого нам временно нужна эксклюзивная блокировка (LOCK_EX)
                    flock(fd, LOCK_EX);

                    // Сдвигаем указатель файла назад на место, где лежит поле is_read текущей структуры.
                    // offsetof(Message, is_read) — это встроенный макрос Си, который сам знает точное смещение поля!
                    lseek(fd, current_offset + offsetof(Message, is_read), SEEK_SET);

                    int new_status = 1;
                    write(fd, &new_status, sizeof(int));

                    // Возвращаем разделяемую блокировку для продолжения чтения
                    flock(fd, LOCK_SH);
                }

                // Запоминаем, где начнется следующая структура (смещение увеличивается на размер сообщения)
                current_offset += sizeof(Message);
                
                // Перемещаем указатель файла на начало следующей записи (на всякий случай после нашей дозаписи статуса)
                lseek(fd, current_offset, SEEK_SET);
            }
            flock(fd, LOCK_UN);
            
            // unlocking and closing a file.
            close(fd);
            sleep(1);
        }    
    }

    return 0;
}