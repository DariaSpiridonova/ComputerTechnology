#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>
#include <stdlib.h>

void * thread_func1 (void * arg)
{
	fprintf (stderr, "Thread1\n");
    double val = *(double *)arg;
    double *square = (double *)calloc(1, sizeof(double));
    *square = sin(val) * sin(val);
	return square;
}

void * thread_func2 (void * arg)
{
	fprintf (stderr, "Thread2\n");
	double val = *(double *)arg;
    double *square = (double *)calloc(1, sizeof(double));
    *square = cos(val) * cos(val);
	return square;
}

int main (void)
{
    double val = 0.0;

    printf("Print any value for checking sin*sin + cos*cos == 1\n");
    if (scanf("%lf", &val) != 1)
    {
        fprintf (stderr, "Input error\n");
        return 1;
    }
    void * val_ptr = &val; // for threads

    // addresses are needed for pthread_join
    void * sin2_ptr;
    void * cos2_ptr;

	pthread_t thread1, thread2;

	if (pthread_create (&thread1, NULL,
			             thread_func1, val_ptr) != 0)
	{
		fprintf (stderr, "Error (thread1)\n");
		return 1;
	}

	if (pthread_create (&thread2, NULL,
			             thread_func2, val_ptr) != 0)
	{
		fprintf (stderr, "Error (thread2)\n");
		return 1;
	}

    pthread_join (thread1, &sin2_ptr);
    pthread_join (thread2, &cos2_ptr);

    double result = *(double *)sin2_ptr + *(double *)cos2_ptr;
    if (fabs(result - 1) < 1e-12)
        printf("sin*sin + cos*cos == 1\n");
    else
        printf("Calculation error\n");

    free(sin2_ptr);
    free(cos2_ptr);

	return 0;
}
