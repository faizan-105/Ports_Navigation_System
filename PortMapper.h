
#pragma once
#ifndef PORTMAPPER_H
#define PORTMAPPER_H
#include <string>
#include "LinkedList.h"
using namespace std;
// Simple port mapper that doesn't use HashTable with strings
class PortMapper
{
private:
    LinkedList<string> portNames;

public:
    PortMapper() {}
    // Add a port name and return its index
    int addPort(const string &portName)
    {
        // Check if already exists
        for (int i = 0; i < portNames.getSize(); i++)
        {
            if (portNames.get(i) == portName)
            {
                return i;
            }
        }
        // Add new port
        portNames.push_back(portName);
        return portNames.getSize() - 1;
    }
    // Find index of port name (returns -1 if not found)
    int findIndex(const string &portName) const
    {
        for (int i = 0; i < portNames.getSize(); i++)
        {
            if (portNames.get(i) == portName)
            {
                return i;
            }
        }
        return -1;
    }
    // Get port name by index
    string getName(int index) const
    {
        if (index >= 0 && index < portNames.getSize())
        {
            return portNames.get(index);
        }
        return "";
    }
    // Get total number of ports
    int getSize() const
    {
        return portNames.getSize();
    }
};
#endif