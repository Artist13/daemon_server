#include "server/server.hpp"

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
#include <mutex>

std::mutex readyMutex;

std::string recv_line(int sockfd) {
  char request[500];
  int length;
  std::string buffer;
  int ret = recv(sockfd, &length, sizeof(length), 0);
  if (ret > 0) {
    length = ntohl(length);
    if (length == -1) {
      return "";
    }
    if (length > 500) {
      // split by chunks
    }
    printf("Ready %i bytes\n", length);
    ret = recv(sockfd, request, length, 0);
    request[length] = '\0';
  } else {
    return "";
  }
  return std::string(request);
}

void handle_connection(int sockfd, std::string output) {
  int fd, length = 0;
  
  std::ofstream file(output, std::ios::out | std::ios::trunc);
  std::string buffer;
  while (true) {
    std::string buffer = recv_line(sockfd);
    if (buffer.length() > 0) {
      printf("Recv %s\n", buffer.c_str());
      file << buffer << std::endl;
    } else {
      break;
    }
  }

  file.close();
  close(sockfd);
}

Server::Server(std::string workDir)
    :m_workDir(workDir) {
  int yes = 1;

  m_sockfd = socket(PF_INET, SOCK_STREAM, 0);

  setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
}

bool Server::isValid() const {
  return !(m_sockfd == -1);
}

void Server::bind(short port) {
  struct sockaddr_in host_addr;

  host_addr.sin_family = AF_INET;
  host_addr.sin_port = htons(port);
  host_addr.sin_addr.s_addr = INADDR_ANY;

  ::bind(m_sockfd, (struct sockaddr *)&host_addr, sizeof(struct sockaddr));
}

void Server::listen() {
  socklen_t sin_size;
  struct sockaddr_in client_addr;
  int new_sockfd;
  bool need_continue = false;

  ::listen(m_sockfd, 20);

  auto loop = [&]() {
    while (readyToAccept) {
      sin_size = sizeof(struct sockaddr_in);
      int new_sockfd =
          accept(m_sockfd, (struct sockaddr *)&client_addr, &sin_size);
      if (new_sockfd == -1) {
        printf("Error in accept\n");
        return;
      }

      std::thread th(handle_connection, new_sockfd, m_workDir + "out.txt");
      th.detach();
      m_threads.push_back(std::move(th));
      readyMutex.lock();
      need_continue = readyToAccept;
      readyMutex.unlock();
      sleep(1);
    }
  };
  readyMutex.lock();
  need_continue = readyToAccept;
  readyMutex.unlock();

  m_threads.push_back(std::thread(loop));
}

void Server::stop() {
  std::lock_guard<std::mutex> guard(readyMutex);
  readyToAccept = false;
}

Server::~Server() {
  stop();
  for (auto &th : m_threads) {
    th.join();
  }
}








