/*************************************************************************
	> File Name: cli.c
	> Author: 
	> Mail: 
	> Created Time: 2018年06月18日 星期一 17时32分08秒
 ************************************************************************/

#include<stdio.h>
#include<assert.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<string.h>
#include<unistd.h>
#include<stdbool.h>
#include<stdlib.h>

int port=7801;
int create_socket()
{
    int sockfd=socket(AF_INET,SOCK_STREAM,0);
    if(sockfd<0)
    {
        perror("socket error");
        return -1;
    }

    return sockfd;
}
int main()
{
    int sockfd=create_socket();
    struct sockaddr_in saddr;
    bzero(&saddr,sizeof(saddr));
    saddr.sin_family=AF_INET;
    saddr.sin_port=htons(port);
    saddr.sin_addr.s_addr=inet_addr("127.0.0.1");

    int con=connect(sockfd,(struct sockaddr*)&saddr,sizeof(saddr));
    if(con<0)
    {
        perror("connect error");
        return -1;
    }
    char buff[256]={0};
    int res=recv(sockfd,buff,127,0);
    close(sockfd);
    int new_sockfd=socket(AF_INET,SOCK_STREAM,0);
    char ip[56]={0};
    strcat(ip,strtok(buff,"#"));
    int port=atoi(strtok(NULL,"#"));  //port
    struct sockaddr_in new_addr;
    new_addr.sin_family=AF_INET;
    new_addr.sin_port=htons(port);
    new_addr.sin_addr.s_addr=inet_addr(ip);
    con=connect(new_sockfd,(struct sockaddr*)&new_addr,sizeof(new_addr));
    while(1)
    {
        char buff[127]={0};
        fgets(buff,127,stdin);
        send(new_sockfd,buff,127,0);
        int res=recv(new_sockfd,buff,127,0);
        if(res==0)
        {
            break;
        }
        printf("buff=%s\n",buff);
        fflush(stdout);
    }

}
