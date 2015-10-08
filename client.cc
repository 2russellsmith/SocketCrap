#include "client.h"


Client::Client(string host, int port) {
    // setup variables
    host_ = host;
    port_ = port;
    buflen_ = 1024;
    buf_ = new char[buflen_+1];
}

Client::~Client() {
}

void Client::run() {
    create();
    process();
}

void
Client::create() {
    struct sockaddr_in server_addr;

    // use DNS to get IP address
    struct hostent *hostEntry;
    hostEntry = gethostbyname(host_.c_str());
    if (!hostEntry) {
        cout << "No such host name: " << host_ << endl;
        exit(-1);
    }

    // setup socket address structure
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_);
    memcpy(&server_addr.sin_addr, hostEntry->h_addr_list[0], hostEntry->h_length);

    // create socket
    server_ = socket(PF_INET,SOCK_STREAM,0);
    if (!server_) {
        perror("socket");
        exit(-1);
    }

    // connect to server
    if (connect(server_,(const struct sockaddr *)&server_addr,sizeof(server_addr)) < 0) {
        perror("connect");
        exit(-1);
    }
}

void
Client::close_socket() {
    close(server_);
}


void
Client::process() {
    string line;
    cout << "% ";
    // loop to handle user interface
    while (getline(cin,line)) {
        // append a newline

		istringstream ss(line);
		string token;
        string message;
		vector<string> v;

		while(getline(ss, token, ' ')) {
	    	v.push_back(token);
		}
        if(v.size() != 0){
            if(v.at(0) == "send" && v.size() == 3){
                cout << "- Type your message. End with a blank line -" << endl;
                string temp;
                temp = "blank";
                while(temp != ""){
                    temp = "";
                    getline(cin, temp);
                    if(temp != ""){
                        message += temp + '\n';
                    } 
                }
                string length = static_cast<ostringstream*>( &(ostringstream() << (message.length())) )->str();;
                message = "put " + v.at(1) + " " + v.at(2) + ' ' + length + '\n' + message;
            }else if(v.at(0) == "quit" && v.size() == 1){
                break;
            }else if(v.at(0) == "read" && v.size() == 3){
                message = "get " + v.at(1) + " " + v.at(2) + '\n';
            }else if(v.at(0) == "list" && v.size() == 2){
                message = "list " + v.at(1) + '\n';
            }else{
                message = line;
            //     message = "error";
            //     cout << "Invalid Input. Please Try Again. Check number of inputs and spelling.\n";
            }
            // if(message != "error"){
                // send request
                bool success = send_request(message);

                // break if an error occurred
                if (not success)
                    break;
                // get a response
                success = get_response();
                // break if an error occurred
                if (not success)
                    break;
            // }
            }

        cout << "% ";
    }
    close_socket();
}

bool
Client::send_request(string request) {
    // prepare to send request
    const char* ptr = request.c_str();
    int nleft = request.length();
    int nwritten;
    // loop to be sure it is all sent
    while (nleft) {
        if ((nwritten = send(server_, ptr, nleft, 0)) < 0) {
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

bool
Client::get_response() {
    string response = "";
    buf_ = new char[buflen_+1];
    // read until we get a newline

    while (response.find("\n") == string::npos) {
        memset(buf_, 0, buflen_);
        int nread = recv(server_,buf_,buflen_,0);
        if (nread < 0) {
            if (errno == EINTR)
                // the socket call was interrupted -- try again
                continue;
            else
                // an error occurred, so break out
                cout << "ERROR" << endl;
                return "";
        } else if (nread == 0) {
            // the socket is closed
                cout << "ERROR" << endl;
            return "";
        }
        // be sure to use append in case we have binary data
        response.append(buf_,nread);
    }
    istringstream ss(response);
    string token;
    string message;
    vector<string> v;

    while(getline(ss, token, '\n')) {
        v.push_back(token);
    }
    if(v.at(0).find("list") != std::string::npos){
        response = "";
        for(int i = 1; i < v.size(); i++){
            response += v.at(i) + '\n';
        }
        cout << response;
    }else if(v.at(0).find("message") != std::string::npos){
        istringstream ss1(v.at(0));
        string token1;
        vector<string> v1;
        response = "";
        while(getline(ss1, token1, ' ')) {
            v1.push_back(token1);
        }
        for(int i = 1; i < v1.size() - 1; i++){
          response += v1.at(i);
        }
        response += '\n';
        for(int i = 1; i < v.size() - 1; i++){
          response += v.at(i) + '\n';
        }
        cout << response;
    }else if(v.at(0).find("error") != std::string::npos){
        cout << response;
    }
    // cout << response;
    return true;
}
