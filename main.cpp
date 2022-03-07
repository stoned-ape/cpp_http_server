//UNIX
#include <pthread.h>
#include <sys/mman.h>
//C-STD
#include <assert.h>
//local
#include "server.h"
//C++
#include <iostream>
using namespace std;


#define PTHREAD(call)({ \
    int ret=call; \
    if(ret!=0){ \
        fprintf(stderr,"pthread error: (%s) in function %s at line %d of file %s\n", \
            strerror(ret),__func__,__LINE__,__FILE__); \
        printf("-> PTHREAD(%s)\n",#call); \
        exit(ret); \
    } \
})


void test_findall(){
    auto v=findall("a b c def","\\w+");
    assert(v[0]=="a");
    assert(v[1]=="b");
    assert(v[2]=="c");
    assert(v[3]=="def");
}


void test_http(){
    //test request
    assert(regex_match("/curl_request",regex("[^]*")));
    assert(regex_match("\r\n\r\n",regex("\r\n\r\n")));
    string str="GET / HTTP/3.7\r\n\r\n";
    auto h0=http(str);
    assert(h0.method=="GET");
    assert(h0.path=="/");
    assert(h0.maj_version==3);
    assert(h0.min_version==7);
    assert(h0.status==-1);
    assert(str==h0.to_str());

    str="GET /a/b/c.ico HTTP/1.1\r\n\r\n";
    h0=http(str);
    assert(h0.path=="/a/b/c.ico");
    assert(str==h0.to_str());

    str="GET /a/b/c/ HTTP/1.1\r\n\r\n";
    h0=http(str);
    assert(h0.path=="/a/b/c/");
    assert(str==h0.to_str());

    str="GET /a/b/c?a=b HTTP/1.1\r\n\r\n";
    h0=http(str);
    assert(h0.path=="/a/b/c");
    assert(h0.raw_get_params=="a=b");
    assert(h0.get_params["a"]=="b");
    assert(str==h0.to_str());

    str="GET /a/b/c?a=b;c=d HTTP/1.1\r\n\r\n";
    h0=http(str);
    assert(h0.raw_get_params=="a=b;c=d");
    assert(h0.get_params["a"]=="b");
    assert(h0.get_params["c"]=="d");
    assert(str==h0.to_str());

    str="GET / HTTP/1.1\r\na: b\r\nc: d\r\ne-e: f\r\n\r\n";
    h0=http(str);
    assert(h0.header_fields["a"]=="b");
    assert(h0.header_fields["c"]=="d");
    assert(h0.header_fields["e-e"]=="f");
    assert(str==h0.to_str());

    str="GET / HTTP/1.1\r\n\r\nsome data";
    h0=http(str);
    assert(h0.data==str2vec("some data"));
    assert(str==h0.to_str());

    str="GET / HTTP/1.1\r\nCookie: a=b\r\n\r\n";
    h0=http(str);
    assert(h0.header_fields["Cookie"]=="a=b");
    assert(h0.get_cookie()["a"]=="b");
    assert(str==h0.to_str());


    str="GET / HTTP/1.1\r\nCookie: a=b;c=d;e=f\r\n\r\n";
    h0=http(str);
    assert(h0.header_fields["Cookie"]=="a=b;c=d;e=f");
    assert(h0.get_cookie()["a"]=="b");
    assert(h0.get_cookie()["c"]=="d");
    assert(h0.get_cookie()["e"]=="f");
    assert(str==h0.to_str());

    //test with curl request
    str="GET / HTTP/1.1\r\nHost: 127.0.0.1:port\r\nUser-Agent: curl/7.64.1\r\nAccept: */*\r\n\r\n";
    h0=http(str);
    assert(h0.header_fields["Host"]=="127.0.0.1:port");
    assert(h0.header_fields["User-Agent"]=="curl/7.64.1");
    assert(h0.header_fields["Accept"]=="*/*");

    //test response
    str="HTTP/4.5 410 Gone\r\n\r\n";
    h0=http(str);
    // h0.print();
    // cout<<h0.to_str()<<'\n';
    assert(h0.method=="");
    assert(h0.path=="");
    assert(h0.status==410);
    assert(h0.maj_version==4);
    assert(h0.min_version==5);
    assert(str==h0.to_str());

    str="HTTP/1.1 200 OK\r\na: b\r\n\r\n";
    h0=http(str);
    assert(h0.status==200);
    assert(h0.header_fields["a"]=="b");
    assert(str==h0.to_str());

    str="HTTP/1.1 404 Not Found\r\na: b\r\n\r\n";
    h0=http(str);
    assert(h0.status==404);
    assert(str==h0.to_str());

    str="HTTP/1.1 200 OK\r\na: b\r\nc: d\r\ne: f\r\n\r\n";
    h0=http(str);
    assert(h0.header_fields["a"]=="b");
    assert(h0.header_fields["c"]=="d");
    assert(h0.header_fields["e"]=="f");
    assert(str==h0.to_str());

    str="HTTP/1.1 200 OK\r\nCookie: a=b\r\n\r\n";
    h0=http(str);
    assert(h0.header_fields["Cookie"]=="a=b");
    assert(h0.get_cookie()["a"]=="b");
    assert(str==h0.to_str());

    str="HTTP/1.1 200 OK\r\nCookie: a=b;c=d;e=f\r\n\r\n";
    h0=http(str);
    assert(h0.header_fields["Cookie"]=="a=b;c=d;e=f");
    assert(h0.get_cookie()["a"]=="b");
    assert(h0.get_cookie()["c"]=="d");
    assert(h0.get_cookie()["e"]=="f");
    assert(str==h0.to_str());

    str="HTTP/1.1 200 OK\r\n\r\nsome more data";
    h0=http(str);
    assert(h0.data==str2vec("some more data"));
    assert(str==h0.to_str());

    //test get/set cookie
    auto ck=h0.get_cookie();
    ck["a"]="b";
    ck["c"]="d";
    h0.set_cookie(ck);
    assert(h0.header_fields["Cookie"]=="a=b;c=d");
    ck=h0.get_cookie();
    ck["c"]="dd";
    ck["e"]="f";
    h0.set_cookie(ck);
    assert(h0.header_fields["Cookie"]=="a=b;c=dd;e=f");
}


