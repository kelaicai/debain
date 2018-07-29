#include<iostream>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<sstream>
#include<unistd.h>
#include"cppjson.h"
#include"./configure.h"
using namespace std;


string get_data(const char *file_name)
{
    assert(file_name!=NULL);
    int fd=open(file_name,O_RDONLY);
    if(fd==-1)
    {
        perror("open error");
        return "";
    }
    int file_file=lseek(fd,0,SEEK_END);
    lseek(fd,0,SEEK_SET);
    std::string str;
    while(true)
    {
        char buff[128]={0};
        if(!read(fd,buff,127))
        {
            break;
        }
        str+=buff;
    }
    return str;
}


Configure::Configure()
{
    cout<<"system configring"<<endl;
    
    try{
    json::Value val;
    std::string str=get_data("./configure.json");
    std::istringstream ss(str);
    val.load_all(ss);
    /*
    std::cout<<val.get("dispatch").as_object()["ip"].as_string()<<std::endl;
    std::cout<<val.get("dispatch").as_object()["port"].as_integer()<<std::endl;
    std::cout<<val.get("timeout").as_integer()<<endl;
    */
    
    dispatch.port=val.get("dispatch").as_object()["port"].as_integer();
    dispatch.ip=val.get("dispatch").as_object()["ip"].as_string();
    loader_method=val.get("balance_method").as_string();
   cout<<"hello"<<endl; 

    //some servers connected with dispatch
    vector<json::Value> sers=val.get("server").as_array();
    for(int i=0;i<sers.size();i++)
    {
        server_info se;
        se.ip=sers[i].as_object()["ip"].as_string();
        se.port=sers[i].as_object()["port"].as_integer();
        
        vec_ser.push_back(se);
        // cout<<"ip="<<ip<<" "<<port<<endl;
        
    } //for1

    }catch(const json::type_error &e)
    {
        assert(e.what()==std::string("exception type incteger,but got boolen"));
    }
      //construc
}


string Configure::get_loader_method()
{
    return loader_method;
}


dispatch_info Configure::get_dispatch_info()
{
    return dispatch;
}

vector<server_info> Configure::get_server_info()
{
   return vec_ser;
}
