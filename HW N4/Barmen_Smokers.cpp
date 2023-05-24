#include <iostream>
#include <string>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

enum { SMOKERS_COUNT = 3, TOBACCO = 0, PAPER = 1, MATCH = 2, BARMEN = 3 };

void barmen(const std::string &items, const int &semid) {
  std::cout << "Barmen search item..." << std::endl;
  for (auto it : items) {
    if (it == 't') {
      struct sembuf found_tobacco {
        TOBACCO, 1, 0
      };
      semop(semid, &found_tobacco, 1);
    } else if (it == 'p') {
      struct sembuf found_paper {
        PAPER, 1, 0
      };
      semop(semid, &found_paper, 1);
    } else if (it == 'm') {
      struct sembuf found_match {
        MATCH, 1, 0
      };
      semop(semid, &found_match, 1);
    }
  }
  return;
}

void smoker(const pid_t &pid, const int &semid) {
  int item_for_smoke = pid;
  while (true) {

    struct sembuf wait_for_item {
      BARMEN, -1, 0
    };
    semop(semid, &wait_for_item, 1);

    struct sembuf receive_item {
      static_cast<ushort>(item_for_smoke), -1, 0
    };
    semop(semid, &receive_item, 1);

    std::string used_item;
    if (item_for_smoke == TOBACCO) {
      used_item = "Tobacco";
    } else if (item_for_smoke == PAPER) {
      used_item = "Paper";
    } else {
      used_item = "Mathces";
    }
    std::cout << "The smoker: " << pid << " used the " << used_item
              << std::endl;
    sleep(5);

    struct sembuf barmen_free {
      BARMEN, 1, 0
    };
    semop(semid, &barmen_free, 1);
  }
  return;
}

int main() {
  pid_t smokers[SMOKERS_COUNT];
  int semid = semget(IPC_PRIVATE, 4, IPC_CREAT | 0644);
  if (semid < 0) {
    std::cerr << "Error creating semaphores" << std::endl;
    exit(semid);
  }
  std::string inputs;
  std::cout << "Input the items: ";
  std::cin >> inputs;
  int barmen_pid = fork();
  if (barmen_pid == 0) {
    barmen(inputs, semid);
  } else if (barmen_pid < 0) {
    std::cerr << "Error creating Barmen fork()";
  }
  for (int i = 0; i < SMOKERS_COUNT; ++i) {
    if ((smokers[i] = fork()) == 0) {
      smoker(i, semid);
    } else if (smokers[i] < 0) {
      std::cerr << "Error creating Smoker: " << i << " fork()" << std::endl;
      exit(smokers[i]);
    }
  }
  if (semctl(semid, TOBACCO, SETVAL, 0) < 0) {
    std::cerr << "Tobacco semctl() error" << std::endl;
    exit(-1);
  }
  if (semctl(semid, MATCH, SETVAL, 0) < 0) {
    std::cerr << "Match semctl() error" << std::endl;
    exit(-1);
  }
  if (semctl(semid, PAPER, SETVAL, 0) < 0) {
    std::cerr << "Paper semctl() error" << std::endl;
    exit(-1);
  }
  if (semctl(semid, BARMEN, SETVAL, 1) < 0) {
    std::cout << "Barmen semctl() error" << std::endl;
    exit(-1);
  }

  sleep(inputs.size() * 5);
  std::cout << "The bar is closing!!\n";
  for (int i = 0; i < SMOKERS_COUNT; ++i) {
    kill(smokers[i], SIGTERM);
  }

  for (int i = 0; i < SMOKERS_COUNT + 1; ++i) {
    wait(NULL);
  }

  return 0;
}