void test_url(){
    string url;
    url_data ud;
    url="http://google.com/";
    ud=url_data(url);
    assert(ud.port==80);
    assert(ud.domain=="google.com");
    assert(ud.req.path=="/");
    assert(ud.req.method=="GET");
    assert(ud.req.get_params.size()==0);
    assert(ud.to_str()==url);

    url="http://google.com:443/";
    ud=url_data(url);
    // cout<<ud.port<<'\n';
    assert(ud.port==443);
    assert(ud.domain=="google.com");
    assert(ud.to_str()==url);

    url="http://127.0.0.1:8000/";
    ud=url_data(url);
    assert(ud.port==8000);
    assert(ud.domain=="127.0.0.1");
    assert(ud.to_str()==url);

    url="http://127.0.0.1:8000/a";
    ud=url_data(url);
    assert(ud.port==8000);
    assert(ud.domain=="127.0.0.1");
    assert(ud.req.path=="/a");
    assert(ud.to_str()==url);

    url="http://127.0.0.1:8000/a/b/c/";
    ud=url_data(url);
    assert(ud.port==8000);
    assert(ud.domain=="127.0.0.1");
    assert(ud.req.path=="/a/b/c/");
    assert(ud.to_str()==url);

    url="http://127.0.0.1:8000/a/b/c/?a=b";
    ud=url_data(url);
    assert(ud.port==8000);
    assert(ud.domain=="127.0.0.1");
    assert(ud.req.path=="/a/b/c/");
    assert(ud.req.get_params["a"]=="b");
    cout<<ud.to_str()<<'\n';
    assert(ud.to_str()==url);

    url="http://127.0.0.1:8000/a/b/c/?a=b;c=d;e=f";
    ud=url_data(url);
    assert(ud.port==8000);
    assert(ud.domain=="127.0.0.1");
    assert(ud.req.path=="/a/b/c/");
    assert(ud.req.get_params["a"]=="b");
    assert(ud.req.get_params["c"]=="d");
    assert(ud.req.get_params["e"]=="f");
    assert(ud.to_str()==url);
}


class multi_process_condition_variable{
    pthread_mutexattr_t attrmutex;
    pthread_condattr_t  attrcond;
    pthread_mutex_t     *mutex;
    pthread_cond_t      *cond_var;
public:
    multi_process_condition_variable(){
        auto map_shared=[](int len)->void*{
            void *ptr=mmap(
                NULL,len,
                PROT_READ|PROT_WRITE,
                MAP_SHARED|MAP_ANONYMOUS,
                -1,0
            );
            SYSCALL((long)ptr);
            return ptr;
        };
        PTHREAD(pthread_mutexattr_init(&attrmutex));
        PTHREAD(pthread_mutexattr_setpshared(&attrmutex,PTHREAD_PROCESS_SHARED));
        mutex=(pthread_mutex_t*)map_shared(sizeof(pthread_mutex_t));
        PTHREAD(pthread_mutex_init(mutex, &attrmutex));
        PTHREAD(pthread_condattr_init(&attrcond));
        PTHREAD(pthread_condattr_setpshared(&attrcond,PTHREAD_PROCESS_SHARED));
        cond_var=(pthread_cond_t*)map_shared(sizeof(pthread_cond_t));
        PTHREAD(pthread_cond_init(cond_var,&attrcond));
    }
    void wait(){
        PTHREAD(pthread_cond_wait(cond_var,mutex));
    }
    void signal(){
        PTHREAD(pthread_cond_signal(cond_var));
    }
    ~multi_process_condition_variable(){
        PTHREAD(pthread_condattr_destroy(&attrcond)); 
        PTHREAD(pthread_cond_destroy(cond_var));
        PTHREAD(pthread_mutexattr_destroy(&attrmutex)); 
        PTHREAD(pthread_mutex_unlock(mutex)); //must be unlocked before destroying
        PTHREAD(pthread_mutex_destroy(mutex));
        SYSCALL(munmap(mutex   ,sizeof(pthread_mutex_t)));
        SYSCALL(munmap(cond_var,sizeof(pthread_cond_t )));
    }

};

