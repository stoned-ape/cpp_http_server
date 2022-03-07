#pragma once
//C++
#include <regex>
using std::regex,std::regex_search,std::smatch;
using std::regex_match,std::regex_replace;
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
//C-STD
#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>

vector<string> findall(string s,string rgx){
    vector<string> v;
    string rem=s;
    while(rem.size()>0){
        smatch sm;
        regex_search(rem,sm,regex(rgx));
        for(int i=0;i<(int)sm.size();i++){
            v.push_back(sm.str(i));
        }
        int sz=sm.prefix().str().size()+sm.str(0).size();
        if(sz==0) break;
        rem=rem.substr(sz);
    }
    return v;
}

vector<char> str2vec(string s){
    vector<char> v;
    for(auto i:s) v.push_back(i);
    return v;
}

string vec2str(vector<char> v){
    string s;
    for(auto i:v) s+=i;
    return s;
}

string status2stat_str(int status){
    switch(status){
        case 200: return "OK";
        case 201: return "Created";
        case 202: return "Accepted";
        case 203: return "Non-Authoritative Information";
        case 204: return "No Content";
        case 205: return "Reset Content";
        case 206: return "Partial Content";
        case 300: return "Multiple Choices";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 303: return "See Other";
        case 304: return "Not Modified";
        case 305: return "Use Proxy";
        case 307: return "Temporary Redirect";
        case 409: return "Conflict";
        case 410: return "Gone";
        case 411: return "Length Required";
        case 412: return "Precondition Failed";
        case 413: return "Request Entity Too Large";
        case 414: return "Request-URI T oo Long";
        case 415: return "Unsupported Media Type";
        case 416: return "Requested Range Not Satisfiable";
        case 417: return "Expectation Failed";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 402: return "Payment Required";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 406: return "Not Acceptable";
        case 407: return "Proxy Authentication Required";
        case 408: return "Request Timeout";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        case 504: return "Gateway Timeout";
        case 505: return "HTTP Version Not Supported";
    }
    return "OK";
}


