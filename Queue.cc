#include "Queue.h"

bool Queue::enqueue(int i) {
    if (!full()) {
        buffer[tail] = i;
        tail = (tail + 1) % SIZE;
        lngth++;
        return true;
    }
    return false;
}

bool Queue::dequeue(int &i) {
    if(!empty()) {
        i = buffer[head];
        head = (head + 1) % SIZE;
        lngth--;
        return true;
    }
    return false;
}