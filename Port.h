
#pragma once
#ifndef PORT_H
#define PORT_H
#include <string>
using namespace std;
class Port
{
public:
    string name;
    float x, y;      // Coordinates for SFML rendering
    int dailyCharge; // From PortCharges.txt
    Port() : name(""), x(0), y(0), dailyCharge(0) {}
    Port(const string &portName, float xPos, float yPos, int charge)
        : name(portName), x(xPos), y(yPos), dailyCharge(charge)
    {
    }
    bool operator==(const Port &other) const
    {
        return name == other.name;
    }
};
#endif