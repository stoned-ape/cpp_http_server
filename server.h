#pragma once
//C++
#include <regex>
using std::regex,std::regex_match,std::regex_replace;
#include <string>
using std::string,std::to_string;
#include <vector>
using std::vector;
#include <iostream>
using std::cout;
#include <map>
using std::map;
#include <sstream>
using std::stringstream;
//UNIX
#include <sys/socket.h>
#include <sys/param.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <signal.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
//C-STD
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <limits.h>
#include <libgen.h>
//local
#include "http.h"



#define SYSCALL_NOEXIT(call)({ \
    int syscall_ret=call; \
    if(syscall_ret==-1){ \
        fprintf(stderr,"syscall error: (%s) in function %s at line %d of file %s\n", \
            strerror(errno),__func__,__LINE__,__FILE__); \
        fprintf(stderr,"-> SYSCALL(%s)\n",#call); \
    } \
    syscall_ret; \
})

//exits on error
#define SYSCALL(call)({ \
    int syscall_ret=SYSCALL_NOEXIT(call); \
    if(syscall_ret==-1) exit(errno); \
    syscall_ret; \
})


http basic_callback([[maybe_unused]]http request){
    auto response=http();
    return response;
}

typedef decltype(&basic_callback) callback_type;

http file_server_callback(http req){
    cout<<"callback fired\n";
    http rsp;
    string path="."+req.path;
    // path=regex_replace(path,regex("\\.\\.+"),".");
    int fd=SYSCALL_NOEXIT(open(path.c_str(),O_RDONLY));
    if(fd==-1) return http::not_found();
    int len=SYSCALL(lseek(fd,0,SEEK_END));
    SYSCALL(lseek(fd,0,SEEK_SET));
    char buf[len+1];
    int n=SYSCALL_NOEXIT(read(fd,buf,len));
    if(n==-1){
        if(errno==EISDIR){
            //tried to open a directory
            stringstream data;
            data<<"<html>\n";
            data<<"<h1>Directory listing for "<<path<<"</h1>\n";
            struct dirent *d;
            DIR *dp=opendir(path.c_str());
            if(*--path.end()!='/') path+='/';
            while((d=readdir(dp))!=nullptr){
                data<<"<a href=/"<<path<<d->d_name<<">"<<d->d_name<<"</a><br>\n";
            }
            closedir(dp);
            data<<"</html>\n";
            rsp.set_data(data);
            rsp.header_fields["Content-type"]="text/html";
            return rsp;
        }else return http::server_error();
    }
    assert(n==len);
    buf[len]=0;
    vector<char> data;
    for(int i=0;i<n;i++) data.push_back(buf[i]);
    rsp.set_data(data);
    SYSCALL(close(fd));
    return rsp;
};


