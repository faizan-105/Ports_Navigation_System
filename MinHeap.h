
#pragma once
#ifndef MINHEAP_H
#define MINHEAP_H
template <typename T>
class MinHeap
{
private:
    struct HeapNode
    {
        T data;
        int priority;
        HeapNode() : data(), priority(0) {}
        HeapNode(const T &d, int p) : data(d), priority(p) {}
    };
    HeapNode *heap;
    int capacity;
    int size;
    void heapifyUp(int index)
    {
        while (index > 0)
        {
            int parent = (index - 1) / 2;
            if (heap[index].priority < heap[parent].priority)
            {
                HeapNode temp = heap[index];
                heap[index] = heap[parent];
                heap[parent] = temp;
                index = parent;
            }
            else
            {
                break;
            }
        }
    }
    void heapifyDown(int index)
    {
        while (true)
        {
            int smallest = index;
            int left = 2 * index + 1;
            int right = 2 * index + 2;
            if (left < size && heap[left].priority < heap[smallest].priority)
            {
                smallest = left;
            }
            if (right < size && heap[right].priority < heap[smallest].priority)
            {
                smallest = right;
            }
            if (smallest != index)
            {
                HeapNode temp = heap[index];
                heap[index] = heap[smallest];
                heap[smallest] = temp;
                index = smallest;
            }
            else
            {
                break;
            }
        }
    }
    void resize()
    {
        capacity *= 2;
        HeapNode *newHeap = new HeapNode[capacity];
        for (int i = 0; i < size; i++)
        {
            newHeap[i] = heap[i];
        }
        delete[] heap;
        heap = newHeap;
    }

public:
    MinHeap(int initialCapacity = 100) : capacity(initialCapacity), size(0)
    {
        heap = new HeapNode[capacity];
    }
    ~MinHeap()
    {
        delete[] heap;
    }
    void insert(const T &data, int priority)
    {
        if (size >= capacity)
        {
            resize();
        }
        heap[size] = HeapNode(data, priority);
        heapifyUp(size);
        size++;
    }
    bool extractMin(T &data, int &priority)
    {
        if (size == 0)
            return false;
        data = heap[0].data;
        priority = heap[0].priority;
        heap[0] = heap[size - 1];
        size--;
        heapifyDown(0);
        return true;
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
        size = 0;
    }
};
#endif