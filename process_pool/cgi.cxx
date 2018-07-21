#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<assert.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<fcntl.h>
#include<sys/epoll.h>
#include<signal.h>
#include<sys/wait.h>
#include<sys/stat.h>

#include"process_pool.h"

class cgi_conn
{
    public:
    cgi_conn(){}
    ~cgi_conn(){}
    
    void init(int epollfd,int sockfd,const sockaddr_in client_addr)
    {
        m_epollfd=epollfd;
        m_sockfd=sockfd;
        m_address=client_addr;
        memset(m_buff,'\0',BUFFER_SIZE);
        m_read_idx=0;
    }

    void process()
    {
        int idx=0;
        int ret=-1;
        while(true)
        {
            idx=m_read_idx;
            ret=recv(m_sockfd,m_buff+idx,BUFFER_SIZE-1-idx,0);
            //if r operatirn happen error,close the client connecion,t.but if there is no data
            //to read ,then quit loop
            if(ret<0)
            { 
                if(errno!=EAGAIN)
               {
                   removefd(m_epollfd,m_sockfd);
               }
            }
            else if(ret==0)  //client close it's  connecion
            {
                removefd(m_epollfd,m_sockfd);
                break;
            }
            else
            {
                m_read_idx+=ret;
                printf("user conent is :%s\n",m_buff);
                //if meet charactor "\r\n" then begin to do client requst
                for(;idx<m_read_idx;++idx)
                {
                    if((idx>=1) && (m_buff[idx-1]=='r') && (m_buff[idx]=='\n'))
                    {
                        break;
                    }
                    //if no meet "\r\n" means to read more client data;
                    if(idx==m_read_idx)
                    {
                       continue;
                        
                    }
                    m_buff[idx-1]='\0';
                    char *file_name=m_buff;
                    //judge client is exsist
                    if(access(file_name,F_OK)==-1)
                    {
                        removefd(m_epollfd,m_sockfd);
                        break;
                    }
                    //create new sub process progame
                    ret=fork();
                    if(ret==-1)
                    {
                        removefd(m_epollfd,m_sockfd);
                        break;
                    }
                    else if(ret>0)
                    {
                        //father only close connetion
                        removefd(m_epollfd,m_sockfd);
                        break;
                    }
                    else
                    {
                        close(STDOUT_FILENO);
                        dup(m_sockfd);
                        execl(m_buff,m_buff,0);
                        exit(0);
                    }
                }
            }    
        }
        
    }

    private:
     static  const int BUFFER_SIZE=1024;
    static int m_epollfd;
    int m_sockfd;
    sockaddr_in m_address;
    char m_buff[BUFFER_SIZE];

    //flag to read buff's last byte of next positio
    int m_read_idx;
};
 cgi_conn::m_epollfd=-1;

int main(int argc,char* argv[])
{
    if(argc<=2)
    {
        printf("Usage %s ip_address port_numbrer\n",basename(argv[0]));
        return 1;
    }

    const char *ip=argv[1];
    int port=atoi(argv[2]);

    int listenfd=socket(PF_INET,SOCK_STREAM,0);

    assert(listenfd>=0);

    int ret=0;
    struct sockaddr_in address;
    bzero(&address,sizeof(address));
    address.sin_family=AF_INET;
    inet_pton(AF_INET,ip,&address.sin_addr);
    address.sin_port=htons(port);

    ret=bind(listenfd,(struct sockaddr*)&address,sizeof(address));
    assert(ret!=-1);

    ret=listen(listenfd,5);
    assert(ret!=-1);

    processpool<cgi_conn>* pool=processpool<cgi_conn>::create(listenfd);
    if(pool)
    {
        pool->run();
        delete pool;
    }
    
    close(listenfd);
    return  0;
    
}