struct server{
    uint16_t host_port;
    uint16_t peer_port;
    string hostname;
    string host_ip;
    string peer_ip;
    map<string,callback_type> callbacks;
    server(uint16_t p):host_port(p){
        char hn[1024];
        SYSCALL(gethostname(hn,sizeof(hn)));
        hostname=hn;

        struct hostent *he;
        he=gethostbyname(hn);
        host_ip=inet_ntoa(*(struct in_addr*)he->h_addr_list[0]);
        cout<<"host_port: "<<host_port<<"\n";
        cout<<"hostname:  "<<hostname<<"\n";
        cout<<"host_ip:   "<<host_ip<<"\n";
    }
    void set_callback(string path_rgx,callback_type cb){
        callbacks[path_rgx]=cb;
        assert(callbacks[path_rgx]==cb);
    }
    static http await_request(int sockfd){
        static char buf[1024];
        vector<char> req;
        int n;
        do{
            n=SYSCALL(recv(sockfd,buf,sizeof(buf),0));
            for(int i=0;i<n;i++) req.push_back(buf[i]);
        }while(n==sizeof(buf));
        http req_struct(req);
        return req_struct;
    }
    static void send_response(int sockfd,http response){
        auto str=response.to_vec();
        int sent=0;
        do{
            sent+=SYSCALL(send(sockfd,&str[0]+sent,str.size()-sent,0));
        }while(sent<(int)str.size());
        assert(sent==(int)str.size());
    }
    static http fetch(string domain,uint16_t port,http req){
        struct addrinfo hints;
        memset(&hints,0,sizeof hints); 
        hints.ai_family=AF_UNSPEC;     
        hints.ai_socktype=SOCK_STREAM;
        hints.ai_flags=AI_PASSIVE;    

        struct addrinfo *res;

        char hn[1024];
        gethostname(hn,sizeof(hn));
        getaddrinfo(domain.c_str(),to_string(port).c_str(),&hints,&res);
        
        int sockfd=SYSCALL(socket(res->ai_family,res->ai_socktype,res->ai_protocol));
        
        SYSCALL(connect(sockfd,res->ai_addr,res->ai_addrlen));
        send_response(sockfd,req);
        auto rsp=await_request(sockfd);
        freeaddrinfo(res);
        return rsp;
    }
    static http get(string url){
        url_data x(url);
        x.req.header_fields["Accept"]="*/*";
        x.req.header_fields["User-Agent"]="curl/7.64.1";
        return fetch(x.domain,x.port,x.req);
    }
    static http wget(string url,string fname=""){
        http rsp=get(url);
        if(fname==""){
            url_data x(url);
            char c[MAXPATHLEN];
            memcpy(c,x.req.path.c_str(),MAXPATHLEN);
            fname=basename(c);
        }
        int fd=SYSCALL(open(fname.c_str(),O_CREAT|O_TRUNC|O_WRONLY));
        SYSCALL(write(fd,&rsp.data[0],rsp.data.size()));
        return rsp;
    }
    void run(){
        int fd=SYSCALL(socket(AF_INET,SOCK_STREAM,0));
        struct sockaddr_in sa;
        memset(&sa,0,sizeof sa);
        sa.sin_family=AF_INET;
        sa.sin_port=htons(host_port);


        sa.sin_addr.s_addr=INADDR_ANY;
        int yes=1;
        SYSCALL(setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)));
        SYSCALL(bind(fd,(struct sockaddr*)&sa,sizeof(sa)));
        SYSCALL(listen(fd,100));

        //reap dead children
        signal(SIGCHLD,[](int){
            int tmp=errno;
            int num_proc;
            do{
                num_proc=waitpid(-1,NULL,WNOHANG);
            }while(num_proc>0);
            errno=tmp;
        });

        if(SYSCALL(getuid())==0){
            char cwd[MAXPATHLEN];
            if(!getwd(cwd)) SYSCALL(-1);
            SYSCALL(chroot(cwd));
            int newuid=-2;
            SYSCALL(setuid(newuid));
        }

        while(1){
            uint32_t len=sizeof(struct sockaddr_in);
            printf("accepting \n");
            int nfd=SYSCALL(accept(fd,(struct sockaddr*)&sa,&len));
            printf("accepted \n");
        
            struct sockaddr_in peer_sa;
            memset(&peer_sa,0,sizeof peer_sa);
            uint32_t n=sizeof(peer_sa);
            getpeername(nfd,(struct sockaddr*)&peer_sa,&n);
            peer_ip=inet_ntoa(peer_sa.sin_addr);
            peer_port=peer_sa.sin_port;
            cout<<"peer_ip:   "<<peer_ip<<'\n';
            cout<<"peer_port: "<<peer_port<<'\n';

            int pid=SYSCALL(fork());
            if(pid==0){
                http req=await_request(nfd);
                http rsp;
                bool match=false;
                for(auto [k,cb]:callbacks){
                    if(regex_match(req.path,regex(k))){
                        rsp=cb(req);
                        match=true;
                        break;
                    }
                }
                if(!match)         rsp=http::not_found();
                else if(req.error) rsp=http::bad_request();
                send_response(nfd,rsp);
                SYSCALL(close(nfd));
                exit(0);
            }
        }
    }
};































//