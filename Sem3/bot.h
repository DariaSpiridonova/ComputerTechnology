#ifndef BOT_H
#define BOT_H

#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>

const size_t SHM_KEY_BASE = 0xDEADBABE;
const size_t SHM_MAXSIZE  = 1 << 20;

const size_t DELAY        = 10;
const size_t OFFSET       = 100;

const size_t MAX_CHANNELS = 16;
const size_t NAME_SIZE    = 32;
const size_t TEXT_SIZE    = 1024;

struct ShmMessage
{
    char sender[NAME_SIZE];
    char recipient[NAME_SIZE];
    char text[TEXT_SIZE];
    int is_read;
};

struct User
{
    char name[NAME_SIZE];
    int  pid;
};

const size_t total_shm_size = sizeof(ShmMessage) * MAX_CHANNELS;
const char* BOT_NAME = "BOT";

// Жив ли процесс: сигнал 0 ничего не посылает, а только проверяет процесс
inline bool IsAlive(int pid)
{
    if (pid <= 0) return false;
    return kill(pid, 0) == 0 || errno == EPERM;
}

// Доступен ли пользователь с таким именем
inline bool IsOnline(User* users, const char* name)
{
    for (size_t i = 0; i < MAX_CHANNELS; i++)
        if (IsAlive(users[i].pid) && strcmp(users[i].name, name) == 0)
            return true;
    return false;
}

#endif