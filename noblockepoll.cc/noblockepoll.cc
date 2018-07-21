#include<sys/socket.h>
#include<sys/epoll.h>
#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<string.h>


int setnoblocking(int fd)
{
    int old_option=fcnt(fd ,F_GETFL);
    int new_option=old_option | O_NOBLOCK;
    fcntl(fd,SETFL,new_option_);
    return old_option
}


