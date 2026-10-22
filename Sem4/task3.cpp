#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

const size_t N = 20   // количество замеров

void * any_func (void * arg)
{
  while (1)
  {
    fprintf (stderr, ".");
    sleep (10);
  }
  return NULL;
}

// разница двух моментов времени в микросекундах
static double diff_us (struct timespec a, struct timespec b)
{
  return (b.tv_sec - a.tv_sec) * 1e6 + (b.tv_nsec - a.tv_nsec) / 1e3;
}

int main (void)
{
  double total = 0;

  for (int i = 0; i < N; i++)
  {
    pthread_t thread;
    void * result;
    struct timespec t1, t2;

    if (pthread_create (&thread, NULL, any_func, NULL) != 0)
    {
      fprintf (stderr, "Error\n");
      return 1;
    }

    sleep (5);

    clock_gettime (CLOCK_MONOTONIC, &t1);   // момент сигнала
    pthread_cancel (thread);
    if (!pthread_equal (pthread_self (), thread))
    {
      pthread_join (thread, &result);
    }
    clock_gettime (CLOCK_MONOTONIC, &t2);   // поток завершился

    if (result == PTHREAD_CANCELED)
    {
      double us = diff_us (t1, t2);
      fprintf (stderr, "\nCanceled, %.1f us\n", us);
      total += us;
    }
  }

  fprintf (stderr, "Average completion time: %.1f us\n", total / N);
  return 0;
}
