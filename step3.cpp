#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char *argv[], char *envp[])
{
    pid_t pid, ppid, chpid;

    pid = getpid();
    ppid = getppid();
    
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("My pid = %d, my ppid = %d\n", pid, ppid);
    int a = 0;
    
    chpid = fork();
    if (chpid < 0)
    {
        /* Ошибка */
        printf("Ошибка\n");
    } 
    else if (chpid == 0)
    {
        /* Порожденный процесс */
        a = a+1; 
        pid = getpid();
        ppid = getppid();

        printf("pid = %d, ppid = %d, a = %d\n", (int)pid, (int)ppid, a); 
    }
    else 
    {
        /* Родительский процесс */
        pid = getpid();
        ppid = getppid();

        printf("My pid = %d, my ppid = %d, result = %d\n", (int)pid, (int)ppid, a); 
        
    }

    return 0;

} 