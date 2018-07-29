/*************************************************************************
	> File Name: configure.h
	> Author: 
	> Mail: 
	> Created Time: 2018年07月29日 星期日 00时49分49秒
 ************************************************************************/

#ifndef _CONFIGURE_H
#define _CONFIGURE_H
#endif
#include<iostream>
#include<string>
#include<vector>
#include<cstring>
#pragma once
using namespace std;
typedef struct configure_info{
    string ip;
    int port;
}dispatch_info,server_info;


class Configure{
    public:
    
    Configure();
    string get_loader_method();
    dispatch_info get_dispatch_info();
    vector<server_info> get_server_info();
    private:
    dispatch_info dispatch;
    vector<server_info> vec_ser;  //ip--->port
    string loader_method;
};


extern string get_data(const char* file_name);
