#include<stdio.h>
#include<sys/socket.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<string.h>
#include<assert.h>
#include<stdbool.h>
#include<pthread.h>
#include<sys/types.h>
int server_port=7802;
int create_sock()
{
	int sockfd=socket(AF_INET,SOCK_STREAM,0);
	if(sockfd<0)
	{
		return -1;		
	}
	struct sockaddr_in saddr;
	memset(&saddr,0,sizeof(saddr));
	saddr.sin_family=AF_INET;
	saddr.sin_port=htons(server_port);
	saddr.sin_addr.s_addr=inet_addr("127.0.0.1");
	int res=bind(sockfd,(struct sockaddr*)&saddr,sizeof(saddr));
	if(res<0)
	{
		perror("bind error");
		return -1;
	}
	listen(sockfd,5);
	return sockfd;
}
void * thread_fun(void *args)
{
	int c=(int)args;
	char buff[128]={0};
	while(1)
	{
        printf("Get one Link\n");
       while(true)
        {
            int size=recv(c,buff,127,0);
            if(size==0)
            {
                break;
            }
            else{
                printf("%s",buff);
                fflush(stdout);
                memset(buff,0,sizeof(buff));
            }
        }
        close(c);
    }
	pthread_exit(0);
}


int main()
{
		int sockfd=create_sock();
		if(sockfd<0)
		{
			perror("socket create error");
		}
		while(1)
		{
			struct sockaddr_in caddr;
			int len=sizeof(caddr);
			int c=accept(sockfd,(struct sockaddr*)&caddr,&len);
			{
				printf("accpet :%d\n",c);
				if(c<0)
				{
					continue;
				}
				pthread_t id;
				pthread_create(&id,NULL,thread_fun,(void*)c);
			}
		}
		return 0;
}
