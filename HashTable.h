
#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <string>

template <typename V>
class HashTable
{
private:
    struct Entry
    {
        string key;
        V value;
        bool occupied;
        bool deleted;

        Entry() : key(""), value(), occupied(false), deleted(false) {}
    };

    Entry *table;
    int capacity;
    int size;

    int hashFunction(const string &key) const
    {
        if (capacity == 0)
            return 0; // Safety check

        unsigned int hash = 0;
        for (size_t i = 0; i < key.length(); i++)
        {
            hash = hash * 31 + static_cast<unsigned char>(key[i]);
        }
        return hash % capacity;
    }

    int probe(int index, int i) const
    {
        return (index + i) % capacity;
    }

    void resize()
    {
        int oldCapacity = capacity;
        Entry *oldTable = table;

        capacity *= 2;
        table = new Entry[capacity];
        size = 0;

        for (int i = 0; i < oldCapacity; i++)
        {
            if (oldTable[i].occupied && !oldTable[i].deleted)
            {
                insert(oldTable[i].key, oldTable[i].value);
            }
        }

        delete[] oldTable;
    }

public:
    HashTable(int initialCapacity = 101) : capacity(initialCapacity), size(0)
    {
        if (capacity < 10)
            capacity = 10; // Minimum capacity
        table = new Entry[capacity];
    }

    ~HashTable()
    {
        delete[] table;
    }

    // Copy constructor
    HashTable(const HashTable &other) : capacity(other.capacity), size(0)
    {
        table = new Entry[capacity];
        for (int i = 0; i < other.capacity; i++)
        {
            if (other.table[i].occupied && !other.table[i].deleted)
            {
                insert(other.table[i].key, other.table[i].value);
            }
        }
    }

    // Assignment operator
    HashTable &operator=(const HashTable &other)
    {
        if (this != &other)
        {
            delete[] table;
            capacity = other.capacity;
            size = 0;
            table = new Entry[capacity];
            for (int i = 0; i < other.capacity; i++)
            {
                if (other.table[i].occupied && !other.table[i].deleted)
                {
                    insert(other.table[i].key, other.table[i].value);
                }
            }
        }
        return *this;
    }

    void insert(const string &key, const V &value)
    {
        if (capacity == 0 || size >= capacity * 0.7)
        {
            resize();
        }

        int index = hashFunction(key);
        int i = 0;

        while (i < capacity && table[probe(index, i)].occupied &&
               !table[probe(index, i)].deleted &&
               table[probe(index, i)].key != key)
        {
            i++;
        }

        if (i >= capacity)
        {
            // Table full, resize
            resize();
            insert(key, value);
            return;
        }

        int finalIndex = probe(index, i);
        if (!table[finalIndex].occupied || table[finalIndex].deleted)
        {
            size++;
        }

        table[finalIndex].key = key;
        table[finalIndex].value = value;
        table[finalIndex].occupied = true;
        table[finalIndex].deleted = false;
    }

    bool find(const string &key, V &value) const
    {
        if (capacity == 0)
            return false;

        int index = hashFunction(key);
        int i = 0;

        while (i < capacity && table[probe(index, i)].occupied)
        {
            int currentIndex = probe(index, i);
            if (!table[currentIndex].deleted && table[currentIndex].key == key)
            {
                value = table[currentIndex].value;
                return true;
            }
            i++;
        }

        return false;
    }

    bool contains(const string &key) const
    {
        V dummy;
        return find(key, dummy);
    }

    bool remove(const string &key)
    {
        if (capacity == 0)
            return false;

        int index = hashFunction(key);
        int i = 0;

        while (i < capacity && table[probe(index, i)].occupied)
        {
            int currentIndex = probe(index, i);
            if (!table[currentIndex].deleted && table[currentIndex].key == key)
            {
                table[currentIndex].deleted = true;
                size--;
                return true;
            }
            i++;
        }

        return false;
    }

    int getSize() const { return size; }
    bool isEmpty() const { return size == 0; }
};

#endif