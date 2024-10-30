#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
//#include <winsock2.h>
#include <arpa/inet.h>
#include <string.h>
#include <thread>
#include <mutex>

#define MESSAGE_LENGTH 1024

int sockd, bind_status, connection_status, connection;
struct sockaddr_in serveraddress, clientaddress;
socklen_t length;

int PORT = 7777;
std::string IP = "127.0.0.1";

//thread_local int p = 0;
std::mutex mutex;

int createConnection(){
	sockd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockd < 0){ return -1; } // couldn't create socket 

	serveraddress.sin_addr.s_addr = inet_addr(IP.data());
	serveraddress.sin_port = htons(PORT); // port for connection
	serveraddress.sin_family = AF_INET;

	connection = connect(sockd, (struct sockaddr*)&serveraddress, sizeof(serveraddress));
	if(connection < 0){ return -4; } // couldn't connect

	std::cout << "Connected" << std::endl;
	return 1;
}

void listenConnection(struct sockaddr_in* serv_addr, int b){
	char response[MESSAGE_LENGTH];
	while(1){
		bzero(response, MESSAGE_LENGTH);
		int a = recv(b, response, sizeof(response), 0);
		
		mutex.lock();
		if(a <= 0) {
		    std::cout << "Connection is closed" << std::endl;
			mutex.unlock();
			break;
		}
		if (strncmp("end", response, 3) == 0) {
		    std::cout << "Client Exited." << std::endl;
			mutex.unlock();
      	    break;
        }
		std::cout << "server: " <<  response << std::endl;
		mutex.unlock();
	}
}

int main(){
	if(createConnection() > 0){
		std::thread t(listenConnection, &serveraddress, sockd);
		t.detach();
		char message[MESSAGE_LENGTH];
		while(1){
			bzero(message, MESSAGE_LENGTH);
			std::cout << "client: ";
			std::cin >> message;
			send(sockd, message, sizeof(message), 0);
			
			if ((strncmp(message, "end", 3)) == 0) {
        	    std::cout << "Client Exit." << std::endl;
        	    break;
        	}
		}	
		close(sockd);
	}
	return 0;
}
