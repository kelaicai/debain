/*************************************************************************
	> File Name: hash.h
	> Author: 
	> Mail: 
	> Created Time: 2018年07月22日 星期日 00时11分04秒
 ************************************************************************/

#ifndef _HASH_H
#define _HASH_H
#endif
#include<map>
#include<vector>
#include<iostream>
#include"./md5.h"
#include<string>
using namespace std;
class Md5{
  public:
    string get_md5(const string &str) {
      return MD5(str).toStr();
    }
};

struct Node
{
    int vnode_count;
    string main_msg;  //repsent IP
    vector<string> vnode_msg;  //MD5(IP#I)
    string local_key;  //repsent MD5(IP)
    Node(string key,const string &msg);
    Node(){}
};
Node::Node(string key,const string &msg):local_key(key),main_msg(msg)
{

}

class RealNode
{
 public:
   typedef string (*hock)(const string &,int );
    map<string,Node>  node;  //node store md5#i md5(ip)
    static hock hock_info;
    string add_node(const string &msg);  //add original Node
    string add_vnode(const string key,int flag);
    static void setHock(hock fun);
    vector<string> del_node(const string &msg);
    string get_msg(const string &key);  //search key's read; msg
};

RealNode::hock RealNode::RealNode::hock_info=NULL;


class CHash
{
    public:

   CHash(int num,const vector<string> &msg);
    //relation store the relationship bettewn vndoe's and ser ip
    void add_serve(const string &msg);
    void del_server(const string &msg);
    string find(const string &msg);
    void show_servers();
    private:
    map<string,string> relation;
    RealNode rnodes;
    int server_count;
};


string RealNode::get_msg(const string &msg)
{
    return node[msg].main_msg;
}

string RealNode::add_node(const string &ipidx)
{
    string new_key=Md5().get_md5(ipidx);
    node[new_key]=Node(new_key,ipidx);
    return new_key;
}

string RealNode::add_vnode(const string key,int num)
{
    const string vnode_md5=hock_info(node[key].main_msg,num);
    string new_key=Md5().get_md5(vnode_md5);
    node[key].vnode_msg.push_back(vnode_md5);
    return new_key;
}

void RealNode::setHock(hock cfun)
{
    hock_info=cfun;
}

vector<string> RealNode::del_node(const string &msg)
{
    string key=Md5().get_md5(msg);
    map<string,Node>::iterator it=node.find(key);
    vector<string> ret=(it->second).vnode_msg;
    ret.push_back(key);
    node.erase(it);
    return ret;
}


//chash
CHash::CHash(int num,const vector<string> &msg):server_count(num)
{
    if(num!=msg.size())  return ;
    string key;
    string new_key;

    int vnode_count=3;
    vector<string>::const_iterator cit=msg.begin();

    for(;cit!=msg.end();++cit)
    {
        key=rnodes.add_node(*cit); //
        relation[key]=key;  //key---->ip
        for(int i=0;i<vnode_count;++i)
        {
            new_key=rnodes.add_vnode(key,i);  //add vnode_info to rnode  --->map<string,sring>
            relation[new_key]=key;
        }
    }
}

void CHash::del_server(const string &msg)
{
    //delete the real node
    vector<string> vnode_key=rnodes.del_node(msg);
    for(int i=0;i<vnode_key.size();++i)
    {
        //delete realnode realation's vnode
        relation.erase(relation.find(vnode_key[i]));
    }
}


string CHash::find(const string &msg)
{
    string dest_key=Md5().get_md5(msg);
     map<string,string>::const_iterator cit=relation.begin();
    for(;cit!=relation.end();++cit)
    {
        if(cit->first==dest_key)
        {
            break;
        }
    }
    if(cit==relation.end())   cit=relation.begin();
    return rnodes.get_msg(cit->second);
}


void CHash::show_servers()
{
    cout<<"all nodes is"<<endl;
    typename map<string,string>::const_iterator cit=relation.begin();
    for(;cit!=relation.end();++cit){
        cout<<cit->first<<"-------->"<<rnodes.get_msg(cit->second)<<endl;
    }
}
