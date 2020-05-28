#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <vector>
#include <thread>

#include "server/logger.hpp"

class Server {
 public:
  Server(std::string workDir = "/");
  ~Server();
  void setWorkDir(std::string workDir);
  void bind(short port);
  void listen();
  void stop();
  bool isValid() const;

 private:
  int m_sockfd;
  bool readyToAccept = true;
  std::vector<std::thread> m_threads;
  std::thread m_main;
  std::string m_workDir;
};

#endif // SERVER_H