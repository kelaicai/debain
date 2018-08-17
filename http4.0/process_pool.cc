#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include"./http_redirection.h"
#include"./configure.h"
#include<cstring>
#include"./hash.h"
#include"md5.h"
const int BUFFER_SIZE=1024;
const int MAX_EVENT_NUMBER=1024;
const int PROCESS_COUNT=2;
const int USER_PER_PROCESS=65536;

#define IP_ADDR "127.0.0.1"
const int PORT=7801;


struct process_in_pool
{
    pid_t pid;
    int pipefd[2];
};

typedef process_in_pool process_in_pool_t;

struct client_data
{
    sockaddr_in client_address;
    char buf[BUFFER_SIZE];
    int read_idx;
};
typedef client_data client_data_t;

//be used to father to son's signal message
int signal_pipefd[2];
int epollfd;
int listenfd;
process_in_pool_t sub_process[PROCESS_COUNT];
bool stop_child=false;

int setnonblocking( int fd )
{
    int old_option = fcntl( fd, F_GETFL );
    int new_option = old_option | O_NONBLOCK;
    fcntl( fd, F_SETFL, new_option );
    return old_option;
}

string func(const string& str, int num) { 
  char buff[100] = "";
  sprintf(buff, "%s#%d", str.c_str(), num);
  return buff;
}
void addfd( int epollfd, int fd )
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl( epollfd, EPOLL_CTL_ADD, fd, &event );
    setnonblocking( fd );
}

void delfd(int epollfd,int fd)
{
    if((epoll_ctl(epollfd,EPOLL_CTL_DEL,fd,NULL))==-1)
    {
        perror("epoll_ctl_error");
    }
}
void string_to_char(string ip_port,server_info & ser)
{
    char buff[30]={0};
    strcpy(buff,ip_port.c_str());

    char cip[30];
    strcpy(cip,strtok(buff,":"));
    string strip(cip);
    int port=atoi(strtok(NULL,":"));
    ser.ip=strip;
    ser.port=port;
}
//before execute sig_handler ,you must  temporary store the errno and do you
//sig_handler event ,and finished the sig_handler event you must recovery the old errno

void sig_handler( int sig )
{
    int tmp_errno = errno;
    int msg = sig;
    send( signal_pipefd[1], ( char* )&msg, 1, 0 );
    errno = tmp_errno;
}
//now we are registering signal that we care 
void addsig( int sig, void(*handler)(int), bool restart = true )
{
    struct sigaction sact;
   memset( &sact, '\0', sizeof( sact ) );
    sact.sa_handler = handler;
    if( restart )
    {
        sact.sa_flags |= SA_RESTART;
    }
    sigfillset( &sact.sa_mask );
    assert( sigaction( sig, &sact, NULL ) != -1 );
}
//now we want to realse our requst's resuorce
//include  --->signals and file discriptor
void realse_resource()
{
    close( signal_pipefd[0] );
    close( signal_pipefd[1] );
    close( listenfd );
    close( epollfd );
}

//we want child to terminate
void child_term_handler(int sig)
{
    stop_child=true;
}
//if child send SIGCHLD signal ,use waitpid() to avoid the zombie process
void child_child_handler( int sig )
{
    pid_t pid;
    int status;
    while ( ( pid = waitpid( -1, &status, WNOHANG ) ) > 0 )
    {
        continue;
    }
}

