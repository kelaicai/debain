/*************************************************************************
	> File Name: http_redirection.cpp
	> Author: 
	> Mail: 
	> Created Time: 2018年07月29日 星期日 00时55分44秒
 ************************************************************************/
#include<cstring>
#include<iostream>
#include<sys/socket.h>
#include"http_redirection.h"
using namespace std;


//myserver is a struct server_info from chash's result 
bool http_redirection(int clientfd,int code,string ip_port)
{
    if(clientfd<0)
    {
        cout<<"clientfd is error,it's a sub number"<<endl;
    }
  	char buff[1024]={0};
	char scode[16]={0};
	char status[32]="Move Permanently";

 // reponse proto
	strcat(buff,"HTTP/1.1");
	sprintf(scode,"%d",code);
	strcat(buff," ");
	strcat(buff,scode);
	strcat(buff," ");
	strcat(buff,status);
	strcat(buff,"\r\n");

  //serve msg
	strcat(buff,"Serve: ");
	strcat(buff,"My_serve");
	strcat(buff,"\r\n");
	
  //location
    strcat(buff,"location:");
    string url="http://";
    url+=ip_port;
    url+="/";
    strcat(buff,url.c_str());
	strcat(buff,"\r\n");
    strcat(buff,"\r\n");
    int res=send(clientfd,buff,strlen(buff),0);
    bool flag=false;
    if(res>0)
    {
        cout<<"send status "<<res<<endl;
        cout<<buff<<endl;
        flag=true;
    }
    else{
        perror("send error:");
    }
    return flag;
}



