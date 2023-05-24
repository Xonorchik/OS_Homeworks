#include <csignal>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

enum { GEN_NUM_COUNT = 100 };

int shmid;
int semid;
int (*arr)[GEN_NUM_COUNT];

void releaseResources() {
  shmdt(reinterpret_cast<void *>(arr));
  shmctl(shmid, IPC_RMID, nullptr);
  semctl(semid, 0, IPC_RMID);
}

void signalHandler(int signum) {
  if (signum == SIGINT) {
    std::cout << "Received SIGINT signal. Exiting..." << std::endl;
    releaseResources();
    exit(0);
  }
}

int main() {
  key_t key = ftok("file_for_shmem", IPC_CREAT | 0666);
  if (key == -1) {
    std::cerr << "Error creating key with function ftok()" << std::endl;
    exit(key);
  }
  shmid = shmget(key, sizeof(int) * GEN_NUM_COUNT, IPC_CREAT | 0666);
  if (shmid == -1) {
    std::cerr << "Error creating shared memory with function shmget()"
              << std::endl;
    exit(shmid);
  }
  arr = reinterpret_cast<int(*)[GEN_NUM_COUNT]>(shmat(shmid, nullptr, 0));
  if (arr == reinterpret_cast<int(*)[GEN_NUM_COUNT]>(-1)) {
    std::cerr << "Error attaching shared memory with function shmat()"
              << std::endl;
    exit(-1);
  }
  key_t sem_key = ftok("file_for_semaphore", 1);
  if (sem_key == -1) {
    std::cerr << "Error creating sem_key with function ftok()" << std::endl;
    exit(sem_key);
  }
  semid = semget(sem_key, 3, IPC_CREAT | 0666);
  if (semid == -1) {
    std::cerr << "Error creating semaphore set with function semget()"
              << std::endl;
    exit(semid);
  }
  pid_t producer;
  pid_t consumer;
  union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
  };

  semun arg;

  arg.val = GEN_NUM_COUNT;
  if (semctl(semid, 0, SETVAL, arg) == -1) {
    std::cerr << "Error initializing semaphore 'empty'" << std::endl;
    exit(1);
  }

  arg.val = 0;
  if (semctl(semid, 1, SETVAL, arg) == -1) {
    std::cerr << "Error initializing semaphore 'full'" << std::endl;
    exit(1);
  }

  arg.val = 1;
  if (semctl(semid, 2, SETVAL, arg) == -1) {
    std::cerr << "Error initializing semaphore 'mutex'" << std::endl;
    exit(1);
  }
  signal(SIGINT, signalHandler);
  int producedNum;
  srand(time(NULL));
  if ((producer = fork()) == 0) {
    for (int i = 0; i < GEN_NUM_COUNT; ++i) {
      struct sembuf op[2];
      op[0].sem_num = 0;
      op[0].sem_op = -1;
      op[0].sem_flg = 0;
      op[1].sem_num = 2;
      op[1].sem_op = -1;
      op[1].sem_flg = 0;
      semop(semid, op, 2);

      producedNum = rand() % 1000;
      (*arr)[i] = producedNum;

      op[0].sem_num = 2;
      op[0].sem_op = 1;
      op[0].sem_flg = 0;
      op[1].sem_num = 1;
      op[1].sem_op = 1;
      op[1].sem_flg = 0;
      semop(semid, op, 2);
    }
    exit(0);
  } else if (producer == -1) {
    std::cerr << "Error creating producer fork()" << std::endl;
  }

  int consumedNum;
  if ((consumer = fork()) == 0) {
    for (int i = 0; i < GEN_NUM_COUNT; ++i) {
      struct sembuf op[2];
      op[0].sem_num = 1;
      op[0].sem_op = -1;
      op[0].sem_flg = 0;
      op[1].sem_num = 2;
      op[1].sem_op = -1;
      op[1].sem_flg = 0;
      semop(semid, op, 2);

      consumedNum = (*arr)[i];
      std::cout << i+1 << ". The consumed Number is: " << consumedNum << std::endl;

      op[0].sem_num = 2;
      op[0].sem_op = 1;
      op[0].sem_flg = 0;
      op[1].sem_num = 0;
      op[1].sem_op = 1;
      op[1].sem_flg = 0;
      semop(semid, op, 2);
    }
    exit(0);
  } else if (consumer == -1) {
    std::cerr << "Error creating consumer fork()" << std::endl;
  }

  if (producer > 0) {
    wait(NULL);
  }
  if (consumer > 0) {
    wait(NULL);
  }

  releaseResources();
  return 0;
}
