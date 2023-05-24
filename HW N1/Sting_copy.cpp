#include <iostream>
#include <pthread.h>
#include <cstring>

enum{NUM_THREADS = 4};

typedef struct{
	char* _dest;
	const char* _src;
	int _start;
	int _size;
}Params;

void* str_copy(void* data) {
	Params* args = (Params*) data;
	strncpy(args->_dest + args->_start, args->_src + args->_start, args->_size);
	pthread_exit(nullptr);
}

int main(int argc, char* argv[]) {
	if(argc != 2) {
		std::cerr << "Incorrect number of arguments" << std::endl;
		exit(1);
	}
	const char* src;
	char dest[256];
	src = argv[1];
	int start = 0;
	int size = strlen(src);
	int seg_size = size / NUM_THREADS;
	pthread_t threads[NUM_THREADS];
	Params args[NUM_THREADS];
	for(int i = 0; i < NUM_THREADS; ++i) {
		args[i]._dest = dest;
		args[i]._start = i * seg_size;
		args[i]._src = src;
		args[i]._size = (i == NUM_THREADS - 1) ? (size - i * seg_size) : seg_size;

		int result = pthread_create(&threads[i], nullptr, str_copy, &args[i]);
		if(result != 0) {
			std::cerr << "Error creating pthread" << std::endl;
			exit(result);
		}
	}

	for(int i = 0; i < NUM_THREADS; ++i) {
		pthread_join(threads[i], nullptr);
	}
	std::cout << "Copied string in dest: " << dest << std::endl;
	return 0;
}
