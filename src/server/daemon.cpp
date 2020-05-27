#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>
#include <sys/stat.h>
#include <fstream>
#include <string>
#include <sys/resource.h>
#include <wait.h>
#include <string.h>
#include <thread>
#include <functional>

#include "server/worker.hpp"
#include "server/signalhelper.hpp"
#include "server/logger.hpp"

static const std::string PID_FILE = "/var/run/my_daemon.pid";
// daemon exit statuses
const int CHILD_NEED_WORK = 1;
const int CHILD_NEED_TERMINATE = 2;
const int FD_LIMIT = 1024 * 10;

// path to output file
static std::string filepath;

// void writeToLog(std::string msg) {
//   std::ofstream log(pathToLog, std::ios::out | std::ios::app);
//   log << msg << std::endl;
// }

// void writeToLog(const char *msg) {
//   writeToLog(std::string(msg));
// }

void usage() {
  std::cout << "Usage: daemon filename.cfg" << std::endl;
}

void unlink(const std::string& file) {
  unlink(file.c_str());
}

// prepare env after fork
void clearChildEnv() {
  umask(0);
  pid_t sid = setsid();
  if (sid < 0) {
    ELOG("setsid failed");
    exit(EXIT_FAILURE);
  }
  if (chdir("/")) {
    ELOG("chdir failed");
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

bool processDaemonStatus(int status) {
  switch (status) {
    case CHILD_NEED_TERMINATE:
      LOG("[MONITOR] Child stopped");
      return false;
    case CHILD_NEED_WORK:
      LOG("[MONITOR] Child restart");
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

  LOG("[MONITOR] Start...");

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
        ELOG("[MONITOR] fork failed ", strerror(errno));
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
            LOG("[MONITOR] Signal ", strsignal(siginfo.si_signo));
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

  LOG("[MONITOR] Stop");

  unlink(PID_FILE);
  return status;
}

// TODO: move it somewhere
std::vector<Worker *> workers;
int initWorkThread() {
  auto serverWorker = new Worker(filepath);
  (*serverWorker)();
  workers.push_back(serverWorker);
  return 0;
}

void destroyWorkThread() {
  for (auto worker : workers) {
    delete worker;
  }
}

static void signal_error(int sig, siginfo_t* si, void* ptr) {
  void* ErrorAddr;
  void* Trace[16];
  int x;
  int TraceSize;
  char** Messages;

  // запишем в лог что за сигнал пришел
  LOG("[DAEMON] Signal: ", strsignal(sig));

#if __WORDSIZE == 64  // если дело имеем с 64 битной ОС
  // получим адрес инструкции которая вызвала ошибку
  ErrorAddr = (void*)((ucontext_t*)ptr)->uc_mcontext.gregs[REG_RIP];
#else
  // получим адрес инструкции которая вызвала ошибку
  ErrorAddr = (void*)((ucontext_t*)ptr)->uc_mcontext.gregs[REG_EIP];
#endif

  // произведем backtrace чтобы получить весь стек вызовов
  TraceSize = backtrace(Trace, 16);
  Trace[1] = ErrorAddr;

  // получим расшифровку трасировки
  Messages = backtrace_symbols(Trace, TraceSize);
  if (Messages) {
    LOG("== Backtrace ==");

    // запишем в лог
    for (x = 1; x < TraceSize; x++) {
      LOG(Messages[x]);
    }

    LOG("== End Backtrace ==\n");
    free(Messages);
  }

  LOG("[DAEMON] Stopped");

  // остановим все рабочие потоки и корректно закроем всё что надо
  destroyWorkThread();

  // завершим процесс с кодом требующим перезапуска
  exit(CHILD_NEED_WORK);
}

int daemonStart() {
  struct sigaction sigact;
  sigset_t sigset;
  int signo;
  int status;

  prepareSigActions(sigact, {SIGFPE, SIGILL, SIGSEGV, SIGBUS}, signal_error);

  prepareSignals(sigset, {SIGQUIT, SIGHUP, SIGINT, SIGTERM});

  SetFdLimit(FD_LIMIT);

  LOG("[DAEMON] Started");

  status = initWorkThread();
  if (!status) {
    while (true) {
      sigwait(&sigset, &signo);
      break;
    }
    destroyWorkThread();
  } else {
    LOG("[DAEMON] Create work thread failed");
  }
  LOG("[DAEMON] Stopped");
  return CHILD_NEED_TERMINATE;
}
