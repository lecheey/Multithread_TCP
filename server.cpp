#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
//#include <winsock2.h>
#include<netinet/in.h>

#include <string.h>
#include <thread>
#include <vector>
#include <mutex>

#include <sys/select.h>
#include <set>
#include <algorithm>

#define MESSAGE_LENGTH 1024

int sockd, bind_status, connection_status, connection, activity;
struct sockaddr_in serveraddress, clientaddress;
socklen_t length;

int PORT = 7777;
std::string IP = "127.0.0.1";

thread_local int p = 0;
std::mutex mutex;

class connectionList{
public:
	std::string _username;
	int _num;
	struct sockaddr_in* _s_address;

	connectionList(int num, struct sockaddr_in* s_address){
		_num = num;
		_s_address = s_address;
	}
};

std::vector<connectionList> connections;

int createConnection(){
	sockd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockd < 0){ return -1; } // couldn't create socket 

	serveraddress.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddress.sin_port = htons(PORT); // port for connection
	serveraddress.sin_family = AF_INET;

	bind_status = bind(sockd, (struct sockaddr*)&serveraddress, sizeof(serveraddress));
	if(bind_status < -1){ return -2; } // couldn't bind socket

	connection_status = listen(sockd, 5);
	if(connection_status < 0){ return -3; } // unable to listen
	
	std::cout << "Listening" << std::endl;
	return 1;
}

void listenConnection(struct sockaddr_in* client_addr, int b){
	char response[MESSAGE_LENGTH];
	while(1){
		bzero(response, MESSAGE_LENGTH);
		int a = recv(b, response, sizeof(response), 0);	
		if(a <= 0) { return; }
		
		mutex.lock();
		if (strncmp("end", response, 3) == 0) {
		    std::cout << "Client Exited." << std::endl;
			mutex.unlock();
      	    break;
        }
		std::cout << "client: " <<  response << std::endl;
		mutex.unlock();
	}
}

void acceptConnection(){
	std::vector<std::thread> poolofthread;

	std::set<int> clients; // list of the descriptors in scope
    clients.clear();
	while(1){
		fd_set readset;
    	FD_ZERO(&readset);
    	FD_SET(sockd, &readset);

		for(std::set<int>::iterator it = clients.begin(); it != clients.end(); it++)
            FD_SET(*it, &readset);

		timeval timeout;
    	timeout.tv_sec = 10;
    	timeout.tv_usec = 0;
		
		// listening for the activity from the descriptors
		int mx = std::max(sockd, *std::max_element(clients.begin(), clients.end()));
		activity = select(mx+1, &readset, NULL, NULL, &timeout);
		// if timeout, close connection and wait for undone threads
		if(activity <= 0){
			std::cout << "connection is closed" << std::endl;
			for(auto& t : poolofthread){
				if(t.joinable())
					t.join();
			}
			break;
		}

		// in case of event make new connection and send it into a new thread
		if(FD_ISSET(sockd, &readset)){
			connection = accept(sockd, (struct sockaddr*)&clientaddress, (socklen_t*)&length);
			std::thread t(listenConnection, &clientaddress, connection);
			poolofthread.push_back(std::move(t));
			connectionList new_conn(connection, &clientaddress);
			connections.push_back(new_conn);
			std::cout << "new connection" << std::endl;
		}
	}
/*
	// simple way without stoping threads before closing the programm
	std::vector<std::thread> poolofthread;
	while((connection = accept(sockd, (struct sockaddr*)&clientaddress, (socklen_t*)&length)) > 0){
		std::thread t(listenConnection, &clientaddress, connection);
		poolofthread.push_back(std::move(t));
		connectionList new_conn(connection, &clientaddress);
		connections.push_back(new_conn);
		std::cout << "new connection" << std::endl;
	}
	std::cout << "connection is closed" << std::endl;
	for(auto& t : poolofthread){
		if(t.joinable())
			t.join();
	}
*/
}

void sendMessage(struct sockaddr_in* client_addr, int b){
	char message[MESSAGE_LENGTH];
	std::cout << "server: ";
	bzero(message, MESSAGE_LENGTH);
	std::cin >> message;
	send(b, message, sizeof(message), 0);
	std::cout << "msg is sent" << std::endl;
}

void chooseReceiver(){
	while(1){
		std::cout << "choose the user: ";
		int c; // particular descriptor
		std::cin >> c;
		
		if(c == 0){ break; }

		for(int i = 0; i < connections.size(); i ++){
			if(c == connections[i]._num){
				sendMessage(connections[i]._s_address, connections[i]._num);
				break;
			}
		}
	}
}

int main(){	
	int a = createConnection();
	std::thread t(acceptConnection);
	t.detach();

	chooseReceiver();

	close(sockd);
	return 0;
}