int run_child(CHash &hash,int index)
{
    epoll_event events[MAX_EVENT_NUMBER];

    int child_epollfd=epoll_create(5);
    assert(child_epollfd!=-1);
    //sub_process begin it's write port
    int pipefd1=sub_process[index].pipefd[1];
    addfd(child_epollfd,pipefd1);

    int ret;

    addsig(SIGTERM,child_term_handler,false);
    addsig(SIGCHLD,child_child_handler);

    client_data *users=new client_data[USER_PER_PROCESS];
    
    //printf("index %d process run   and stop_child=%d\n",index,stop_child);
    while(!stop_child)
    {  //how can i understand timeout=-1 and epoll_wait's return value ?
        int count=epoll_wait(child_epollfd,events,MAX_EVENT_NUMBER,5000);
        if((count<0) && (errno!=EINTR))
        {

            printf("index %d process run\n",index);
            perror("sub_process epoll failed:");
            break;
        }
        //Now we begin to execute IO events by the IO events types.h
        for(int i=0;i<count;++i)
        {
            int fd=events[i].data.fd;
            //from father's signal 
            if((fd==pipefd1) && (events[i].events & EPOLLIN))
            {
                int client=0;
                //now sub_process write something to pipefd1
                //this pipefd1 belong father  and son ,and the result is stored in the client value, if there read success ,it  present there is new cilent arrival
                ret=recv(fd,(char*)&client,sizeof(client),0);
                if(ret<0)
                {
                    if(errno!=EAGAIN)
                    {
                        stop_child=true;
                    }
                }
                else if(ret==0)
                {
                    //timeout ??
                    printf("client over\n");
                    stop_child=true;
                }
                else  //there are some data need to read
                {
                    struct sockaddr_in client_addr;
                    socklen_t client_addrlen=sizeof(client_addr);
                    int connfd=accept(listenfd,(struct sockaddr*)&client_addr,&client_addrlen);
                    if(connfd<0)
                    {
                        printf(" accept fd=%d errno is %d\n",fd,errno);
                        continue;
                    }
                  //  memset(users[connfd].buf,0,BUFFER_SIZE);
                //    users[connfd].client_address=client_addr;//struct to struct 
                 //   users[connfd].read_idx=0;
                   // addfd(child_epollfd,connfd);
                    //http reponse review
                    /*
                    string ip_port(inet_ntoa(client_addr.sin_addr.s_addr));
                    string real_ip_port=find(ip_port);
                    server_info ser;
                   string_to_char(real_ip_port,ser);
                   *p
                   */
                    string tmp(inet_ntoa(client_addr.sin_addr));
                    string ip_port=hash.find(tmp);
                    //send http redireciton reponse;
                    if(http_redirection(connfd,301,ip_port))
                    {
                        cout<<"http_redirection successfully!"<<endl;
                        shutdown(connfd,SHUT_RDWR); 
                        cout<<"close"<<endl;
                    }else
                    {
                        cout<<"http_redirection failed !"<<endl;
                    }
                    //finidsh the connection bettewn dispatch and client
                }
            }
                else if(events[i].events && EPOLLIN)
                {
                    int idx=0;
                    while(true)
                    {
                        idx=users[fd].read_idx;
                        ret=recv(fd,users[fd].buf,BUFFER_SIZE,0);
                        if(ret<0)
                        {
                            //
                            if(errno!=EAGAIN)  
                            {
                                epoll_ctl(child_epollfd,EPOLL_CTL_DEL,fd,0);
                                close(fd);
                            }
                            //ordinary read data successfully
                           //  add send message
                            break;
                        }
                        else if(ret==0)  //client close this connection  with ct
                        {
                            epoll_ctl(child_epollfd,EPOLL_CTL_DEL,fd,0);
                            close(fd);  //thoroughly close the fd
                            break;
                        }
                        else
                        {

                        /*   //  buffer is not emptypty 
                            users[fd].read_idx+=ret;
                            idx=users[fd].read_idx;
                            printf("user's content is :%s\n",users[fd].buf);

                            //
                            if((idx<2) || (users[fd].buf[idx-2]!='\r') || (users[fd].buf[idx-1]!='\n'))
                            {
                                continue;
                            }
                            char* file_name=users[fd].buf;
                            if(access(file_name,F_OK)==-1)
                            {
                                epoll_ctl(child_epollfd,EPOLL_CTL_DEL,fd,0);
                                close(fd);
                                break;
                            }
                            ret=fork();
                            if(ret==-1)
                            {
                                epoll_ctl(child_epollfd,EPOLL_CTL_DEL,fd,0);
                                close(fd);
                                break;
                            }
                            else if(ret>0) 
                            {
                                epoll_ctl(child_epollfd,EPOLL_CTL_DEL,fd,0);
                                close(fd);
                                break;
                            }
                            else
                            {
                                close(STDOUT_FILENO);
                                dup(fd);
                                execl(users[fd].buf,users[fd].buf,0);
                                exit(0);
                            }
                            */
                        }   
                    }
                }
				else
                {
                  continue; 
                }
		}
    }
    
    delete [] users;
    close(pipefd1);
    close(child_epollfd);
    return 0;
}
int main()
{
    /*configure socket*/
    listenfd=socket(AF_INET,SOCK_STREAM,0);
    int reuse=1;
    setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse));
    if(listenfd<0)
    {
        perror("socket error");
        return -1;
    }

    struct sockaddr_in ser_addr;
    bzero(&ser_addr,sizeof(ser_addr));
    ser_addr.sin_family=AF_INET;
   ser_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
   ser_addr.sin_port=htons(PORT);
   
   int ret=bind(listenfd,(struct sockaddr*)&ser_addr,sizeof(ser_addr));
    if(ret==-1)
    {
        perror("bind error");
        printf("pid=%d\n",getpid());
    }
    ret=listen(listenfd,5);
    if(ret==-1)
    {
        perror("bind error");
    }
    
    

    /*read some configure infomation from configure.json*/
   Configure config;
   vector<string> serve_ip;
   vector<server_info> vec_server=config.get_server_info();
   int server_count=vec_server.size();
   for(int i=0;i<server_count;i++)
    {
        char buf[10]={0};
        sprintf(buf,"%d",vec_server[i].port);
        string str=vec_server[i].ip+":"+string(buf);
        serve_ip.push_back(str);
    }

    //connect to every server
    int sock[3];  //need motify
    struct sockaddr_in sock_addr[3];

   for(int i=0;i<server_count;i++)
    {
        sock[i]=socket(AF_INET,SOCK_STREAM,0);
        sock_addr[i].sin_family=AF_INET;
        sock_addr[i].sin_port=htons(vec_server[i].port);
        sock_addr[i].sin_addr.s_addr=inet_addr(vec_server[i].ip.c_str());
        ret=connect(sock[i],(struct sockaddr*)&sock_addr[i],sizeof(sock_addr[i]));
        if(ret<0)
        {
            cout<<"No "<<i<<"connection failed"<<endl;
        }
    }
    
    //build relation bettewn serve and plan vnode
   
    RealNode::setHock(func);
    CHash hash(server_count,serve_ip);
   cout<<"realation build successfully!"<<endl;
    for(int i=0;i<PROCESS_COUNT;++i)
    {
        ret=socketpair(AF_UNIX,SOCK_STREAM,0,sub_process[i].pipefd);
    
        if(ret==-1)
        {
          perror("socketpair error");
        }
        //create son process
        sub_process[i].pid=fork();
        if(sub_process[i].pid<0)
        {
            perror("fork error");
            continue;
        }
        else if(sub_process[i].pid>0)
        {
            close(sub_process[i].pipefd[1]);
            setnonblocking(sub_process[i].pipefd[0]);  //set son's read noblocking
            continue;
        }
        else{
                close(sub_process[i].pipefd[0]);
                setnonblocking(sub_process[i].pipefd[1]);  //set father's write noblocking
            run_child(hash,i);
            exit(0);
        }

    }  //init sub_process and master porcess
    printf("i am = parent %d\n",getpid()); 

    epoll_event events[MAX_EVENT_NUMBER];
    int master_epollfd=epoll_create(5);
    if(master_epollfd==-1)
    {
        perror("epoll errro");
    }

    addfd(master_epollfd,listenfd);
    addsig(SIGCHLD,sig_handler);
    addsig(SIGTERM,sig_handler);
    addsig(SIGINT,sig_handler);
    addsig(SIGPIPE,sig_handler);
    static int sub_process_counter=0;
    bool stop_server=false;

    
    

    while(!stop_server)
    {
        int ready_fd_num=epoll_wait(master_epollfd,events,MAX_EVENT_NUMBER,1000);
        if(ready_fd_num<0 && errno!= EINTR)
        {
            perror("epoll wait error");
        }
        for(int i=0;i<ready_fd_num;++i)
        {
            int fd=events[i].data.fd;
            //if fd is a sockfd
            if(fd==listenfd)
            {
                int new_conn=1;
                //send messgae to sub_process 
                //
                //master's responsibility is alarm sub_process accept a new 
                //connection and sub_process execute client transaction(事务)
                send(sub_process[sub_process_counter++].pipefd[0],(char*)&new_conn,sizeof(new_conn),0);
                printf("send request to child %d\n",sub_process_counter-1);
                sub_process_counter%=PROCESS_COUNT;
            }
            else if((fd==signal_pipefd[0]) && (events[i].events &EPOLLIN))
            {
                int sig;
                char signals[1024];
                ret=recv(signal_pipefd[0],signals,sizeof(signals),0);
                if(ret==-1)
                {
                    //send successfully  on noblocking 
                    continue;
                }
                else if(ret==0)
                {
                    //close client
                    continue;
                }
                else
                {
                    for(int j=0;i<ret;++j)
                    {
                        switch(signals[j])
                        {
                            case SIGCHLD:
                            {
                                pid_t pid;
                                int stat;
                                while((pid=waitpid(-1,&stat,WNOHANG))<0)
                                {
                                    for(int k=0;k<PROCESS_COUNT;++k)
                                    {
                                        if(sub_process[i].pid==pid)
                                        {
                                            close(sub_process[k].pipefd[0]);
                                            sub_process[i].pid=-1;
                                        }
                                    }//for 
                                }//while pid=waitpidd
                                stop_server=true;
                                for( int i = 0; i < PROCESS_COUNT; ++i )
                                {
                                    if( sub_process[i].pid != -1 )
                                    {
                                        stop_server = false;
                                    }
                                } //for --->PROCESS_COUNT
                                break;
                            }//case SIGCHLD
                            case SIGTERM:
                            case SIGINT:
                            {
                                printf("kill all the child now\n");
                                for(int j=0;j<PROCESS_COUNT;++j)
                                {
                                    int pid=sub_process[j].pid;
                                    kill(pid,SIGTERM);
                                } //process count
                                break;
                            }//case SIGINT
                            defaut:
                            {
                                break;
                            }//default
                        }//switch
                    }//for  -->ret
                }//else signal_pipefd[0] and EPOLLIN
            }
            else
            {
                continue;
            }
        }//for  --->ready_fd_num
    } // if  ---->epoll_wait
} 












