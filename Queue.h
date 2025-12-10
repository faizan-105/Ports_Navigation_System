
#pragma once
#ifndef QUEUE_H
#define QUEUE_H
template <typename T>
class Queue
{
private:
    struct Node
    {
        T data;
        Node *next;
        Node(const T &value) : data(value), next(nullptr) {}
    };
    Node *front;
    Node *rear;
    int size;

public:
    Queue() : front(nullptr), rear(nullptr), size(0) {}
    ~Queue()
    {
        clear();
    }
    void enqueue(const T &value)
    {
        Node *newNode = new Node(value);
        if (!rear)
        {
            front = rear = newNode;
        }
        else
        {
            rear->next = newNode;
            rear = newNode;
        }
        size++;
    }
    bool dequeue()
    {
        if (!front)
            return false;
        Node *temp = front;
        front = front->next;
        if (!front)
            rear = nullptr;
        delete temp;
        size--;
        return true;
    }
    T &getFront()
    {
        return front->data;
    }
    const T &getFront() const
    {
        return front->data;
    }
    bool isEmpty() const
    {
        return size == 0;
    }
    int getSize() const
    {
        return size;
    }
    void clear()
    {
        while (front)
        {
            Node *temp = front;
            front = front->next;
            delete temp;
        }
        rear = nullptr;
        size = 0;
    }
};
#endif