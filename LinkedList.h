
#ifndef LINKEDLIST_H
#define LINKEDLIST_H
template <typename T>
class LinkedList
{
private:
    struct Node
    {
        T data;
        Node *next;
        Node(const T &value) : data(value), next(nullptr) {}
    };
    Node *head;
    Node *tail;
    int size;

public:
    LinkedList() : head(nullptr), tail(nullptr), size(0) {}
    // CRITICAL: Copy constructor (fixes the crash!)
    LinkedList(const LinkedList &other) : head(nullptr), tail(nullptr), size(0)
    {
        Node *current = other.head;
        while (current)
        {
            push_back(current->data);
            current = current->next;
        }
    }
    // CRITICAL: Assignment operator (fixes the crash!)
    LinkedList &operator=(const LinkedList &other)
    {
        if (this != &other)
        {
            clear();
            Node *current = other.head;
            while (current)
            {
                push_back(current->data);
                current = current->next;
            }
        }
        return *this;
    }
    ~LinkedList()
    {
        clear();
    }
    void push_back(const T &value)
    {
        Node *newNode = new Node(value);
        if (!head)
        {
            head = tail = newNode;
        }
        else
        {
            tail->next = newNode;
            tail = newNode;
        }
        size++;
    }
    void push_front(const T &value)
    {
        Node *newNode = new Node(value);
        newNode->next = head;
        head = newNode;
        if (!tail)
        {
            tail = head;
        }
        size++;
    }
    bool remove(int index)
    {
        if (index < 0 || index >= size)
            return false;
        if (index == 0)
        {
            Node *temp = head;
            head = head->next;
            delete temp;
            if (!head)
                tail = nullptr;
            size--;
            return true;
        }
        Node *current = head;
        for (int i = 0; i < index - 1; i++)
        {
            current = current->next;
        }
        Node *temp = current->next;
        current->next = temp->next;
        if (temp == tail)
            tail = current;
        delete temp;
        size--;
        return true;
    }
    T &get(int index)
    {
        // SAFETY CHECK: Prevents crash
        if (index < 0 || index >= size)
        {
            static T dummy;
            return dummy;
        }
        Node *current = head;
        for (int i = 0; i < index; i++)
        {
            current = current->next;
        }
        return current->data;
    }
    const T &get(int index) const
    {
        // SAFETY CHECK: Prevents crash
        if (index < 0 || index >= size)
        {
            static T dummy;
            return dummy;
        }
        Node *current = head;
        for (int i = 0; i < index; i++)
        {
            current = current->next;
        }
        return current->data;
    }
    int getSize() const { return size; }
    bool isEmpty() const { return size == 0; }
    void clear()
    {
        while (head)
        {
            Node *temp = head;
            head = head->next;
            delete temp;
        }
        tail = nullptr;
        size = 0;
    }
    // Iterator support for range-based for loops
    class Iterator
    {
    private:
        Node *current;

    public:
        Iterator(Node *node) : current(node) {}
        T &operator*() { return current->data; }
        Iterator &operator++()
        {
            current = current->next;
            return *this;
        }
        bool operator!=(const Iterator &other) const
        {
            return current != other.current;
        }
    };
    Iterator begin() { return Iterator(head); }
    Iterator end() { return Iterator(nullptr); }
};
#endif