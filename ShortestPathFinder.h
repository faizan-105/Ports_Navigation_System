#ifndef SHORTEST_PATH_FINDER_H
#define SHORTEST_PATH_FINDER_H

#include "PathFinder.h"
#include "LinkedList.h"
#include "PortMapper.h"
#include <limits>
#include <cmath>
using namespace std;
class ShortestPathFinder
{
private:
    Graph *graph;

public:
    ShortestPathFinder(Graph *g) : graph(g) {}

    // Find shortest path (minimum hops/distance) - same result for both standard and bidirectional
    PathResult findShortestPath(const string &origin,
                                const string &destination,
                                const string &date)
    {
        PathResult result;

        cout << "\n=== Finding SHORTEST path using Dijkstra (Minimum Hops) ===" << endl;

        // Validate ports exist
        if (!graph->hasPort(origin))
        {
            cout << "ERROR: Origin not found!" << endl;
            return result;
        }
        if (!graph->hasPort(destination))
        {
            cout << "ERROR: Destination not found!" << endl;
            return result;
        }

        // Build port mapper
        PortMapper portMapper;
        LinkedList<Port> allPorts = graph->getAllPorts();

        for (int i = 0; i < allPorts.getSize(); i++)
        {
            portMapper.addPort(allPorts.get(i).name);
        }

        int numPorts = portMapper.getSize();
        cout << "Mapped " << numPorts << " ports" << endl;

        // Get indices
        int originIdx = portMapper.findIndex(origin);
        int destIdx = portMapper.findIndex(destination);

        if (originIdx == -1 || destIdx == -1)
        {
            cout << "ERROR: Could not find port indices!" << endl;
            return result;
        }

        // Arrays for Dijkstra - minimize hops instead of cost
        int *hops = new int[numPorts]; // Number of hops (routes taken)
        int *parent = new int[numPorts];
        bool *visited = new bool[numPorts];
        int *totalCost = new int[numPorts]; // Store actual cost for display, but optimize by hops
        string *arrivalDates = new string[numPorts];
        string *arrivalTimes = new string[numPorts];

        for (int i = 0; i < numPorts; i++)
        {
            hops[i] = numeric_limits<int>::max();
            totalCost[i] = numeric_limits<int>::max();
            parent[i] = -1;
            visited[i] = false;
            arrivalDates[i] = "";
            arrivalTimes[i] = "";
        }

        hops[originIdx] = 0;
        totalCost[originIdx] = 0;
        arrivalDates[originIdx] = date;
        arrivalTimes[originIdx] = "00:00";

        cout << "Running Dijkstra's algorithm for shortest path (minimum hops)..." << endl;

        // Dijkstra's algorithm optimizing for hops
        while (true)
        {
            int minHops = numeric_limits<int>::max();
            int minIdx = -1;

            // Find unvisited node with minimum hops
            for (int i = 0; i < numPorts; i++)
            {
                if (!visited[i] && hops[i] < minHops)
                {
                    minHops = hops[i];
                    minIdx = i;
                }
            }

            if (minIdx == -1)
                break;

            visited[minIdx] = true;

            // Found destination
            if (minIdx == destIdx)
            {
                cout << "FOUND SHORTEST PATH TO DESTINATION!" << endl;
                result.found = true;
                result.totalCost = totalCost[destIdx];
                break;
            }

            // Update neighbors
            string currentPort = portMapper.getName(minIdx);
            string currentArrivalDate = arrivalDates[minIdx];
            string currentArrivalTime = arrivalTimes[minIdx];

            LinkedList<Route> connectingRoutes = graph->getConnectingRoutes(
                currentPort, currentArrivalDate, currentArrivalTime);

            for (int i = 0; i < connectingRoutes.getSize(); i++)
            {
                const Route &route = connectingRoutes.get(i);
                int neighborIdx = portMapper.findIndex(route.destination);

                if (neighborIdx == -1 || visited[neighborIdx])
                    continue;

                // Calculate layover hours and port charge
                int layoverHours = 0;
                int portCharge = 0;
                bool connectionValid = true;

                if (parent[minIdx] != -1)
                {
                    string fromPort = portMapper.getName(parent[minIdx]);
                    string toPort = currentPort;
                    string prevArrivalDate = arrivalDates[minIdx];
                    string prevArrivalTime = arrivalTimes[minIdx];

                    LinkedList<Route> prevRoutes = graph->getConnectingRoutes(fromPort, arrivalDates[parent[minIdx]], arrivalTimes[parent[minIdx]]);
                    for (int j = 0; j < prevRoutes.getSize(); j++)
                    {
                        if (prevRoutes.get(j).destination == toPort &&
                            prevRoutes.get(j).date == prevArrivalDate)
                        {
                            const Route &previousRoute = prevRoutes.get(j);
                            if (!previousRoute.canConnectTo(route))
                            {
                                connectionValid = false;
                            }
                            else
                            {
                                layoverHours = Route::calculateLayoverHours(previousRoute, route);
                            }
                            break;
                        }
                    }
                }

                if (!connectionValid)
                {
                    continue;
                }

                // Apply port charge if layover > 12 hours
                if (layoverHours > 12)
                {
                    Port currentPortInfo;
                    if (graph->getPort(currentPort, currentPortInfo))
                    {
                        int days = (layoverHours + 11) / 24;
                        if (days == 0)
                            days = 1;
                        portCharge = currentPortInfo.dailyCharge * days;
                    }
                }

                // Optimize by hops, not cost
                int newHops = hops[minIdx] + 1; // Each route taken is one hop
                int newCost = totalCost[minIdx] + route.cost + portCharge;

                if (newHops < hops[neighborIdx])
                {
                    hops[neighborIdx] = newHops;
                    totalCost[neighborIdx] = newCost;
                    parent[neighborIdx] = minIdx;
                    arrivalDates[neighborIdx] = route.date;
                    arrivalTimes[neighborIdx] = route.arrivalTime;
                }
                // If same number of hops, choose the one with lower cost
                else if (newHops == hops[neighborIdx] && newCost < totalCost[neighborIdx])
                {
                    totalCost[neighborIdx] = newCost;
                    parent[neighborIdx] = minIdx;
                    arrivalDates[neighborIdx] = route.date;
                    arrivalTimes[neighborIdx] = route.arrivalTime;
                }
            }
        }

        // Reconstruct path
        if (result.found)
        {
            cout << "Reconstructing shortest path..." << endl;

            LinkedList<int> pathIndices;
            int current = destIdx;
            while (current != -1)
            {
                pathIndices.push_back(current);
                current = parent[current];
            }

            // Reverse to get correct order
            for (int i = pathIndices.getSize() - 1; i >= 0; i--)
            {
                result.path.push_back(portMapper.getName(pathIndices.get(i)));
            }

            // Reconstruct routes by querying the graph
            for (int i = pathIndices.getSize() - 1; i > 0; i--)
            {
                int fromIdx = pathIndices.get(i);
                int toIdx = pathIndices.get(i - 1);
                string fromPort = portMapper.getName(fromIdx);
                string toPort = portMapper.getName(toIdx);
                string departDate = arrivalDates[fromIdx];
                string departTime = arrivalTimes[fromIdx];
                string arriveDate = arrivalDates[toIdx];

                // Find the route used
                LinkedList<Route> routes = graph->getConnectingRoutes(fromPort, departDate, departTime);
                for (int j = 0; j < routes.getSize(); j++)
                {
                    if (routes.get(j).destination == toPort && routes.get(j).date == arriveDate)
                    {
                        result.routes.push_back(routes.get(j));
                        break;
                    }
                }
            }

            // Calculate and store layover information
            for (int i = 0; i < result.routes.getSize() - 1; i++)
            {
                const Route &arrivingRoute = result.routes.get(i);
                const Route &departingRoute = result.routes.get(i + 1);

                string layoverPort = arrivingRoute.destination;

                int layoverHours = Route::calculateLayoverHours(arrivingRoute, departingRoute);

                int portCharge = 0;
                if (layoverHours > 12)
                {
                    Port portInfo;
                    if (graph->getPort(layoverPort, portInfo))
                    {
                        int days = (layoverHours + 11) / 24;
                        if (days == 0)
                            days = 1;
                        portCharge = portInfo.dailyCharge * days;
                    }
                }

                LayoverInfo layover(layoverPort, layoverHours, portCharge,
                                    arrivingRoute.date, arrivingRoute.arrivalTime,
                                    departingRoute.date, departingRoute.departureTime);
                result.layovers.push_back(layover);
            }

            cout << "Optimal Path (Shortest): ";
            for (int i = 0; i < result.path.getSize(); i++)
            {
                cout << result.path.get(i);
                if (i < result.path.getSize() - 1)
                    cout << " -> ";
            }
            cout << endl;

            cout << "\nRoute Details:" << endl;
            int routeCost = 0;
            for (int i = 0; i < result.routes.getSize(); i++)
            {
                const Route &route = result.routes.get(i);
                routeCost += route.cost;
                cout << "  " << (i + 1) << ". " << route.origin << " -> " << route.destination
                     << " (Cost: $" << route.cost << ", " << route.date
                     << " " << route.departureTime << "-" << route.arrivalTime << ")" << endl;

                if (i < result.layovers.getSize())
                {
                    const LayoverInfo &layover = result.layovers.get(i);
                    cout << "     Docking at " << layover.portName << " for " << layover.layoverHours
                         << " hours (Arrived: " << layover.arrivalDate << " " << layover.arrivalTime
                         << ", Departed: " << layover.departureDate << " " << layover.departureTime << ")";
                    if (layover.layoverHours > 12)
                    {
                        cout << " [Port Charge: $" << layover.portCharge << " (>12h layover)]";
                    }
                    else
                    {
                        cout << " [No port charge (≤12h layover)]";
                    }
                    cout << endl;
                }
            }

            cout << "\nCost Breakdown:" << endl;
            cout << "  Route Costs: $" << routeCost << endl;
            int totalPortCharges = 0;
            for (int i = 0; i < result.layovers.getSize(); i++)
            {
                totalPortCharges += result.layovers.get(i).portCharge;
            }
            if (totalPortCharges > 0)
            {
                cout << "  Port Charges: $" << totalPortCharges << endl;
            }
            else
            {
                cout << "  Port Charges: $0 (no layovers > 12 hours)" << endl;
            }
            cout << "  Total Cost: $" << result.totalCost << " (Hops: " << hops[destIdx] << ")" << endl;

            // Calculate total travel time (route times + layover times)
            int totalTravelHours = 0;
            for (int i = 0; i < result.routes.getSize(); i++)
            {
                const Route &route = result.routes.get(i);
                // Each route takes approximately 24 hours (1 day travel)
                totalTravelHours += 24;
            }
            // Add layover hours
            for (int i = 0; i < result.layovers.getSize(); i++)
            {
                totalTravelHours += result.layovers.get(i).layoverHours;
            }
            result.totalTravelTime = totalTravelHours;
            cout << "  Total Travel Time: " << totalTravelHours << " hours ("
                 << (totalTravelHours / 24) << " days " << (totalTravelHours % 24) << " hours)" << endl;
        }
        else
        {
            cout << "No path found to destination!" << endl;
        }

        delete[] hops;
        delete[] parent;
        delete[] visited;
        delete[] totalCost;
        delete[] arrivalDates;
        delete[] arrivalTimes;

        return result;
    }

