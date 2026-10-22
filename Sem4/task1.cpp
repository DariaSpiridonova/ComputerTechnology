#include <stdio.h>
#include <pthread.h>
#include <stdint.h>

void * any_func (void * arg)
{
  // arg имеет тип void *. Приводим к int * (указатель на int),
  // затем разыменовываем (*): получаем число, на которое он указывает
  int a = *(int *) arg;
  a++;
  return (void *)(intptr_t) a;
}

int main (void)
{
  pthread_t thread;
  int parg = 2007;
  void *pdata;

    if (pthread_create (&thread, NULL, any_func, &parg) != 0)
    {
        fprintf (stderr, "Error\n");
        return 1;
    }

  pthread_join (thread, &pdata);

  parg = (int)(intptr_t) pdata;
  printf ("%d\n", parg); // 2008

  parg = (int)(intptr_t)any_func(&parg);
  printf ("%d\n", parg); // 2009

  return 0;
}
