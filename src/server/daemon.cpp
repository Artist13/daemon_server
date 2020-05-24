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

static const std::string PID_FILE = "/var/run/my_daemon.pid";
const int CHILD_NEED_WORK = 1;
const int CHILD_NEED_TERMINATE = 2;
const int FD_LIMIT = 1024 * 10;

static const std::string pathToLog = "/var/log/my_daemon.log";

void writeToLog(std::string msg) {
  std::ofstream log(pathToLog, std::ios::out | std::ios::app);
  log << msg << std::endl;
}

void usage() {
  // TODO: need i this cfg?
  std::cout << "Usage: daemon filename.cfg" << std::endl;
}

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
int headWork();
// daemon main process code
int mainWork();

// mock for config loading
int loadConfig(char *filename) {
  return 0;
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
      std::cerr << "Start daemon failed." << std::strerror(errno) << std::endl;
      return -1;
    case 0:
      // Child code
      clearChildEnv();
      return headWork();
    default:
      // Parent code
      return 0;
  }
}

void setPidFile(const std::string& filename) {
  std::fstream pid_file(filename, std::ios::out | std::fstream::trunc);
  pid_file << getpid() << std::endl;
}

int headWork() {
  pid_t pid;
  int status;
  bool need_start = true;
  sigset_t sigset;
  siginfo_t siginfo;

  writeToLog("[MONITOR] Start...");

  sigemptyset(&sigset);

  sigaddset(&sigset, SIGTERM);
  sigaddset(&sigset, SIGHUP);
  sigaddset(&sigset, SIGINT);
  sigaddset(&sigset, SIGQUIT);
  sigaddset(&sigset, SIGCHLD);

  sigprocmask(SIG_BLOCK, &sigset, NULL);

  setPidFile(PID_FILE);

  bool need_continue = true;
  while(need_continue) {
    if (need_start) {
      pid = fork();
    }

    need_start = true;

    switch(pid) {
      case -1:
        writeToLog("[MONITOR] fork failed" + std::string(strerror(errno)));
        break;
      case 0:
        return mainWork();
      default:
        sigwaitinfo(&sigset, &siginfo);
        switch (siginfo.si_signo) {
          case SIGCHLD:
            wait(&status);
            status = WEXITSTATUS(status);
            switch (status) {
              case CHILD_NEED_TERMINATE:
                writeToLog("[MONITOR] Child stopped");
                need_continue = false;
                break;
              case CHILD_NEED_WORK:
                writeToLog("[MONITOR] Child restart");
                break;
            }
            break;
          case SIGTERM:
          case SIGHUP:
          case SIGINT:
          case SIGQUIT:
            writeToLog("[MONITOR] Signal " + std::string(strsignal(siginfo.si_signo)));
            kill(pid, SIGTERM);
            status = 0;
            need_continue = false;
            break;
          default:
            break;
        }
    }
  }

  writeToLog("[MONITOR] Stop");

  unlink(PID_FILE.c_str());
  return status;
}

int SetFdLimit(int maxFd) {
  struct rlimit lim;

  lim.rlim_cur = maxFd;
  lim.rlim_max = maxFd;

  return setrlimit(RLIMIT_NOFILE, &lim);
}

int initWorkThread() {
  return 0;
}

void destroyWorkThread() {

}

int mainWork() {
  // struct sigaction sigact;
  sigset_t sigset;
  int signo;
  int status;

  sigemptyset(&sigset);
  sigaddset(&sigset, SIGQUIT);
  sigaddset(&sigset, SIGINT);
  sigaddset(&sigset, SIGTERM);

  sigprocmask(SIG_BLOCK, &sigset, NULL);

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
