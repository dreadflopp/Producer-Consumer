/**
 * @author Mattias Lindell
 * @date 181013
 * @desc FIFO queue
 */

#ifndef QUEUE_H
#define QUEUE_H
#define SIZE 10

class Queue {
private:
    int buffer[SIZE];
    int head;
    int tail;
    int lngth;
public:
    Queue(): head(0), tail(0), lngth(0) {}
    bool full() { return lngth == SIZE; }
    bool empty() { return lngth == 0; }
    bool enqueue(int i);
    bool dequeue(int &i);
    int length() { return lngth; }
};


#endif //QUEUE_H
