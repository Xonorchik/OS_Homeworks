#include <cstdlib>
#include <iostream>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

enum { 
  INCREASE_COUNT = 10000,
  NUM_CHILDREN = 2
};

void waitSemaphore(int semid) {  
  struct sembuf sb;
  sb.sem_num = 0;
  sb.sem_op = -1;
  sb.sem_flg = SEM_UNDO;

  semop(semid, &sb, 1);
}

void signalSemaphore(int semid) {
  struct sembuf sb;
  sb.sem_num = 0;
  sb.sem_op = 1;
  sb.sem_flg = SEM_UNDO;

  semop(semid, &sb, 1);
}

void waitingChildren() {
  int status;
  pid_t child;
  while ((child = wait(&status)) != -1) {
    /*if (WIFEXITED(status)) {
      int exitStatus = WEXITSTATUS(status);
      std::cout << "Child process " << child << " exited with code: " << exitStatus << std::endl;
    }*/ 
    // i had problem with code and writed this for debug
    // if you open comment this can called for every exit code
    if (WIFSIGNALED(status)) {
      int termSignal = WTERMSIG(status);
      std::cout << "Child process " << child << " signaled with code: " << termSignal << std::endl;
    }
  }
  if (errno != ECHILD) {
    std::cerr << "Error calling wait()" << std::endl;
    exit(1);
  }
}

int main() {
  key_t sem_key = ftok("file_for_sem", 1);
  if (sem_key == -1) {
    std::cerr << "Error creating sem_key with function ftok()" << std::endl;
    exit(sem_key);
  }
  int semid = semget(sem_key, 1, IPC_CREAT | 0666);
  if (semid == -1) {
    std::cerr << "Error creating shared memory with function semget()" << std::endl;
    exit(semid);
  }
  semctl(semid, 0, SETVAL, 1);
  key_t key = ftok("file_for_shmem", SHM_RND);
  if (key == -1) {
    std::cerr << "Error creating key with function ftok()" << std::endl;
    exit(key);
  }
  int shmid = shmget(key, sizeof(int), IPC_CREAT | 0666);
  if (shmid == -1) {
    std::cerr << "Error creating shared memory with function shmget()" << std::endl;
    exit(shmid);
  }
  int* increaseNum = (int*) shmat(shmid, nullptr, SHM_RND);
  if (increaseNum == (int*)-1) {
    std::cerr << "Error getting shared memory with shmat() function" << std::endl;
    exit(1);
  }
  *increaseNum = 0;
  for(int i = 0; i < NUM_CHILDREN; ++i) {
    if (fork() == 0) {
      for (int j = 0; j < INCREASE_COUNT; ++j) {
        waitSemaphore(semid);
        (*increaseNum)++;
        signalSemaphore(semid);
      }
      return 0;
    }
  }
  waitingChildren();
  std::cout << *increaseNum << std::endl;
  shmdt(increaseNum);
  semctl(semid, 0, IPC_RMID);
  return 0;
}
