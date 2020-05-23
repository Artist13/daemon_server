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

const short port = 5999;

void handle_connection(int, struct sockaddr_in *);

int main(int argc, char **argv) {
  int sockfd;
  struct sockaddr_in host_addr, client_addr;
  socklen_t sin_size;
  int yes = 1;

  if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
    printf("Error in socket create\n");
    return -1;
  }
  printf("Socket created\n");

  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
    printf("Error in setsockopt\n");
    return -1;
  }
  printf("Setsock setuped\n");

  host_addr.sin_family = AF_INET;
  host_addr.sin_port = htons(port);
  host_addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(sockfd, (struct sockaddr *)&host_addr, sizeof(struct sockaddr)) == -1) {
    printf("Error in bind\n");
    return -1;
  }
  printf("Socket binded\n");
  
  if (listen(sockfd, 20) == -1) {
    printf("Error in listen\n");
    return -1;
  }

  printf("Wait connections...");
  while (true) {
    sin_size = sizeof(struct sockaddr_in);
    int new_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size);
    if (new_sockfd == -1) {
      printf("Error in accept\n");
      return -1;
    }

    handle_connection(new_sockfd, &client_addr);
  }
  return 0;
}

void handle_connection(int sockfd, struct sockaddr_in *client_addr_ptr) {
  char *ptr, request[500], resource[500];
  int fd, length = 0;
  
  std::ofstream file("out.txt", std::ios::out);
  std::string buffer;
  if (file.is_open()) {

    while (true) {
      int ret = recv(sockfd, &length, sizeof(length), 0);
      if (ret != -1) {
        length = ntohl(length);
        if (length == -1) {
          break;
        }
        if (length > 500) {
          // split by chunks
        }
        printf("Ready %i bytes\n", length);
        ret = recv(sockfd, request, length, 0);
        request[length] = '\0';
        printf("Recv %s\n", request);
        file.write(request, length);
      }
    }

  }
  file.close();
  close(sockfd);
}




