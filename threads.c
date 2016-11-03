#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>
#include <signal.h>
#include <stdlib.h>
#include <semaphore.h>
#include <errno.h>

#define NUM_THREADS 8

static int threads_created = 0;
static int sig_count = 0;
static int sig1_total = 0;
static int sig2_total = 0;
sem_t signal_thread, signal_report;
int gen_value;
pthread_mutex_t gen_mutex, handler1_mutex, handler2_mutex, report_mutex;

void *sig_gen();
void *handler();
void *sig_report();

int main(int argc, char **argv){
	sigset_t mask1, allmask;
	int i;	
	
	sigfillset(&allmask);
	pthread_sigmask(SIG_BLOCK, &allmask, NULL);

	pthread_t pth[NUM_THREADS];	

	//Creates the 4 handler threads
	for(i = 3; i < 7; i++){
		pthread_create(&pth[i], NULL, handler, &i);
	}

	//Creates 1 reporting thread
	pthread_create(&pth[7], NULL, sig_report, NULL);
	
	sleep(3);
	//Creates 3 generator threads
	for(i = 0; i < 3; i++){
		pthread_create(&pth[i], NULL, sig_gen, pth);
	}
	
	for(i = 0; i < 8; i++){
		pthread_join(pth[i], NULL);
	}
}

//A thread randomly send out SIGUSR1 or SIGUSR2 until sig_count reaches 10,000
void *sig_gen(pthread_t *pth){
	pthread_t *thread = pth;
	while(sig_count < 10){
		srand(time(NULL));
		int which_signal = rand() % 2;
		int sleep_time = (rand() % 10) * 100000;
		int i;

		pthread_mutex_lock(&gen_mutex);

		if(which_signal == 0){
			for(i = 3; i < 8; i++){
				if(pthread_kill(thread[i], SIGUSR1) != 0){
					perror("Signal failed to send");
					EXIT_FAILURE;
				}
				usleep(5);
			}
		}else if(which_signal == 1){
			for(i = 3; i < 8; i++){
				if(pthread_kill(pth[i], SIGUSR2) != 0){
					perror("Signal failed to send");
					EXIT_FAILURE;
				}
				usleep(5);
			}
		}
		sig_count++;
		usleep(sleep_time);
		printf("%d\n", sig_count);
		
		pthread_mutex_unlock(&gen_mutex);
	}
}

//A thread receive either SIGUSR1 or SIGUSR2, and updates the counter for the corresponding signal
void *handler(int ignore){
	int sig_id, err;
	sigset_t mask1;
	sigemptyset(&mask1);
	sigaddset(&mask1, SIGUSR1);
	sigaddset(&mask1, SIGUSR2);

	while(err = sigwait(&mask1, &sig_id) >= 0){
		if(sig_id == SIGUSR1){
			pthread_mutex_lock(&handler1_mutex);
			sig1_total++;
			printf("SIGUSR1 signal received!\n");
			pthread_mutex_unlock(&handler1_mutex);
		}else if(sig_id == SIGUSR2){
			pthread_mutex_lock(&handler2_mutex);
			sig2_total++;
			printf("SIGUSR2 signal received!\n");
			pthread_mutex_unlock(&handler2_mutex);
		}
	}
}

//Also receives signals from sig_gen() and reports time signals were received
void *sig_report(){
	sigset_t mask1;
	int sig_id;
	
	sigemptyset(&mask1);
	sigaddset(&mask1, SIGUSR1);
	sigaddset(&mask1, SIGUSR2);

	while(sigwait(&mask1, &sig_id) >= 0){
		int local_sig = 0, avg1_s = 0, avg1_u = 0, avg2_s = 0, avg2_u = 0, num_sig1 = 0, num_sig2 = 0;
		int i;
		struct timeval report_time1, report_time1p, report_time2, report_time2p;

		gettimeofday(&report_time1p, NULL);
		gettimeofday(&report_time2p, NULL);
		
		while(local_sig < 10){
			local_sig++;
			if(sig_id == SIGUSR1){
                              	gettimeofday(&report_time1, NULL);
                       		avg1_s += (report_time1.tv_sec - report_time1p.tv_sec);
				avg1_u += (report_time1.tv_usec - report_time1p.tv_usec);
				report_time1p = report_time1;
				num_sig1++;
			}
                       	if(sig_id == SIGUSR2){
                               	gettimeofday(&report_time2, NULL);
				avg2_s += (report_time2.tv_sec - report_time2p.tv_sec);
				avg2_u += (report_time2.tv_usec - report_time2p.tv_usec);
				report_time2p = report_time2;
				num_sig2++;
                       	}
		}
	//	avg1_s = avg1_s/num_sig1;
	//	avg1_u = avg1_u/num_sig1;
	//	avg2_s = avg2_s/num_sig2;
	//	avg2_u = avg2_u/num_sig2;
		
		printf("The average time between signals for SIGUSR1 is %d second(s) and %d microsecond(s)\n", avg1_s, avg1_u);
		printf("The average time between signals for SIGUSR2 is %d second(s) and %d microsecond(s)\n\n", avg2_s, avg2_u);
	}
}
