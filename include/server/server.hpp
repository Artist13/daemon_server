#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <vector>
#include <thread>

class Server {
 public:
  Server(std::string workDir);
  ~Server();
  void bind(short port);
  void listen();
  void stop();
  bool isValid() const;

 private:
  int m_sockfd;
  bool readyToAccept = true;
  std::vector<std::thread> m_threads;
  std::string m_workDir;
};

#endif // SERVER_H