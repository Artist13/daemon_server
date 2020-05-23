#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string>
#include <fstream>
#include <unistd.h>

const std::string HOST_IP = "127.0.0.1";
const short PORT = 5999;

int main(int argc, char **argv) {
  int sockfd;
  struct sockaddr_in server;

  if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
    printf("Error in socket create\n");
    return -1;
  }

  server.sin_family = AF_INET;
  server.sin_port = htons(PORT);
  if(inet_pton(AF_INET, HOST_IP.c_str(), &server.sin_addr) == -1) {
    printf("Error in address\n");
    return -1;
  }

  if (connect(sockfd, (struct sockaddr*)&server, sizeof(server)) == -1) {
    printf("Error in connect\n");
    return -1;
  }

  std::ifstream file("test.txt", std::ios::in);
  std::string buffer;
  if (file.is_open()) {
    while (getline(file, buffer)) {
      printf("%s\n", buffer.c_str());
      int length = buffer.length();
      length = htonl(length);
      send(sockfd, &length, sizeof(length), 0);
      send(sockfd, buffer.c_str(), buffer.length(), 0);
    }
  }
  int fend = htonl(-1);
  send(sockfd, &fend, sizeof(fend), 0);
  
  // Read file and send
    // Send meta data
    // Send data
  // Send file sended succesfully
  file.close();
  close(sockfd);
  return 0;
}
