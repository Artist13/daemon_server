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
#include <fcntl.h>

std::mutex readyMutex;

template <typename T>
T recv(int sockfd) {
  size_t len = sizeof(T);
  size_t readed = 0;
  T result;
  while (readed < len) {
    int ret = recv(sockfd, &result, len - readed, 0);
    if (ret > 0) {
      readed += ret;
    } else {
      // error
      return T();
    }
  }
  return result;
}

template <typename T>
T *recv(int sockfd, size_t size) {
  size_t len = sizeof(T) * size;
  size_t readed = 0;
  T *result = new T[size];
  while (readed < len) {
    int ret = recv(sockfd, result, len - readed, 0);
    if (ret > 0) {
      readed += ret;
    } else {
      // error
      return nullptr;
    }
  }
  return result;
}

int recv_line(int sockfd, std::string& buffer) {
  int length = recv<int>(sockfd);
  if (length != 0) {
    length = ntohl(length);
    if (length == -1) {
      return -1;
    }
    if (length > 500) {
      // split by chunks
    }
    printf("Ready %i bytes\n", length);
    char *request = recv<char>(sockfd, length);
    if (request) {
      buffer = std::string(request, length);
      return length;
    } else {
      return -1;
    }
  } else {
    // This is not error
    buffer = "";
    return 0;
  }
}

void handle_connection(int sockfd, std::string output) {
  int fd, length = 0;
  
  std::ofstream file(output, std::ios::out | std::ios::trunc);
  std::string buffer;
  while (true) {
    std::string buffer;
    auto ret = recv_line(sockfd, buffer);
    if (ret == -1) {
      break;
    } else {
      printf("Recv %s\n", buffer.c_str());
      file << buffer << std::endl;
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

  int flags = fcntl(m_sockfd, F_GETFL, 0);
  flags = flags | O_NONBLOCK;
  fcntl(m_sockfd, F_SETFL, flags);
}

void Server::setWorkDir(std::string workDir) {
  m_workDir = workDir;
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
  bool need_continue = readyToAccept;

  ::listen(m_sockfd, 20);
  // TODO: make it non-blocking
  fd_set master_set, working_set;
  struct timeval timeout;

  FD_ZERO(&master_set);
  FD_SET(m_sockfd, &master_set);
  timeout.tv_sec = 1;
  timeout.tv_usec = 0;
  while (need_continue) {
    readyMutex.lock();
    need_continue = readyToAccept;
    readyMutex.unlock();

    memcpy(&working_set, &master_set, sizeof(working_set));
    sin_size = sizeof(struct sockaddr_in);
    int rc = select(m_sockfd + 1, &working_set, nullptr, nullptr, &timeout);
    if (rc == -1) {
      // error
      return;
    }
    if (rc == 0) {
      // no connections;
      continue;
    }
    if (FD_ISSET(m_sockfd, &working_set)) {
      while (true) {
        int new_sockfd =
            accept(m_sockfd, (struct sockaddr *)&client_addr, &sin_size);
        if (new_sockfd < 0) {
          if (errno == EWOULDBLOCK || errno == EAGAIN) {
            printf("No pending connections; sleeping for one second.\n");
            // sleep(1);
          } else {
            perror("error when accepting connection");
            return;
          }
          break;
        }
        m_threads.push_back(
            std::thread(handle_connection, new_sockfd, m_workDir + "out.txt"));
      }
    }
  }
}

void Server::stop() {
  readyMutex.lock();
  readyToAccept = false;
  readyMutex.unlock();
  
  for (auto &th : m_threads) {
    th.join();
  }
}

Server::~Server() {
  stop();
}








