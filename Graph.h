
#pragma once
#ifndef GRAPH_H
#define GRAPH_H

#include "Port.h"
#include "Route.h"
#include "LinkedList.h"
#include "Queue.h"
#include <string>
using namespace std;
class Graph
{
private:
    struct EdgeNode
    {
        Route route;
        EdgeNode *next;

        EdgeNode(const Route &r) : route(r), next(nullptr) {}
    };

    struct VertexNode
    {
        Port port;
        EdgeNode *edges;
        Queue<string> *dockingQueue;

        VertexNode() : port(), edges(nullptr), dockingQueue(nullptr) {}
        VertexNode(const Port &p) : port(p), edges(nullptr)
        {
            dockingQueue = new Queue<string>();
        }
        ~VertexNode()
        {
            while (edges)
            {
                EdgeNode *temp = edges;
                edges = edges->next;
                delete temp;
            }
            if (dockingQueue)
            {
                delete dockingQueue;
            }
        }
    };

    LinkedList<VertexNode *> vertices;
    int vertexCount;

    // Helper: Find port index by name (linear search - NO HASHTABLE!)
    int findPortIndex(const string &portName) const
    {
        for (int i = 0; i < vertices.getSize(); i++)
        {
            if (vertices.get(i)->port.name == portName)
            {
                return i;
            }
        }
        return -1;
    }

public:
    Graph() : vertexCount(0) {}

    ~Graph()
    {
        for (int i = 0; i < vertices.getSize(); i++)
        {
            delete vertices.get(i);
        }
    }

    void addPort(const Port &port)
    {
        if (hasPort(port.name))
        {
            return;
        }

        VertexNode *newVertex = new VertexNode(port);
        vertices.push_back(newVertex);
        vertexCount++;
    }

    void addRoute(const Route &route)
    {
        int originIndex = findPortIndex(route.origin);
        if (originIndex == -1)
        {
            return;
        }

        VertexNode *originVertex = vertices.get(originIndex);
        EdgeNode *newEdge = new EdgeNode(route);

        newEdge->next = originVertex->edges;
        originVertex->edges = newEdge;
    }

    LinkedList<Route> getRoutesFrom(const string &portName) const
    {
        LinkedList<Route> routes;
        int index = findPortIndex(portName);

        if (index != -1)
        {
            VertexNode *vertex = vertices.get(index);
            EdgeNode *current = vertex->edges;

            while (current)
            {
                routes.push_back(current->route);
                current = current->next;
            }
        }

        return routes;
    }

    LinkedList<Route> getRoutesFromOnDate(const string &portName,
                                          const string &date) const
    {
        LinkedList<Route> routes;
        int index = findPortIndex(portName);

        if (index != -1)
        {
            VertexNode *vertex = vertices.get(index);
            EdgeNode *current = vertex->edges;

            while (current)
            {
                if (current->route.date == date)
                {
                    routes.push_back(current->route);
                }
                current = current->next;
            }
        }

        return routes;
    }

    // Get routes from a port on the next day (for next-day connections)
    LinkedList<Route> getRoutesFromNextDay(const string &portName,
                                           const string &currentDate) const
    {
        LinkedList<Route> routes;
        string nextDay = Route::getNextDay(currentDate);
        return getRoutesFromOnDate(portName, nextDay);
    }

    // Get routes from a port that can connect after a given arrival time and date
    // This includes same-day routes (if departure >= arrival) and any future routes
    // Supports multi-day waiting if cheaper
    LinkedList<Route> getConnectingRoutes(const string &portName,
                                          const string &arrivalDate, const string &arrivalTime) const
    {
        LinkedList<Route> validRoutes;

        // Get ALL routes from this port
        LinkedList<Route> allRoutes = getRoutesFrom(portName);

        for (int i = 0; i < allRoutes.getSize(); i++)
        {
            const Route &route = allRoutes.get(i);

            // Check if route departs on or after arrival date
            if (Route::compareDates(route.date, arrivalDate) > 0)
            {
                // Future date - always valid
                validRoutes.push_back(route);
            }
            else if (route.date == arrivalDate)
            {
                // Same date - check if departure time >= arrival time
                if (Route::isTimeBefore(arrivalTime, route.departureTime) || arrivalTime == route.departureTime)
                {
                    validRoutes.push_back(route);
                }
            }
            // If route.date < arrivalDate, skip it (past routes not valid)
        }

        return validRoutes;
    }

    LinkedList<Route> getAllRoutes() const
    {
        LinkedList<Route> allRoutes;

        for (int i = 0; i < vertices.getSize(); i++)
        {
            VertexNode *vertex = vertices.get(i);
            EdgeNode *current = vertex->edges;

            while (current)
            {
                allRoutes.push_back(current->route);
                current = current->next;
            }
        }

        return allRoutes;
    }

    LinkedList<Port> getAllPorts() const
    {
        LinkedList<Port> allPorts;

        for (int i = 0; i < vertices.getSize(); i++)
        {
            allPorts.push_back(vertices.get(i)->port);
        }

        return allPorts;
    }

    bool hasPort(const string &portName) const
    {
        return findPortIndex(portName) != -1;
    }

    bool getPort(const string &portName, Port &port) const
    {
        int index = findPortIndex(portName);
        if (index != -1)
        {
            port = vertices.get(index)->port;
            return true;
        }
        return false;
    }

    void addShipToQueue(const string &portName, const string &shipName)
    {
        int index = findPortIndex(portName);
        if (index != -1)
        {
            vertices.get(index)->dockingQueue->enqueue(shipName);
        }
    }

    bool removeShipFromQueue(const string &portName)
    {
        int index = findPortIndex(portName);
        if (index != -1)
        {
            return vertices.get(index)->dockingQueue->dequeue();
        }
        return false;
    }

    int getQueueSize(const string &portName) const
    {
        int index = findPortIndex(portName);
        if (index != -1)
        {
            return vertices.get(index)->dockingQueue->getSize();
        }
        return 0;
    }

    // Get all ships in docking queue for a port
    LinkedList<string> getQueueShips(const string &portName) const
    {
        LinkedList<string> ships;
        int index = findPortIndex(portName);
        if (index != -1 && vertices.get(index)->dockingQueue != nullptr)
        {
            Queue<string> *queue = vertices.get(index)->dockingQueue;
            // Create a temporary queue to preserve original
            Queue<string> tempQueue;
            while (!queue->isEmpty())
            {
                string ship = queue->getFront();
                ships.push_back(ship);
                tempQueue.enqueue(ship);
                queue->dequeue();
            }
            // Restore original queue
            while (!tempQueue.isEmpty())
            {
                queue->enqueue(tempQueue.getFront());
                tempQueue.dequeue();
            }
        }
        return ships;
    }

    int getVertexCount() const { return vertexCount; }
};

#endif