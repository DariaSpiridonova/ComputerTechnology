#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <time.h>

/* sin(x) = sum_{n=0}^{N-1} (-1)^n * x^(2n+1) / (2n+1)!
 * Each term is considered in its own stream; the main thread sums it up
*/

#define MAX_TERMS 30

typedef struct
{
    double x;       /* sin arg */
    int    n;       /* term number */
    double term;    /* thread result */
} job_t;

void * term_func (void * arg)
{
    job_t * job = (job_t *) arg;

    /* x^(2n+1) / (2n+1)! calculate by steps: t *= x / i */
    double t = job->x;
    for (int i = 2; i <= 2 * job->n + 1; i++)
        t *= job->x / i;

    job->term = (job->n % 2) ? -t : t;   /* sign of (-1)^n */
    return NULL;
}

static double now_sec (void)
{
    struct timespec ts;
    clock_gettime (CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

/* calculate sin(x) by N terms, N threads */
static double sin_threads (double x, int N)
{
    pthread_t thread[MAX_TERMS]; // to identify current thread
    job_t     job[MAX_TERMS];    // for terms structures with data

    for (int n = 0; n < N; n++)
    {
        job[n].x = x;
        job[n].n = n;
        if (pthread_create (&thread[n], NULL, term_func, &job[n]) != 0)
        {
            fprintf (stderr, "Error (thread %d)\n", n);
            exit (1);
        }
    }

    double sum = 0.0;
    for (int n = 0; n < N; n++)
    {
        pthread_join (thread[n], NULL);
        sum += job[n].term;
    }
    return sum;
}

int main (void)
{
    double x;
    printf ("Print sin argument in radians: ");
    if (scanf ("%lf", &x) != 1)
    {
        fprintf (stderr, "Input error\n");
        return 1;
    }

    /* cast to [-pi, pi] */
    x = fmod (x, 2 * M_PI);
    if (x >  M_PI) x -= 2 * M_PI;
    if (x < -M_PI) x += 2 * M_PI;

    double lib_sin = sin (x);
    printf ("sin() from libm: %.17g\n\n", lib_sin);
    printf ("%8s %22s %12s %12s\n", "threads", "result", "error", "time, us");

    int counts[] = {1, 2, 3, 5, 8, 12, 16, 20, 30};
    for (int i = 0; i < (int)(sizeof counts / sizeof counts[0]); i++)
    {
        int N = counts[i];
        double t0 = now_sec (); // start
        double result = sin_threads (x, N);
        double us = (now_sec () - t0) * 1e6; // finish
        printf ("%8d %22.17g %12.3e %12.1f\n", N, result, fabs (result - lib_sin), us);
    }

    /* for comparison: the same row without streams (N = 20) */
    double t0 = now_sec ();
    double s = 0, t = x;
    for (int n = 0; n < 20; n++)
    {
        s += t;
        t *= -x * x / ((2.0 * n + 2) * (2.0 * n + 3));
    }
    printf ("\nthe same row without streams (N = 20): %.1f us\n", (now_sec () - t0) * 1e6);

    t0 = now_sec ();
    volatile double v = sin (x);
    (void) v;
    printf ("Стандартный sin(): %.2f us\n", (now_sec () - t0) * 1e6);

    return 0;
}
