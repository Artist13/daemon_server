#ifndef SIGNAL_HELPER_H
#define SIGNAL_HELPER_H

#include <execinfo.h>
#include <cerrno>
#include <signal.h>
#include <vector>

// helper func to prepare sigset
void prepareSignals(sigset_t& sigset, const std::vector<int>& signals) {
  sigemptyset(&sigset);

  for (auto signal : signals) {
    sigaddset(&sigset, signal);
  }

  sigprocmask(SIG_BLOCK, &sigset, NULL);
}

void prepareSigActions(struct sigaction& sigact, const std::vector<int>& signals, void (*handler)(int sig, siginfo_t* si, void* ptr)) {
  // сигналы об ошибках в программе будут обрататывать более тщательно
  // указываем что хотим получать расширенную информацию об ошибках
  sigact.sa_flags = SA_SIGINFO;
  // задаем функцию обработчик сигналов
  sigact.sa_sigaction = handler;

  sigemptyset(&sigact.sa_mask);
  // установим наш обработчик на сигналы
  for (auto sig : signals) {
    sigaction(sig, &sigact, 0);  
  }
}

#endif // SIGNAL_HELPER_H