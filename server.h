#pragma once

#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <map>
#include <sstream>
#include <iostream>
#include <vector>
#include <string>

using namespace std;

class Server {
public:
    Server(int port);
    ~Server();

    void run();
    

private:
    void create();
    void close_socket();
    void serve();
    void handle(int);
    string get_request(int);
    bool send_response(int, string);

    
    map<string, vector<pair<string, string> > > messages;

    int port_;
    int server_;
    int buflen_;
    char* buf_;
};
