#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>

static void *ThreadFunc(void *Param);

#define MAX_THREADS	10
pthread_mutex_t mutex;	// cond와 nRequest를 동시에 보호
pthread_cond_t cond;
int nRequest;

int main()
{
	pthread_t m_Thread[MAX_THREADS];
	int identifier[MAX_THREADS];

	pthread_mutex_init(&mutex, NULL); 
	pthread_cond_init(&cond, NULL); 
	nRequest = 0;


	srand(time(NULL));
	for(int i=0; i<MAX_THREADS; i++)
	{
		identifier[i] = i;
		pthread_create(&m_Thread[i], NULL, ThreadFunc, &identifier[i]);
	}

	int wait = 0;
	srand(time(NULL));

	/*
	printf("sleep 15 Start\n");
	sleep(15);
	printf("sleep 15 End\n");
	*/

	while( true )
	{
		pthread_mutex_lock(&mutex);
		nRequest++;
		pthread_cond_signal(&cond);
		pthread_mutex_unlock(&mutex);
		wait = rand() % 100;
		printf("main - wait for %.2f sec\n", (double)wait / 100);
		usleep(wait * 10000);
	}

	return 0;
}

static void *ThreadFunc(void *Param)
{
	int identifier = *((int *)Param);
	unsigned int rc;
	int wait = 0;
	while( true )
	{
		//printf("mutex_lock St %d\n", identifier);
		pthread_mutex_lock(&mutex);
		//printf("mutex_lock Ed %d\n", identifier);

		/*
		if(identifier == 0)
		{
			printf("Thread sleep St %d\n", identifier);
			sleep(5);
			printf("Thread sleep Ed %d\n", identifier);
		}
		*/

		rc = 0;

		if( 0 == nRequest )
			rc = pthread_cond_wait(&cond, &mutex);	// 여기서 lock이 풀리고 wait

		if( nRequest > 0 && 0 == rc )
		{
			wait = rand() % 10;
			printf("I'm %d - wait for %d sec(%d)\n", identifier, wait, nRequest);
			nRequest--;
			pthread_mutex_unlock(&mutex);
			sleep(wait);
		}
		else
		{
			pthread_mutex_unlock(&mutex);
		}
	}
}
