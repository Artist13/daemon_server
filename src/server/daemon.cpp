#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <sys/stat.h>
#include <signal.h>
#include <fstream>
#include <string>
#include <sys/resource.h>
#include <wait.h>
#include <string.h>
#include <thread>
#include <vector>

#include "server/worker.hpp"

static const std::string PID_FILE = "/var/run/my_daemon.pid";
// daemon exit statuses
const int CHILD_NEED_WORK = 1;
const int CHILD_NEED_TERMINATE = 2;
const int FD_LIMIT = 1024 * 10;

static const std::string pathToLog = "/var/log/my_daemon.log";
// path to output file
static std::string filepath;
// thread with server 
static std::thread th;

void writeToLog(std::string msg) {
  std::ofstream log(pathToLog, std::ios::out | std::ios::app);
  log << msg << std::endl;
}

void usage() {
  std::cout << "Usage: daemon filename.cfg" << std::endl;
}

void unlink(const std::string& file) {
  unlink(file.c_str());
}

// helper func to prepare sigset
void prepareSignals(sigset_t& sigset, const std::vector<int>& signals) {
  sigemptyset(&sigset);

  for (auto signal : signals) {
    sigaddset(&sigset, signal);
  }

  sigprocmask(SIG_BLOCK, &sigset, NULL);
}

// prepare env after fork
void clearChildEnv() {
  umask(0);
  pid_t sid = setsid();
  if (sid < 0) {
    writeToLog("setsid failed");
    exit(EXIT_FAILURE);
  }
  if (chdir("/")) {
    writeToLog("chdir failed");
    exit(EXIT_FAILURE);
  }
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);
}
// daemon control process code
int monitorStart();
// daemon main process code
int daemonStart();

int loadConfig(char *filename) {
  std::ifstream cfg(filename, std::ios::in);
  // read from config only dir where would place output
  cfg >> filepath;
  return 0;
}

void setPidFile(const std::string& filename) {
  std::fstream pid_file(filename, std::ios::out | std::fstream::trunc);
  pid_file << getpid() << std::endl;
}

int SetFdLimit(int maxFd) {
  struct rlimit lim;

  lim.rlim_cur = maxFd;
  lim.rlim_max = maxFd;

  return setrlimit(RLIMIT_NOFILE, &lim);
}

int main(int argc, char **argv) {
  pid_t pid;

  if (argc != 2) {
    usage();
    return -1;
  }

  if (loadConfig(argv[1]) == -1) {
    std::cerr << "Load config failed." << std::endl;
    return -1;
  }

  switch(pid = fork()) {
    case -1:
      // Error
      std::cerr << "Start daemon failed." << std::strerror(errno) << std::endl;
      return -1;
    case 0:
      // Child code
      clearChildEnv();
      return monitorStart();
    default:
      // Parent code
      return 0;
  }
}

void monitorForkError() {
  writeToLog("[MONITOR] fork failed" + std::string(strerror(errno)));
}

bool processDaemonStatus(int status) {
  switch (status) {
    case CHILD_NEED_TERMINATE:
      writeToLog("[MONITOR] Child stopped");
      return false;
    case CHILD_NEED_WORK:
      writeToLog("[MONITOR] Child restart");
      return true;
    default:
      return true;
  }
}

int monitorStart() {
  pid_t pid;
  int status;
  bool need_start = true;
  sigset_t sigset;
  siginfo_t siginfo;

  writeToLog("[MONITOR] Start...");

  prepareSignals(sigset, {SIGTERM, SIGHUP, SIGINT, SIGQUIT, SIGCHLD});

  setPidFile(PID_FILE);

  bool need_continue = true;
  while(need_continue) {
    if (need_start) {
      pid = fork();
    }

    need_start = true;

    switch(pid) {
      case -1:
        // Error
        monitorForkError();
        break;
      case 0:
        // Child code
        return daemonStart();
      default: {
        // Parent code
        sigwaitinfo(&sigset, &siginfo);
        switch (siginfo.si_signo) {
          case SIGCHLD:
            wait(&status);
            status = WEXITSTATUS(status);
            need_continue = processDaemonStatus(status);
            break;
          case SIGTERM:
          case SIGHUP:
          case SIGINT:
          case SIGQUIT:
            writeToLog("[MONITOR] Signal " +
                       std::string(strsignal(siginfo.si_signo)));
            kill(pid, SIGTERM);
            status = 0;
            need_continue = false;
            break;
          default:
            break;
        }
      }
    }
  }

  writeToLog("[MONITOR] Stop");

  unlink(PID_FILE);
  return status;
}

// TODO: move it somewhere
std::vector<Worker> workers;
Worker serverWorker;
Server local(filepath);
int initWorkThread() {
  serverWorker.setUp(filepath);
  serverWorker();
  // workers.push_back(std::move(serverWorker));
  return 0;
}

void destroyWorkThread() {
  // for (auto& worker : workers) {
  //   worker.tearDown();
  // }
  serverWorker.tearDown();
}

int daemonStart() {
  // struct sigaction sigact;
  sigset_t sigset;
  int signo;
  int status;

  prepareSignals(sigset, {SIGQUIT, SIGINT, SIGTERM});

  SetFdLimit(FD_LIMIT);

  writeToLog("[DAEMON] Started");

  status = initWorkThread();
  if (!status) {
    while (true) {
      sigwait(&sigset, &signo);
      break;
    }
    destroyWorkThread();
  } else {
    writeToLog("[DAEMON] Create work thread failed");
  }
  writeToLog("[DAEMON] Stopped");
  return CHILD_NEED_TERMINATE;
}
