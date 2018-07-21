#include<iostream>
#include<pthread.h>
#include<unistd.h>
#include<cstring>
#include<errno.h>
#include<cstdlib>

class Condition
{
    private:
    pthread_mutex_t pmutex;
    pthread_cond_t pcond;
    public:
    Condition();
    int condition_Unlock();
    int condition_Lock();
    int condition_Wait();
    int condition_TimeWait(Condition &_cond,const struct timespec &abstime);
    int conditon_Signal();
    int condition_Broadcast();
    ~Condition();

};
Condition::Condition()
{
    int status;
    if((status==pthread_mutex_init(&pmutex,NULL))
    {
         perror("pthread_mutex_init error");  
    }
       if(status=pthread_cond_init(&pcond,NULL))
       {
           perror("pthread_cond_init error");
       }
}
int Condition::condition_Lock()
{
           return pthread_mutex_lock(&pmutex);
}

int Condition::condition_Unlock()
{
    return pthread_mutex_unlock(&pmutex);
}

int Condition::condition_Wait()
{
    return pthread_cond_wait(&pcond,_&pmutex);           
}

int Condition::condition_TimeWait()
{
    return    pthread_cond_timewait(&pmutex,&pcond);
}

int Condition::condition_signal()
{
    return pthread_cond_signal(&pcond);   
}

int Condition::condition_Broadcast()
{
    return pthread_cond_broadcast(&pcond);           
}
Condition::~Condition()
{
    int status;
     if((status=pthread_cond_destroy(&pcond))) 
    {
        perror("pthread_cond_destroy error");
    }
    if((status=pthread_mutex_destroy(&pmutex)))
    {
        perror("pthread_mutex_destroy error");
    }
}

struct Task
{
    void *(*run)(void *arg);
    void *arg;
    struct Task *next;
};


typedef Condtion Condition_t ;
typedef Task Task_t;

class Thread_Pool
{
    private:
    Condition_t ready;
    Task_t first;
    Task_t last;
    int counter;
    int idle;
    int max_threads;
    int quit;
}


typedef Thread_Pool Thread_Pool_t;

int main()
{
    return 0;
}

