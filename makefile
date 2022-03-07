all: c_server
args=-std=c++2a -g -Wall -Wextra 
#-fsanitize=undefined -fsanitize=address 

c_server: main.cpp http.h server.h makefile
	g++-11 $(args) main.cpp -o c_server


run: c_server
	zsh -c 'pkill c_server;echo _' 
	./c_server

fs: c_server
	zsh -c 'pkill c_server;echo _' 
	./c_server -fs

debug: c_server
	lldb c_server

