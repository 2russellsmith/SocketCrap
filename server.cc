#include "server.h"

Server::Server(int port) {
    // setup variables
    port_ = port;
    buflen_ = 1024;
    buf_ = new char[buflen_+1];
    std::map<string,string> messages;
}

Server::~Server() {
    delete buf_;
}

void
Server::run() {
    // create and run the server
    create();
    serve();
}

void
Server::create() {
    struct sockaddr_in server_addr;

    // setup socket address structure
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // create socket
    server_ = socket(PF_INET,SOCK_STREAM,0);
    if (!server_) {
        perror("socket");
        exit(-1);
    }

    // set socket to immediately reuse port when the application closes
    int reuse = 1;
    if (setsockopt(server_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("setsockopt");
        exit(-1);
    }

    // call bind to associate the socket with our local address and
    // port
    if (bind(server_,(const struct sockaddr *)&server_addr,sizeof(server_addr)) < 0) {
        perror("bind");
        exit(-1);
    }

    // convert the socket to listen for incoming connections
    if (listen(server_,SOMAXCONN) < 0) {
        perror("listen");
        exit(-1);
    }
}

void
Server::close_socket() {
    close(server_);
}

void
Server::serve() {
    // setup client
    int client;
    struct sockaddr_in client_addr;
    socklen_t clientlen = sizeof(client_addr);

      // accept clients
    while ((client = accept(server_,(struct sockaddr *)&client_addr,&clientlen)) > 0) {

        handle(client);
    }
    close_socket();
}

void
Server::handle(int client) {
    // loop to handle all requests
    while (1) {
        // get a request
        string request = get_request(client);
        string response;
        cout << request << endl;
        istringstream ss(request);
        string token;
        vector<string> v;
        while(getline(ss, token, ' ')) {
            v.push_back(token);
        }
        // break if client is done or an error occurred
        if (request.empty()){
            break;
        }
        // send response
        if(v.at(0) == "put"){
            istringstream ss1(request);
            string token1;
            vector<string> v1;
            while(getline(ss1, token1, '\n')) {
                v1.push_back(token1);
            }
            istringstream ss2(v1.at(0));
            string token2;
            vector<string> v2;
            while(getline(ss2, token2, ' ')) {
                v2.push_back(token2);
            }
            if(v2.size() == 4){
                string name = v2.at(1) + '\n';
                string subject = v2.at(2);
                string length = v2.at(3);
                string message;
                for(int i = 1; i < v1.size(); i++){
                    message += v1.at(i) + '\n';
                }
                messages[name].push_back(std::pair<string, string> (subject, length + "\n" + message));
                cout << name << ": " << messages[name].size() << endl;
                response = "OK\n";
            }else{
                response = "error Bad Request\n";
            } 

        }else if(v.at(0) == "list" && v.size() == 2){
            string name = v.at(1);
            if(messages.find(name) == messages.end()){
                response = "list 0\n"; 
            }else{
                response = "list " + static_cast<ostringstream*>( &(ostringstream() << (messages[name].size())) )->str() + '\n';
            }
            for(int i = 0; i < messages[name].size(); i++){
                response += static_cast<ostringstream*>( &(ostringstream() << (i + 1)) )->str() + " " + messages[name].at(i).first + '\n';
            }
        }else if(v.at(0) == "get" ){
            string name = v.at(1) + '\n';
            int index = atoi(v.at(2).c_str()) - 1;
            response = "message " + messages[name].at(index).first + " " + messages[name].at(index).second;
        }else if(request == "reset\n"){
            messages.clear();
            response = "OK\n";
        }else{
            response = "error Bad Request\n";
        }
        cout << "Response: " << response << endl;
        bool success = send_response(client,response);
        // break if an error occurred
        if (not success)
            break;
    }
    close(client);
}

string
Server::get_request(int client) {
    string request = "";
    // read until we get a newline
    while (request.find("\n") == string::npos) {
        int nread = recv(client,buf_,1024,0);
        if (nread < 0) {
            if (errno == EINTR)
                // the socket call was interrupted -- try again
                continue;
            else
                // an error occurred, so break out
                return "";
        } else if (nread == 0) {
            // the socket is closed
            return "";
        }
        // be sure to use append in case we have binary data
        request.append(buf_,nread);
    }
    // a better server would cut off anything after the newline and
    // save it in a cache
    return request;
}

bool
Server::send_response(int client, string response) {
    // prepare to send response
    const char* ptr = response.c_str();
    int nleft = response.length();
    int nwritten;
    // loop to be sure it is all sent
    while (nleft) {
        if ((nwritten = send(client, ptr, nleft, 0)) < 0) {
            if (errno == EINTR) {
                // the socket call was interrupted -- try again
                continue;
            } else {
                // an error occurred, so break out
                perror("write");
                return false;
            }
        } else if (nwritten == 0) {
            // the socket is closed
            return false;
        }
        nleft -= nwritten;
        ptr += nwritten;
    }
    return true;
}
