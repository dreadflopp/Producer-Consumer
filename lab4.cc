/**
 * @author Mattias Lindell
 * @date 181016
 */

#include <iostream>
#include <unistd.h>
#include <new>
#include <random>
#include <climits>
#include <sstream>
#include <cstring>
#include "Queue.h"
#include "SharedMem.h"
#include "semaphore.h"
#include "Restart.h"

const int DEFAULT_PRODTOTAL = 5000;
bool SLEEP = false;

// semaphores
sem_t *spaceAvail, *itemAvail, *lock;

// buffer
Queue *buffer;
const int BUFFSIZE = 10;

void randomSleep();
int producer(int amount);
int consumer(int amount);

int main(int argc, char *argv[]) {
    // parse arguments
    int prodTotal = DEFAULT_PRODTOTAL;
    if (argc > 3) {
        fprintf(stderr, "Usage: %s numbersToProduce sleep[optional]\n", argv[0]);
        return 1;
    }
    if (argc == 2) {
        std::istringstream firstArg(argv[1]);
        if (!(firstArg >> prodTotal)) {
            fprintf(stderr, "Usage: %s numbersToProduce sleep[optional]\n", argv[0]);
            return 1;
        }
    } else if (argc == 3) {
        std::istringstream firstArg(argv[1]);
        if (!(firstArg >> prodTotal) || strcmp(argv[2], "sleep") != 0) {
            fprintf(stderr, "Usage: %s numbersToProduce sleep[optional]\n", argv[0]);
            return 1;
        }
        SLEEP = true;
    }

    // create shared memory
    SharedMem sharedMem;
    sharedMem.allocate(IPC_PRIVATE, 3 * sizeof(sem_t) + sizeof(Queue));

    if (sharedMem.attach() == -1) {
        fprintf(stderr, "failed to attach shared memory");
        return 1;
    }

    // put semaphores in shared mem
    char *memAddress = sharedMem.getAddr();
    spaceAvail = new (memAddress) sem_t;
    itemAvail = new (memAddress + sizeof(sem_t)) sem_t;
    lock = new (memAddress + 2 * sizeof(sem_t)) sem_t;

    // put buffer in the shared memory
    buffer = new (memAddress + 4 * sizeof(sem_t)) Queue;

    // initialize semaphores
    if (sem_init(spaceAvail, 1, BUFFSIZE) == -1 || sem_init(itemAvail, 1, 0) == -1 || sem_init(lock, 1, 1) == -1) {
        perror("Failed to initialize semaphore\n");
        return 1;
    }

    // create a child processes
    pid_t pid;  // If this variable is zero after a fork, the process is a child
    if ((pid = fork()) == -1) {
        perror("Failed to create a process\n");
        return 1;
    }

    /* child specific code */
    if (pid == 0) {
        if (sharedMem.attach() == -1) {
            fprintf(stderr, "failed to attach shared memory");
            return 1;
        }
        if (consumer(prodTotal) == 1) {
            return 1;
        }
        if (sharedMem.detach() == -1) {
            fprintf(stderr, "failed to detach shared memory");
            return 1;
        }
    }

    /* parent specific code */
    else {
        if (sharedMem.attach() == -1)  {
            fprintf(stderr, "failed to attach shared memory");
            return 1;
        }
        if (producer(prodTotal) == 1) {
            return 1;
        }
        if (sharedMem.detach() == -1)  {
            fprintf(stderr, "failed to detach shared memory");
            return 1;
        }

        // wait for all children
        while (r_wait(NULL) > 0);

        // clean up
        sem_destroy(spaceAvail);
        sem_destroy(itemAvail);
        sem_destroy(lock);
        buffer->~Queue();

        sharedMem.remove();
    }

    return 0;
}

int producer(int amount) {
    // produce random ints and place in shared memory
    std::default_random_engine generator;
    std::uniform_int_distribution<int> distribution(1,1000);

    int producedCounter = 0;
    while(producedCounter < amount) {

        // decrease space available. Blocks if no space is available.
        while (sem_wait(spaceAvail) == -1) {
            if(errno != EINTR) {
                fprintf(stderr, "failed to lock semaphore\n");
                return 1;
            }
        }

        // *** lock ***
        while (sem_wait(lock) == -1) {
            if(errno != EINTR) {
                fprintf(stderr, "failed to lock semaphore\n");
                return 1;
            }
        }

        // *** critical section ***
        // increase prod counter
        producedCounter++;

        // random data
        int number = distribution(generator);

        // add value to buffer and print result
        buffer->enqueue(number);
        fprintf(stdout, "Produced (%d/%d): %d nBuffer=%d\n", producedCounter, amount, number, buffer->length());

        // *************************


        // *** unlock ***
        if (sem_post(lock) == -1) {
            fprintf(stderr, "Thread failed to unlock semaphore\n");
            return 1;
        }

        // increase items available
        if (sem_post(itemAvail) == -1) {
            fprintf(stderr, "failed to post semaphore itemAvail\n");
            return 1;
        }

        randomSleep();
    }
    return 0;
}

int consumer(int amount) {
    int number;
    int consumedCounter = 0;
    while(consumedCounter < amount) {
        // decrease items available. Blocks if no items are available.
        sem_wait(itemAvail);

        // *** lock ***
        while (sem_wait(lock) == -1) {
            if(errno != EINTR) {
                fprintf(stderr, "failed to lock semaphore\n");
                return 1;
            }
        }

        // *** citical section ***
        // increase consumed counter
        consumedCounter++;

        // dequeue number and print result
        buffer->dequeue(number);
        fprintf(stdout, "\tConsumed (%d/%d): %d nBuffer=%d\n", consumedCounter, amount, number, buffer->length());

        // *************************

        // *** unlock ***
        if (sem_post(lock) == -1) {
            fprintf(stderr, "failed to unlock semaphore\n");
            return 1;
        }

        // increase space available
        if (sem_post(spaceAvail) == -1) {
            fprintf(stderr, "failed to post semaphore spaceAvail\n");
            return 1;
        }

        randomSleep();
    }
    return 0;
}

void randomSleep() {
    if (SLEEP) {
        struct timespec tv;
        int msec = (int) (((double) random() / INT_MAX) * 1000);
        tv.tv_sec = 0;
        tv.tv_nsec = 1000000 * msec;
        if (nanosleep(&tv, NULL) == -1) {
            std::cerr << "nanosleep error";
        }
    }
}