#include <iostream>
#include <pthread.h>

int printer = 0;

enum { NUM_THREADS = 4 };

pthread_mutex_t printerMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t printerAvailable = PTHREAD_COND_INITIALIZER;
bool isPrinterAvailable = true;

void *use_printer(void *args) {
  int value = *(int *)args;
  pthread_mutex_lock(&printerMutex);
  while (!isPrinterAvailable) {
    pthread_cond_wait(&printerAvailable, &printerMutex);
  }
  isPrinterAvailable = false;
  printer++;
  std::cout << "Value: " << value << std::endl;
  isPrinterAvailable = true;
  pthread_cond_broadcast(&printerAvailable);
  pthread_mutex_unlock(&printerMutex);
  pthread_exit(nullptr);
}

int main(int argc, char *argv[]) {
  if (argc != 5) {
    std::cerr << "Incorrect number of arguments. Correct is 4" << std::endl;
  }
  pthread_t threads[NUM_THREADS];
  int values[NUM_THREADS];
  for (int i = 0; i < NUM_THREADS; ++i) {
    values[i] = atoi(argv[i + 1]);
    int result = pthread_create(&threads[i], nullptr, use_printer, &values[i]);
    if (result != 0) {
      std::cerr << "Error creating pthread" << std::endl;
      exit(result);
    }
  }
  for (int i = 0; i < NUM_THREADS; ++i) {
    pthread_join(threads[i], nullptr);
  }
  return 0;
}