    // Find shortest path with preferences
    PathResult findShortestPathWithPreferences(const string &origin,
                                               const string &destination,
                                               const string &date,
                                               const PreferenceFilter &preferences)
    {
        PathResult result;

        cout << "\n=== Finding SHORTEST path with PREFERENCES using Dijkstra (Minimum Hops) ===" << endl;

        // Validate ports exist
        if (!graph->hasPort(origin))
        {
            cout << "ERROR: Origin not found!" << endl;
            return result;
        }
        if (!graph->hasPort(destination))
        {
            cout << "ERROR: Destination not found!" << endl;
            return result;
        }

        // Build port mapper
        PortMapper portMapper;
        LinkedList<Port> allPorts = graph->getAllPorts();

        for (int i = 0; i < allPorts.getSize(); i++)
        {
            portMapper.addPort(allPorts.get(i).name);
        }

        int numPorts = portMapper.getSize();
        cout << "Mapped " << numPorts << " ports with preference filtering..." << endl;

        int originIdx = portMapper.findIndex(origin);
        int destIdx = portMapper.findIndex(destination);

        if (originIdx == -1 || destIdx == -1)
        {
            cout << "ERROR: Could not find port indices!" << endl;
            return result;
        }

        // Arrays for Dijkstra with preferences
        int *hops = new int[numPorts];
        int *parent = new int[numPorts];
        bool *visited = new bool[numPorts];
        int *totalCost = new int[numPorts];
        string *arrivalDates = new string[numPorts];
        string *arrivalTimes = new string[numPorts];

        for (int i = 0; i < numPorts; i++)
        {
            hops[i] = numeric_limits<int>::max();
            totalCost[i] = numeric_limits<int>::max();
            parent[i] = -1;
            visited[i] = false;
            arrivalDates[i] = "";
            arrivalTimes[i] = "";
        }

        hops[originIdx] = 0;
        totalCost[originIdx] = 0;
        arrivalDates[originIdx] = date;
        arrivalTimes[originIdx] = "00:00";

        cout << "Running Dijkstra's algorithm for shortest path with preference filtering..." << endl;

        while (true)
        {
            int minHops = numeric_limits<int>::max();
            int minIdx = -1;

            for (int i = 0; i < numPorts; i++)
            {
                if (!visited[i] && hops[i] < minHops)
                {
                    minHops = hops[i];
                    minIdx = i;
                }
            }

            if (minIdx == -1)
                break;

            visited[minIdx] = true;

            if (minIdx == destIdx)
            {
                cout << "FOUND SHORTEST PATH TO DESTINATION!" << endl;
                result.found = true;
                result.totalCost = totalCost[destIdx];
                break;
            }

            string currentPort = portMapper.getName(minIdx);
            string currentArrivalDate = arrivalDates[minIdx];
            string currentArrivalTime = arrivalTimes[minIdx];

            LinkedList<Route> connectingRoutes = graph->getConnectingRoutes(
                currentPort, currentArrivalDate, currentArrivalTime);

            for (int i = 0; i < connectingRoutes.getSize(); i++)
            {
                const Route &route = connectingRoutes.get(i);

                // Apply preference filter
                if (!preferences.matchesRoute(route))
                {
                    continue;
                }

                int neighborIdx = portMapper.findIndex(route.destination);

                if (neighborIdx == -1 || visited[neighborIdx])
                    continue;

                // Check if neighbor is excluded
                if (preferences.hasPortPreference)
                {
                    bool isExcluded = false;
                    if (preferences.excludedPorts.getSize() > 0)
                    {
                        for (int k = 0; k < preferences.excludedPorts.getSize(); k++)
                        {
                            if (preferences.excludedPorts.get(k) == route.destination)
                            {
                                isExcluded = true;
                                break;
                            }
                        }
                    }
                    if (isExcluded)
                        continue;
                }

                // Calculate layover hours
                int layoverHours = 0;
                bool connectionValid = true;

                if (parent[minIdx] != -1)
                {
                    string fromPort = portMapper.getName(parent[minIdx]);
                    string toPort = currentPort;
                    string prevArrivalDate = arrivalDates[minIdx];
                    string prevArrivalTime = arrivalTimes[minIdx];

                    LinkedList<Route> prevRoutes = graph->getConnectingRoutes(fromPort, arrivalDates[parent[minIdx]], arrivalTimes[parent[minIdx]]);
                    for (int j = 0; j < prevRoutes.getSize(); j++)
                    {
                        if (prevRoutes.get(j).destination == toPort &&
                            prevRoutes.get(j).date == prevArrivalDate)
                        {
                            const Route &previousRoute = prevRoutes.get(j);
                            if (!previousRoute.canConnectTo(route))
                            {
                                connectionValid = false;
                            }
                            else
                            {
                                layoverHours = Route::calculateLayoverHours(previousRoute, route);
                            }
                            break;
                        }
                    }
                }

                if (!connectionValid)
                {
                    continue;
                }

                int portCharge = 0;
                if (layoverHours > 12)
                {
                    Port currentPortInfo;
                    if (graph->getPort(currentPort, currentPortInfo))
                    {
                        int days = (layoverHours + 11) / 24;
                        if (days == 0)
                            days = 1;
                        portCharge = currentPortInfo.dailyCharge * days;
                    }
                }

                int newHops = hops[minIdx] + 1;
                int newCost = totalCost[minIdx] + route.cost + portCharge;

                if (newHops < hops[neighborIdx])
                {
                    hops[neighborIdx] = newHops;
                    totalCost[neighborIdx] = newCost;
                    parent[neighborIdx] = minIdx;
                    arrivalDates[neighborIdx] = route.date;
                    arrivalTimes[neighborIdx] = route.arrivalTime;
                }
                else if (newHops == hops[neighborIdx] && newCost < totalCost[neighborIdx])
                {
                    totalCost[neighborIdx] = newCost;
                    parent[neighborIdx] = minIdx;
                    arrivalDates[neighborIdx] = route.date;
                    arrivalTimes[neighborIdx] = route.arrivalTime;
                }
            }
        }

        // Reconstruct path
        if (result.found)
        {
            cout << "Reconstructing shortest path..." << endl;

            LinkedList<int> pathIndices;
            int current = destIdx;
            while (current != -1)
            {
                pathIndices.push_back(current);
                current = parent[current];
            }

            for (int i = pathIndices.getSize() - 1; i >= 0; i--)
            {
                result.path.push_back(portMapper.getName(pathIndices.get(i)));
            }

            for (int i = pathIndices.getSize() - 1; i > 0; i--)
            {
                int fromIdx = pathIndices.get(i);
                int toIdx = pathIndices.get(i - 1);
                string fromPort = portMapper.getName(fromIdx);
                string toPort = portMapper.getName(toIdx);
                string departDate = arrivalDates[fromIdx];
                string departTime = arrivalTimes[fromIdx];
                string arriveDate = arrivalDates[toIdx];

                LinkedList<Route> routes = graph->getConnectingRoutes(fromPort, departDate, departTime);
                for (int j = 0; j < routes.getSize(); j++)
                {
                    if (routes.get(j).destination == toPort && routes.get(j).date == arriveDate)
                    {
                        result.routes.push_back(routes.get(j));
                        break;
                    }
                }
            }

            for (int i = 0; i < result.routes.getSize() - 1; i++)
            {
                const Route &arrivingRoute = result.routes.get(i);
                const Route &departingRoute = result.routes.get(i + 1);

                string layoverPort = arrivingRoute.destination;
                int layoverHours = Route::calculateLayoverHours(arrivingRoute, departingRoute);

                int portCharge = 0;
                if (layoverHours > 12)
                {
                    Port portInfo;
                    if (graph->getPort(layoverPort, portInfo))
                    {
                        int days = (layoverHours + 11) / 24;
                        if (days == 0)
                            days = 1;
                        portCharge = portInfo.dailyCharge * days;
                    }
                }

                LayoverInfo layover(layoverPort, layoverHours, portCharge,
                                    arrivingRoute.date, arrivingRoute.arrivalTime,
                                    departingRoute.date, departingRoute.departureTime);
                result.layovers.push_back(layover);
            }

            cout << "Optimal Path (Shortest): ";
            for (int i = 0; i < result.path.getSize(); i++)
            {
                cout << result.path.get(i);
                if (i < result.path.getSize() - 1)
                    cout << " -> ";
            }
            cout << endl;

            cout << "\nRoute Details:" << endl;
            int routeCost = 0;
            for (int i = 0; i < result.routes.getSize(); i++)
            {
                const Route &route = result.routes.get(i);
                routeCost += route.cost;
                cout << "  " << (i + 1) << ". " << route.origin << " -> " << route.destination
                     << " (Cost: $" << route.cost << ", " << route.date
                     << " " << route.departureTime << "-" << route.arrivalTime << ")" << endl;

                if (i < result.layovers.getSize())
                {
                    const LayoverInfo &layover = result.layovers.get(i);
                    cout << "     Docking at " << layover.portName << " for " << layover.layoverHours
                         << " hours (Arrived: " << layover.arrivalDate << " " << layover.arrivalTime
                         << ", Departed: " << layover.departureDate << " " << layover.departureTime << ")";
                    if (layover.layoverHours > 12)
                    {
                        cout << " [Port Charge: $" << layover.portCharge << " (>12h layover)]";
                    }
                    else
                    {
                        cout << " [No port charge (≤12h layover)]";
                    }
                    cout << endl;
                }
            }

            cout << "\nCost Breakdown:" << endl;
            cout << "  Route Costs: $" << routeCost << endl;
            int totalPortCharges = 0;
            for (int i = 0; i < result.layovers.getSize(); i++)
            {
                totalPortCharges += result.layovers.get(i).portCharge;
            }
            if (totalPortCharges > 0)
            {
                cout << "  Port Charges: $" << totalPortCharges << endl;
            }
            else
            {
                cout << "  Port Charges: $0 (no layovers > 12 hours)" << endl;
            }
            cout << "  Total Cost: $" << result.totalCost << " (Hops: " << hops[destIdx] << ")" << endl;

            // Calculate total travel time (route times + layover times)
            int totalTravelHours = 0;
            for (int i = 0; i < result.routes.getSize(); i++)
            {
                const Route &route = result.routes.get(i);
                // Each route takes approximately 24 hours (1 day travel)
                totalTravelHours += 24;
            }
            // Add layover hours
            for (int i = 0; i < result.layovers.getSize(); i++)
            {
                totalTravelHours += result.layovers.get(i).layoverHours;
            }
            result.totalTravelTime = totalTravelHours;
            cout << "  Total Travel Time: " << totalTravelHours << " hours ("
                 << (totalTravelHours / 24) << " days " << (totalTravelHours % 24) << " hours)" << endl;
        }
        else
        {
            cout << "No path found to destination with given preferences!" << endl;
        }

        delete[] hops;
        delete[] parent;
        delete[] visited;
        delete[] totalCost;
        delete[] arrivalDates;
        delete[] arrivalTimes;

        return result;
    }
};

#endif
