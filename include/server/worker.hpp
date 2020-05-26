#ifndef WORKER_H
#define WORKER_H

#include <thread>

#include "server/server.hpp"

class Worker {
 public:
  Worker() {}
  void setUp(std::string dir) {
    m_local.setWorkDir(std::move(dir));
  }
  void tearDown() {
    m_local.stop();
    m_th.join();
  }
  void operator() () {
    m_th = std::thread([&]() {
      m_local.bind(5999);
      m_local.listen();
    });
  }

 private:
  Server m_local;
  std::thread m_th;
};

#endif // WORKER_H