struct http{
    int status;
    string method;
    string path;
    map<string,string> get_params;
    string raw_get_params;
    int maj_version;
    int min_version;
    map<string,string> header_fields;
    vector<char> data;
    bool error;
    void set_data(vector<char> _data){
        data=_data;
        header_fields["Content-Length"]=::to_string(data.size());
    }
    void set_data(string _data){
        set_data(str2vec(_data));
    }
    void set_data(stringstream &_data){
        set_data(_data.str());
    }
    map<string,string> get_cookie(){
        map<string,string> cookie;
        for(auto &[k,v]:header_fields){
            if(k=="Cookie"){
                auto v2=findall(v,"\\w+=[^;]+");
                for(auto i:v2){
                    auto v3=findall(i,"(\\w+)=([^;]+)");
                    cookie[v3[1]]=v3[2];
                }
            }
        }
        return cookie;
    }
    void set_cookie(map<string,string> cookie){
        stringstream ss;
        int i=0;
        for(auto &[k,v]:cookie){
            if(i!=0) ss<<";";
            ss<<k<<"="<<v;
            i++;
        }
        header_fields["Cookie"]=ss.str();
    }
    http(){
        status=200;
        path="/";
        maj_version=1;
        min_version=1;
        set_data("<h1>defualt response</h1>");
        header_fields["Connection"]="close";
        error=false;
    }
private:
    static string error_data(int status){
        stringstream ss;
        ss<<"<h1>"<<status<<" ("<<status2stat_str(status)<<")<h1>\n";
        return ss.str();
    }
public:
    static http redirect(string path){
        http rsp;
        rsp.status=303;
        rsp.header_fields["Location"]=path;
        rsp.set_data(error_data(rsp.status));
        return rsp;
    }
    static http not_found(){
        http rsp;
        rsp.status=404;
        rsp.set_data(error_data(rsp.status));
        return rsp;
    }
    static http server_error(){
        http rsp;
        rsp.status=500;
        rsp.set_data(error_data(rsp.status));
        return rsp;
    }
    static http bad_request(){
        http rsp;
        rsp.status=400;
        rsp.set_data(error_data(rsp.status));
        return rsp;
    }
private:
    bool valid_method(){
        static string methods[]={"OPTIONS","GET","HEAD","POST","PUT","DELETE","TRACE","CONNECT"};
        for(auto m:methods) if(m==method) return true;
        return false;
    }
public:
    http(vector<char> raw_vec){
        string raw=vec2str(raw_vec);
        error=false;
        string data_rgx=R"(\r\n\r\n([^]*))";
        string header_fields_rgx=R"(((?:\r\n[\w-]+:\s*[^\r]+)*))";
        string req_rgx;
        req_rgx+=R"(([A-Z]+) )"; //GET,POST etc
        req_rgx+=R"(((?:/[\w.]*)+))"; // /path/to/resorce
        req_rgx+=R"((?:\?(\w+=\w+(?:;\w+=\w+)*))?)"; //get parameters
        req_rgx+=R"( HTTP/(\d)\.(\d))"; //HTTP/1.1 prolly
        req_rgx+=header_fields_rgx;
        req_rgx+=data_rgx;

        string rsp_rgx;
        rsp_rgx+=R"(HTTP/(\d)\.(\d) )";
        rsp_rgx+=R"((\d+) )";
        rsp_rgx+=R"(([A-Za-z -]+)?)"; //"OK","Not Found","Bad Request" etc
        rsp_rgx+=header_fields_rgx;
        rsp_rgx+=data_rgx;

        auto set_header_fields=[&](string x){
            auto v0=findall(x,R"([\w-]+:[^\n]+)");
            for(auto i:v0){
                auto v1=findall(i,R"(([\w-]+):[\s]*([^\r]+))");
                header_fields[v1[1]]=v1[2];
            }
        };

        if(regex_match(raw,regex(req_rgx))){
            auto v=findall(raw,req_rgx);
            method=v[1];
            if(!valid_method()){
                error=true;
                return;
            }
            path=v[2];
            if(regex_match(v[3],regex(R"(\w+=\w+(;\w+=\w+)*)"))){
                raw_get_params=v[3];
                auto v0=findall(v[3],R"(\w+=\w+)");
                for(auto i:v0){
                    auto v1=findall(i,R"(\w+)");
                    get_params[v1[0]]=v1[1];
                }
            }
            maj_version=(int)(v[4][0]-'0');
            min_version=(int)(v[5][0]-'0');
            set_header_fields(v[6]);
            data=str2vec(v[7]);
            status=-1;
        }else if(regex_match(raw,regex(rsp_rgx))){
            auto v=findall(raw,rsp_rgx);
            maj_version=(int)(v[1][0]-'0');
            min_version=(int)(v[2][0]-'0');
            status=atoi(v[3].c_str());
            set_header_fields(v[5]);
            data=str2vec(v[6]);
        }else{
            fprintf(stderr,"invalid http string:\n%s\n",raw.c_str());
            error=true;
            return;
        }
        //the stringified response is null terminated
        //but the true response may be longer if it
        //contains binary data
        for(uint64_t i=raw.size();i<raw_vec.size();i++){
            data.push_back(raw_vec[i]);
        }
    }
    http(string raw):http(str2vec(raw)){}
    vector<char> to_vec(){
        stringstream ss;
        if(status!=-1){ //is a response
            ss<<"HTTP/"<<maj_version<<"."<<min_version<<" "<<status<<" ";
            ss<<status2stat_str(status)<<"\r\n";
        }else{
            assert(valid_method());
            ss<<method<<" "<<path;
            if(get_params.size()!=0){
                ss<<"?";
                int i=0;
                for(auto &[k,v]:get_params){
                    if(i!=0) ss<<';';
                    ss<<k<<"="<<v;
                    i++;
                }
            }
            ss<<" HTTP/"<<maj_version<<"."<<min_version<<"\r\n"; 
        }
        for(auto &[k,v]:header_fields){
            ss<<k<<": "<<v<<"\r\n";
        }
        ss<<"\r\n";
        auto v=str2vec(ss.str());
        for(auto i:data) v.push_back(i);
        return v;
    }
    string to_str(){
        return vec2str(to_vec());
    }
    void print(){
        cout<<"http:{\n";
        cout<<"\tstatus:         "<<status<<'\n';
        cout<<"\tstat_str:       "<<status2stat_str(status)<<'\n';
        cout<<"\tmethod:         "<<method<<'\n';
        cout<<"\tpath:           "<<path<<'\n';
        cout<<"\traw_get_params: "<<raw_get_params<<'\n';
        cout<<"\tget_params:{\n";
        for(auto &[k,v]:get_params){
            cout<<"\t\t"<<k<<":"<<v<<'\n';
        }
        cout<<"\t}\n";
        cout<<"\tmaj_version:    "<<maj_version<<'\n';
        cout<<"\tmin_version:    "<<min_version<<'\n';
        cout<<"\theader_fields:{\n";
        for(auto &[k,v]:header_fields){
            cout<<"\t\t"<<k<<":"<<v<<'\n';
        }
        cout<<"\t}\n";
        cout<<"\tcookie:{\n";
        map<string,string> cookie=get_cookie();
        for(auto &[k,v]:cookie){
            cout<<"\t\t"<<k<<":"<<v<<'\n';
        }
        cout<<"\t}\n";
        cout<<"\tdata:           "<<vec2str(data)<<'\n';
        cout<<"}\n";
    }

};



struct url_data{
    http req;
    string domain;
    uint16_t port;
    url_data(){};
    url_data(string url){
        port=80;
        string path="/",url_rgx;
        url_rgx+=R"(http://)";
        url_rgx+=R"(([\w.-]+))"; //domain
        url_rgx+=R"((?::(\d+))?)"; //port number
        url_rgx+=R"(()";
        url_rgx+=R"((?:(?:/[\w.]*)+))"; // /path/to/resorce
        url_rgx+=R"((?:\?(?:\w+=\w+(?:;\w+=\w+)*))?)"; //get parameters
        url_rgx+=R"()?)";
        assert(regex_match(url,regex(url_rgx)));
        auto v=findall(url,url_rgx);
        assert(v[0]==url);
        domain=v[1];
        const char *c=v[2].c_str();
        if(v[2]!="") port=strtol(c,NULL,10);
        if(v[3]!="") path=v[3];
        req=http("GET "+path+" HTTP/1.1\r\n\r\n");
    };
    string to_str(){
        stringstream ss;
        ss<<"http://"<<domain;
        if(port!=80) ss<<":"<<port;
        ss<<req.path;
        if(req.get_params.size()){
            ss<<"?";
            int i=0;
            for(auto &[k,v]:req.get_params){
                ss<<k<<"="<<v;
                if(i+1!=(int)req.get_params.size()) ss<<";";
                i++;
            }
        }
        return ss.str();
    }
};



















//

