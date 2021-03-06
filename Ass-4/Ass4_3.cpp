#include <bits/stdc++.h>
#include <pthread.h>
#include <signal.h>
#include <ctime>
#include <unistd.h> 

using namespace std;
#define MAX_QUEUE_SIZE 3000
#define NUM 1000
#define RUNNING 0
#define SUSPENDED 1
#define TERMINATED 2
#define DELTA 1
#define MAX_THREAD 20

queue<int> buffer;
vector<int> status(100,0);
pthread_t workers[MAX_THREAD],scheduler,reporter;
pthread_mutex_t mute, st_lock;

void suspend(int id)
{
    pause();
}

void resume(int id)
{
    ;
}

void producer(){
    for(int i=0;i<NUM;i++)
        {
            // printf("buffer size %d\n",buffer.size());
            while(buffer.size()>=MAX_QUEUE_SIZE);
            pthread_mutex_lock(&mute);
            buffer.push(rand()%100);
            pthread_mutex_unlock(&mute);
        }
}

void consumer(){
    while(1)
        {
            while(buffer.size()<=0);
            pthread_mutex_lock(&mute);
            buffer.pop();
            pthread_mutex_unlock(&mute);
        }
}

void * work(void* args)
{

    int id = *(int*)args;
    
    int x = (rand()%2);
    // printf("x = %d\n",x);
    if(x==0)
    {
        producer();
    }
    else
    {
        consumer();
    }
    
    status[id] = TERMINATED;
    pthread_exit(0);
}

void * schedule(void* args)
{
    sleep(0.5);
    queue<int> ready;
    int n = *(int *)args,i;
    for(i=0;i<n;i++)
    {
        ready.push(i);
    }
    while(ready.size()!=0)
    {
        if(ready.size()!=1)
        {
            int current = ready.front();
            // printf("%d\n", current);
            ready.pop();
            // printf("scheduler waiting for st_lock\n");
            pthread_mutex_lock(&st_lock);
            status[current] = RUNNING;
            pthread_kill(workers[current], SIGUSR2);
            pthread_mutex_unlock(&st_lock);
            // printf("scheduler released for st_lock\n");
            // printf("-------------waking up : %d\n", current);
            sleep(DELTA);
            pthread_mutex_lock(&st_lock);
            if(status[current]!=TERMINATED)
            {
                // printf("scheduler waiting for st_lock\n");
                status[current] = SUSPENDED;
                pthread_kill(workers[current],SIGUSR1);
                // printf("------------suspending down : %d\n", current);
                // printf("scheduler released for st_lock\n");
                ready.push(current);
            }
            pthread_mutex_unlock(&st_lock);
            sleep(0.01);

        }
        else
        {
            int current = ready.front();
            // printf("%d\n", current);
            ready.pop();
            pthread_mutex_lock(&st_lock);
            status[current] = RUNNING;
            // printf("------------waking up : %d\n", current);
            pthread_kill(workers[current], SIGUSR2);
            pthread_mutex_unlock(&st_lock);
            while(status[current]!=TERMINATED);
            break;
        }
        
    }
    pthread_exit(0);
}

void *report(void * args)
{
    sleep(0.1);
    string name[3]={"RUNNING","SUSPENDED","TERMINATED"};
    int n = *(int*)args;
    int previous_status[n];
    for(int i=0;i<n;i++)
        previous_status[i] = SUSPENDED;
    while(1)
    {
        pthread_mutex_lock(&st_lock);
        for(int i=0;i<n;i++)
        {
            // printf("reporter waiting for st_lock\n");
            if(previous_status[i] != status[i])
            {
                if(previous_status[i]==SUSPENDED&&status[i]==TERMINATED)
                {
                    printf("%d: SUSPENDED -> RUNNING\n",i);
                    printf("%d: RUNNING -> TERMINATED\n",i);
                }
                else
                {
                    printf("%d: %s -> %s\n", i, name[previous_status[i]].c_str(), name[status[i]].c_str());
                }
            }
            previous_status[i] = status[i];
            // printf("reporter released for st_lock\n");
        }      
        pthread_mutex_unlock(&st_lock);
    }
    pthread_exit(0);
}

int main()
{
    int n,i,pstat;
    cout<<"Enter N: ";
    cin>>n;
    if(n>MAX_THREAD)
    {
        printf("Please enter the value less than 20\n");
        return 0;
    }

    if (pthread_mutex_init(&mute, NULL) != 0) 
    { 
        printf("\n mutex init has failed\n"); 
        return 1; 
    } 

    if (pthread_mutex_init(&st_lock, NULL) != 0) 
    { 
        printf("\n mutex init has failed\n"); 
        return 1; 
    } 
    // status.resize(n);
    srand(time(NULL));
    int args[n];
    signal(SIGUSR1,suspend);
    signal(SIGUSR2,resume);
    for(i=0;i<n;i++)
    {
        args[i]=i;
        pstat = pthread_create(&workers[i],NULL, work,(void *)(args+i));
        if(pstat != 0) 
        {
            printf("While creating thread %d, pthread_create returned error code %d\n", i, pstat);
            exit(1);
        }
        // printf("worker waiting for st_lock\n");
        pthread_mutex_lock(&st_lock);
        status[i] = SUSPENDED;
        pthread_kill(workers[i],SIGUSR1);
        pthread_mutex_unlock(&st_lock);
        // printf("worker released st_lock\n");

    }
    if ((pstat = pthread_create(&scheduler, NULL, schedule, (void*)&n))!=0)
    {
        printf("While creating Scheduler thread, pthread_create returned error code %d\n", pstat);
        exit(1);
    }
    if ((pstat = pthread_create(&reporter, NULL, report, (void *)&n)) != 0)
    {
        printf("While creating Reporter thread, pthread_create returned error code %d\n", pstat);
        exit(1);
    }
    for(i=0;i<n;i++)
    {
        pthread_join(workers[i],NULL);
    }
    pthread_join(scheduler, NULL);
    pthread_join(reporter, NULL);
    return 0;
}