//need a custom assert that kills the server before exiting
#define tmp_assert(exp,pid)({ \
    if(!(exp)){ \
        fprintf(stderr,"Assertion failed: (%s), function %s, file %s, line %d.\n", \
            #exp,__func__,__FILE__,__LINE__); \
        SYSCALL(kill((pid),SIGKILL)); \
        exit(-1); \
    } \
})

void test_server(){
    //we dont want the client to send requests to the server 
    //until it is ready to accept them
    //in a multi-threaded context we would do this with condition variables
    //since the client and server are separate processes the
    //condition variable must be shared between them with shared memory
    multi_process_condition_variable cv;


    uint16_t port=8080;
    int pid=SYSCALL(fork());
    
    if(pid==0){
        cout<<"child\n";
        server sv(port);
        sv.set_callback("/a",[](http)->http{
            http rsp;
            stringstream ss;
            ss<<"some data";
            rsp.set_data(ss);
            return rsp;
        });

        sv.set_callback("/goaway",[](http)->http{
            return http::redirect("/");
        });

        sv.set_callback("/cookie",[](http req)->http{
            http rsp;
            stringstream ss;
            ss<<req.header_fields["Cookie"];
            rsp.set_data(ss);
            return rsp;
        });
        //tell the parent we are ready for them to connect
        cv.signal();
        sv.run();

        exit(0);
    }
    //wait until the server is ready to accept a connection
    cv.wait();
    cout<<"parent: child pid="<<pid<<"\n";
    http req,rsp;
    req=http("GET /a HTTP/1.1\r\n\r\n");

    rsp=server::fetch("127.0.0.1",port,req);
    tmp_assert(vec2str(rsp.data)=="some data",pid);

    // rsp=server::fetch("localhost",port,req);
    // tmp_assert(vec2str(rsp.data)=="some data");

    rsp=server::fetch("0.0.0.0",port,req);
    tmp_assert(vec2str(rsp.data)=="some data",pid);

    //server should be accessible via its hostname
    char hn[512];
    gethostname(hn,sizeof(hn));
    string hostname=hn;
    rsp=server::fetch(hostname,port,req);
    tmp_assert(vec2str(rsp.data)=="some data",pid);


    req=http("GET /goaway HTTP/1.1\r\n\r\n");
    rsp=server::fetch("127.0.0.1",port,req);
    tmp_assert(rsp.status==303,pid);
    tmp_assert(rsp.header_fields["Location"]=="/",pid);

    req=http("GET /cookie HTTP/1.1\r\nCookie: a=b;c=d\r\n\r\n");
    rsp=server::fetch("127.0.0.1",port,req);
    tmp_assert(vec2str(rsp.data)=="a=b;c=d",pid);

    req=http("GET /invalidurl HTTP/1.1\r\n\r\n");
    rsp=server::fetch("127.0.0.1",port,req);
    tmp_assert(rsp.status==404,pid);

    rsp=server::get("http://127.0.0.1:8080/a");
    tmp_assert(vec2str(rsp.data)=="some data",pid);

    // rsp=server::get("http://localhost:8080/a");
    // tmp_assert(vec2str(rsp.data)=="some data");

    rsp=server::get("http://www.google.com/");
    tmp_assert(rsp.status==200,pid);
    

    //done testing, kill the server
    SYSCALL(kill(pid,SIGKILL));
    //reap the dead process
    SYSCALL(waitpid(pid,0,WNOHANG));
}



void test_file_server(){
    multi_process_condition_variable cv;

    uint16_t port=8080;
    int pid=SYSCALL(fork());
    
    if(pid==0){
        cout<<"child\n";
        server sv(port);
        sv.set_callback("/[^]*",file_server_callback);
        //tell the parent we are ready for them to connect
        cv.signal();
        sv.run();

        exit(0);
    }
    //wait until the server is ready to accept a connection
    cv.wait();
    cout<<"parent: child pid="<<pid<<"\n";
    http rsp;

    rsp=server::wget("http://127.0.0.1:8080/favicon.ico","f1.ico");
    
    tmp_assert(0==system("diff favicon.ico f1.ico"),pid);

    //done testing, kill the server
    SYSCALL(kill(pid,SIGKILL));
    //reap the dead process
    SYSCALL(waitpid(pid,0,WNOHANG));
}


int main(int argc,char **argv){
    if(argc==2 && string(argv[1])=="-fs"){
        server sv(8000);
        sv.set_callback("/[^]*",file_server_callback);
        sv.run();
    }else{
        test_findall();
        test_http();
        test_url();
        // return 0;
        test_server();
        test_file_server();
        // test_ping();
    }    

}




























//