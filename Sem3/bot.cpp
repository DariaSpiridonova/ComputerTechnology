#include "bot.h"

int main(int argc, char* argv[])
{
    if (argc < 2) { printf("Использование: %s <номер чата>\n", argv[0]); return 1; }

    key_t shm_key = atoi(argv[1]);
    shm_key += SHM_KEY_BASE;                         // тот же ключ, что у клиентов

    int shmid = shmget(shm_key, SHM_MAXSIZE, IPC_CREAT | 0777);
    if (shmid < 0) { perror("shmget"); return 1; }

    ShmMessage* msgs = (ShmMessage*)shmat(shmid, NULL, 0);
    if (msgs == (ShmMessage*)(-1)) { perror("shmat"); return 1; }
    User* users = (User*)((char*)msgs + total_shm_size);

    printf("Бот запущен (чат %s)\n", argv[1]);

    while (1)
    {
        for (size_t i = 0; i < MAX_CHANNELS; i++)
        {
            // Нас интересуют только новые сообщения с указанным получателем
            if (msgs[i].is_read != 0 || msgs[i].recipient[0] == '\0') continue;

            // Получатель в сети? Тогда сообщение не трогаем
            if (IsOnline(users, msgs[i].recipient)) continue;

            // Получатель недоступен: запоминаем данные и забираем сообщение
            char sender[NAME_SIZE], recipient[NAME_SIZE];
            strcpy(sender, msgs[i].sender);
            strcpy(recipient, msgs[i].recipient);
            msgs[i].is_read = 1;

            // Не отвечаем на собственные сообщения (иначе возможен бесконечный цикл)
            if (strcmp(sender, BOT_NAME) == 0) continue;

            printf("Бот: %s недоступен (писал %s)\n", recipient, sender);

            // Ищем свободную ячейку для уведомления отправителю
            for (size_t j = 0; j < MAX_CHANNELS; j++)
            {
                if (msgs[j].is_read == 1 || msgs[j].recipient[0] == '\0')
                {
                    strcpy(msgs[j].sender, BOT_NAME);
                    strcpy(msgs[j].recipient, sender);
                    snprintf(msgs[j].text, TEXT_SIZE,
                             "Пользователь %s недоступен", recipient);
                    msgs[j].is_read = 0;             // флаг последним
                    break;
                }
            }
        }
        usleep(100000);                              // 100 мс, чтобы не грузить процессор
    }
}