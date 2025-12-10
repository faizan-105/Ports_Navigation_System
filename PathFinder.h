
#ifndef PATHFINDER_H
#define PATHFINDER_H

#include "Graph.h"
#include "MinHeap.h"
#include "LinkedList.h"
#include "PortMapper.h"
#include "PreferenceFilter.h"
#include <string>
#include <limits>
#include <iostream>
using namespace std;
struct LayoverInfo
{
    string portName;
    int layoverHours;     // Total hours docked (including waiting)
    int portCharge;       // Charge only if > 12 hours
    string arrivalDate;   // Arrival date (DD/MM/YYYY)
    string arrivalTime;   // Arrival time (HH:MM)
    string departureDate; // Departure date (DD/MM/YYYY)
    string departureTime; // Departure time (HH:MM)

    LayoverInfo() : portName(""), layoverHours(0), portCharge(0),
                    arrivalDate(""), arrivalTime(""), departureDate(""), departureTime("") {}
    LayoverInfo(const string &port, int hours, int charge,
                const string &arrDate, const string &arr, const string &depDate, const string &dep)
        : portName(port), layoverHours(hours), portCharge(charge),
          arrivalDate(arrDate), arrivalTime(arr), departureDate(depDate), departureTime(dep) {}
};

struct PathResult
{
    bool found;
    int totalCost;
    int totalTravelTime; // NEW: Total time in hours (travel + layover)
    LinkedList<string> path;
    LinkedList<Route> routes;
    LinkedList<LayoverInfo> layovers; // Store layover information

    PathResult() : found(false), totalCost(0), totalTravelTime(0) {}
};

class PathFinder
{
private:
    Graph *graph;

    // Helper to check if port is already in current path (avoid cycles)
    bool isPortInPath(const LinkedList<string> &path, const string &port)
    {
        for (int i = 0; i < path.getSize(); i++)
        {
            if (path.get(i) == port)
            {
                return true;
            }
        }
        return false;
    }

    // DFS to find ALL possible paths from origin to destination
    void findAllPathsDFS(const string &current,
                         const string &destination,
                         const string &date,
                         LinkedList<string> &currentPath,
                         LinkedList<LinkedList<string>> &allPaths,
                         int maxDepth = 10)
    {

        // Add current port to path
        currentPath.push_back(current);

        // Depth limit to prevent infinite recursion
        if (currentPath.getSize() > maxDepth)
        {
            currentPath.remove(currentPath.getSize() - 1);
            return;
        }

        // If reached destination, save this path
        if (current == destination)
        {
            allPaths.push_back(currentPath);
            currentPath.remove(currentPath.getSize() - 1);
            return;
        }

        // Get all routes from current port on the given date
        LinkedList<Route> routes = graph->getRoutesFromOnDate(current, date);

        // Try each possible next destination
        for (int i = 0; i < routes.getSize(); i++)
        {
            const Route &route = routes.get(i);

            // Only visit if not already in current path (avoid cycles)
            if (!isPortInPath(currentPath, route.destination))
            {
                findAllPathsDFS(route.destination, destination, date,
                                currentPath, allPaths, maxDepth);
            }
        }

        // Backtrack
        currentPath.remove(currentPath.getSize() - 1);
    }

    // Calculate total cost of a path
    int calculatePathCost(const LinkedList<string> &path, const string &date)
    {
        int totalCost = 0;

        for (int i = 0; i < path.getSize() - 1; i++)
        {
            string from = path.get(i);
            string to = path.get(i + 1);

            LinkedList<Route> routes = graph->getRoutesFromOnDate(from, date);

            // Find the route from 'from' to 'to'
            bool found = false;
            for (int j = 0; j < routes.getSize(); j++)
            {
                if (routes.get(j).destination == to)
                {
                    totalCost += routes.get(j).cost;
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                return numeric_limits<int>::max(); // Invalid path
            }
        }

        return totalCost;
    }

public:
    PathFinder(Graph *g) : graph(g) {}

    // NEW METHOD: Find all possible paths (for visualization)
    LinkedList<LinkedList<string>> findAllPaths(const string &origin,
                                                const string &destination,
                                                const string &date)
    {

        LinkedList<LinkedList<string>> allPaths;
        LinkedList<string> currentPath;

        cout << "\n=== Finding ALL possible paths ===" << endl;
        cout << "Origin: " << origin << endl;
        cout << "Destination: " << destination << endl;
        cout << "Date: " << date << endl;

        findAllPathsDFS(origin, destination, date, currentPath, allPaths);

        cout << "Found " << allPaths.getSize() << " total paths" << endl;

        // Print all paths
        for (int i = 0; i < allPaths.getSize(); i++)
        {
            cout << "Path " << (i + 1) << ": ";
            const LinkedList<string> &path = allPaths.get(i);
            for (int j = 0; j < path.getSize(); j++)
            {
                cout << path.get(j);
                if (j < path.getSize() - 1)
                    cout << " -> ";
            }
            int cost = calculatePathCost(path, date);
            cout << " (Route Cost: $" << cost;
            cout << " - Note: Port charges for layovers > 12h not included)" << endl;
        }

        return allPaths;
    }

    // IMPROVED METHOD: Find cheapest path (Dijkstra's algorithm)
    PathResult findCheapestPath(const string &origin,
                                const string &destination,
                                const string &date)
    {
        PathResult result;

        cout << "\n=== Finding CHEAPEST path using Dijkstra ===" << endl;

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

        // Arrays for Dijkstra with time-based routing
        int *distances = new int[numPorts];
        int *parent = new int[numPorts];
        bool *visited = new bool[numPorts];
        string *arrivalDates = new string[numPorts]; // Track arrival date at each port
        string *arrivalTimes = new string[numPorts]; // Track arrival time at each port

        for (int i = 0; i < numPorts; i++)
        {
            distances[i] = numeric_limits<int>::max();
            parent[i] = -1;
            visited[i] = false;
            arrivalDates[i] = "";
            arrivalTimes[i] = "";
        }

        distances[originIdx] = 0;
        arrivalDates[originIdx] = date;
        arrivalTimes[originIdx] = "00:00"; // Start at beginning of day

        cout << "Running Dijkstra's algorithm with time-based routing..." << endl;

        // Dijkstra's algorithm with time validation
        for (int count = 0; count < numPorts; count++)
        {
            // Find minimum distance unvisited vertex
            int minDist = numeric_limits<int>::max();
            int minIdx = -1;

            for (int i = 0; i < numPorts; i++)
            {
                if (!visited[i] && distances[i] < minDist)
                {
                    minDist = distances[i];
                    minIdx = i;
                }
            }

            if (minIdx == -1)
                break; // No more reachable vertices

            visited[minIdx] = true;

            // Found destination
            if (minIdx == destIdx)
            {
                cout << "FOUND OPTIMAL PATH TO DESTINATION!" << endl;
                result.found = true;
                result.totalCost = distances[destIdx];
                break;
            }

            // Update neighbors with time-based validation
            string currentPort = portMapper.getName(minIdx);
            string currentArrivalDate = arrivalDates[minIdx];
            string currentArrivalTime = arrivalTimes[minIdx];

            // Get connecting routes (same day + future days) that are time-compatible
            LinkedList<Route> connectingRoutes = graph->getConnectingRoutes(
                currentPort, currentArrivalDate, currentArrivalTime);

            for (int i = 0; i < connectingRoutes.getSize(); i++)
            {
                const Route &route = connectingRoutes.get(i);
                int neighborIdx = portMapper.findIndex(route.destination);

                if (neighborIdx == -1 || visited[neighborIdx])
                    continue;

                // Calculate layover hours and validate connection
                int layoverHours = 0;
                bool connectionValid = true;

                // Find and validate the previous route if exists
                if (parent[minIdx] != -1)
                {
                    string fromPort = portMapper.getName(parent[minIdx]);
                    string toPort = currentPort;
                    string prevArrivalDate = arrivalDates[minIdx];
                    string prevArrivalTime = arrivalTimes[minIdx];

                    // Find the route that was used to reach currentPort
                    LinkedList<Route> prevRoutes = graph->getConnectingRoutes(fromPort, arrivalDates[parent[minIdx]], arrivalTimes[parent[minIdx]]);
                    for (int j = 0; j < prevRoutes.getSize(); j++)
                    {
                        if (prevRoutes.get(j).destination == toPort &&
                            prevRoutes.get(j).date == prevArrivalDate)
                        {
                            // Validate connection inline while prevRoutes is still in scope
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

                // Skip if connection is invalid
                if (!connectionValid)
                {
                    continue;
                }

                // Apply port charge only for layovers > 12 hours
                int portCharge = 0;
                if (layoverHours > 12)
                {
                    Port currentPortInfo;
                    if (graph->getPort(currentPort, currentPortInfo))
                    {
                        // Calculate number of days (round up)
                        int days = (layoverHours + 11) / 24; // Round up to nearest day
                        if (days == 0)
                            days = 1; // At least 1 day for layover > 12 hours
                        portCharge = currentPortInfo.dailyCharge * days;
                    }
                }

                // Calculate total cost: route cost + port charge (only for layovers > 12h)
                int newDist = distances[minIdx] + route.cost + portCharge;
                if (newDist < distances[neighborIdx])
                {
                    distances[neighborIdx] = newDist;
                    parent[neighborIdx] = minIdx;
                    arrivalDates[neighborIdx] = route.date;
                    arrivalTimes[neighborIdx] = route.arrivalTime;
                }
            }
        }

        // Reconstruct path using stored routes
        if (result.found)
        {
            cout << "Reconstructing optimal path..." << endl;

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

                // The layover happens at the destination of arrivingRoute (which is origin of departingRoute)
                string layoverPort = arrivingRoute.destination;

                int layoverHours = Route::calculateLayoverHours(arrivingRoute, departingRoute);

                // Calculate port charge if layover > 12 hours
                int portCharge = 0;
                if (layoverHours > 12)
                {
                    Port portInfo;
                    if (graph->getPort(layoverPort, portInfo))
                    {
                        int days = (layoverHours + 11) / 24; // Round up to nearest day
                        if (days == 0)
                            days = 1; // At least 1 day for layover > 12 hours
                        portCharge = portInfo.dailyCharge * days;
                    }
                }

                LayoverInfo layover(layoverPort, layoverHours, portCharge,
                                    arrivingRoute.date, arrivingRoute.arrivalTime,
                                    departingRoute.date, departingRoute.departureTime);
                result.layovers.push_back(layover);
            }

            cout << "Optimal Path: ";
            for (int i = 0; i < result.path.getSize(); i++)
            {
                cout << result.path.get(i);
                if (i < result.path.getSize() - 1)
                    cout << " -> ";
            }
            cout << endl;

            // Display route details with layover information
            cout << "\nRoute Details:" << endl;
            int routeCost = 0;
            for (int i = 0; i < result.routes.getSize(); i++)
            {
                const Route &route = result.routes.get(i);
                routeCost += route.cost;
                cout << "  " << (i + 1) << ". " << route.origin << " -> " << route.destination
                     << " (Cost: $" << route.cost << ", " << route.date
                     << " " << route.departureTime << "-" << route.arrivalTime << ")" << endl;

                // Show layover information after each route (except the last one)
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
            cout << "  Total Cost: $" << result.totalCost << endl;

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

        delete[] distances;
        delete[] parent;
        delete[] visited;
        delete[] arrivalDates;
        delete[] arrivalTimes;

        return result;
    }

    bool hasRoutesOnDate(const string &origin, const string &date)
    {
        LinkedList<Route> routes = graph->getRoutesFromOnDate(origin, date);
        return routes.getSize() > 0;
    }

    // Find cheapest path with user preferences (Dijkstra's algorithm with filtering)
    PathResult findCheapestPathWithPreferences(const string &origin,
                                               const string &destination,
                                               const string &date,
                                               const PreferenceFilter &preferences)
    {
        PathResult result;

        cout << "\n=== Finding CHEAPEST path with PREFERENCES using Dijkstra ===" << endl;

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

        // Arrays for Dijkstra with time-based routing and preferences
        int *distances = new int[numPorts];
        int *parent = new int[numPorts];
        bool *visited = new bool[numPorts];
        string *arrivalDates = new string[numPorts];
        string *arrivalTimes = new string[numPorts];

        for (int i = 0; i < numPorts; i++)
        {
            distances[i] = numeric_limits<int>::max();
            parent[i] = -1;
            visited[i] = false;
            arrivalDates[i] = "";
            arrivalTimes[i] = "";
        }

        distances[originIdx] = 0;
        arrivalDates[originIdx] = date;
        arrivalTimes[originIdx] = "00:00";

        cout << "Running Dijkstra's algorithm with preference filtering and time-based routing..." << endl;

        // Dijkstra's algorithm with preference filtering and time validation
        for (int count = 0; count < numPorts; count++)
        {
            // Find minimum distance unvisited vertex
            int minDist = numeric_limits<int>::max();
            int minIdx = -1;

            for (int i = 0; i < numPorts; i++)
            {
                if (!visited[i] && distances[i] < minDist)
                {
                    minDist = distances[i];
                    minIdx = i;
                }
            }

            if (minIdx == -1)
                break; // No more reachable vertices

            visited[minIdx] = true;

            // Found destination
            if (minIdx == destIdx)
            {
                cout << "FOUND OPTIMAL PATH TO DESTINATION!" << endl;
                result.found = true;
                result.totalCost = distances[destIdx];
                break;
            }

            // Update neighbors with preference filtering and time validation
            string currentPort = portMapper.getName(minIdx);
            string currentArrivalDate = arrivalDates[minIdx];
            string currentArrivalTime = arrivalTimes[minIdx];

            // Get connecting routes (same day + next day) that are time-compatible
            LinkedList<Route> connectingRoutes = graph->getConnectingRoutes(
                currentPort, currentArrivalDate, currentArrivalTime);

            for (int i = 0; i < connectingRoutes.getSize(); i++)
            {
                const Route &route = connectingRoutes.get(i);

                // Apply preference filter
                if (!preferences.matchesRoute(route))
                {
                    continue; // Skip routes that don't match preferences
                }

                int neighborIdx = portMapper.findIndex(route.destination);

                if (neighborIdx == -1 || visited[neighborIdx])
                    continue;

                // Check if neighbor is excluded
                if (preferences.hasPortPreference)
                {
                    bool isExcluded = false;
                    for (int j = 0; j < preferences.excludedPorts.getSize(); j++)
                    {
                        if (portMapper.getName(neighborIdx) == preferences.excludedPorts.get(j))
                        {
                            isExcluded = true;
                            break;
                        }
                    }
                    if (isExcluded)
                        continue;
                }

                // Calculate layover hours and validate connection
                int layoverHours = 0;
                bool connectionValid = true;

                // Find and validate the previous route if exists
                if (parent[minIdx] != -1)
                {
                    string fromPort = portMapper.getName(parent[minIdx]);
                    string toPort = currentPort;
                    string prevArrivalDate = arrivalDates[minIdx];
                    string prevArrivalTime = arrivalTimes[minIdx];

                    // Find the route that was used to reach currentPort
                    LinkedList<Route> prevRoutes = graph->getConnectingRoutes(fromPort, arrivalDates[parent[minIdx]], arrivalTimes[parent[minIdx]]);
                    for (int j = 0; j < prevRoutes.getSize(); j++)
                    {
                        if (prevRoutes.get(j).destination == toPort &&
                            prevRoutes.get(j).date == prevArrivalDate)
                        {
                            // Validate connection inline while prevRoutes is still in scope
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

                // Skip if connection is invalid
                if (!connectionValid)
                {
                    continue;
                }

                // Apply port charge only for layovers > 12 hours
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

                // Calculate total cost: route cost + port charge (only for layovers > 12h)
                int newDist = distances[minIdx] + route.cost + portCharge;
                if (newDist < distances[neighborIdx])
                {
                    distances[neighborIdx] = newDist;
                    parent[neighborIdx] = minIdx;
                    arrivalDates[neighborIdx] = route.date;
                    arrivalTimes[neighborIdx] = route.arrivalTime;
                }
            }
        }

        // Reconstruct path
        if (result.found)
        {
            cout << "Reconstructing optimal path..." << endl;

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

            // Check if path includes required ports
            if (preferences.hasPortPreference && preferences.requiredPorts.getSize() > 0)
            {
                if (!preferences.pathMatchesPorts(result.path))
                {
                    cout << "WARNING: Path does not include all required ports!" << endl;
                    // Still return the path, but mark it as potentially incomplete
                }
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

            // Check voyage time limit
            if (preferences.hasTimeLimit && !preferences.isVoyageTimeValid(result.routes))
            {
                cout << "WARNING: Voyage time exceeds maximum limit!" << endl;
                // Still return the path, but user should be notified
            }

            // Calculate and store layover information
            for (int i = 0; i < result.routes.getSize() - 1; i++)
            {
                const Route &arrivingRoute = result.routes.get(i);
                const Route &departingRoute = result.routes.get(i + 1);

                // The layover happens at the destination of arrivingRoute (which is origin of departingRoute)
                string layoverPort = arrivingRoute.destination;

                int layoverHours = Route::calculateLayoverHours(arrivingRoute, departingRoute);

                // Calculate port charge if layover > 12 hours
                int portCharge = 0;
                if (layoverHours > 12)
                {
                    Port portInfo;
                    if (graph->getPort(layoverPort, portInfo))
                    {
                        int days = (layoverHours + 11) / 24; // Round up to nearest day
                        if (days == 0)
                            days = 1; // At least 1 day for layover > 12 hours
                        portCharge = portInfo.dailyCharge * days;
                    }
                }

                LayoverInfo layover(layoverPort, layoverHours, portCharge,
                                    arrivingRoute.date, arrivingRoute.arrivalTime,
                                    departingRoute.date, departingRoute.departureTime);
                result.layovers.push_back(layover);
            }

            cout << "Optimal Path: ";
            for (int i = 0; i < result.path.getSize(); i++)
            {
                cout << result.path.get(i);
                if (i < result.path.getSize() - 1)
                    cout << " -> ";
            }
            cout << endl;

            // Display route details with layover information
            cout << "\nRoute Details:" << endl;
            int routeCost = 0;
            for (int i = 0; i < result.routes.getSize(); i++)
            {
                const Route &route = result.routes.get(i);
                routeCost += route.cost;
                cout << "  " << (i + 1) << ". " << route.origin << " -> " << route.destination
                     << " (Cost: $" << route.cost << ", " << route.date
                     << " " << route.departureTime << "-" << route.arrivalTime << ")" << endl;

                // Show layover information after each route (except the last one)
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
            cout << "  Total Cost: $" << result.totalCost << endl;

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

        delete[] distances;
        delete[] parent;
        delete[] visited;
        delete[] arrivalDates;
        delete[] arrivalTimes;

        return result;
    }

    // Get all connecting routes that match preferences
    LinkedList<Route> getAllConnectingRoutesWithPreferences(const string &origin,
                                                            const string &destination,
                                                            const string &date,
                                                            const PreferenceFilter &preferences)
    {
        LinkedList<Route> connectingRoutes;

        cout << "\n=== Getting ALL connecting routes with PREFERENCES ===" << endl;
        cout << "From: " << origin << " To: " << destination << endl;

        // Build port mapper
        PortMapper portMapper;
        LinkedList<Port> allPorts = graph->getAllPorts();

        for (int i = 0; i < allPorts.getSize(); i++)
        {
            portMapper.addPort(allPorts.get(i).name);
        }

        int originIdx = portMapper.findIndex(origin);
        int destIdx = portMapper.findIndex(destination);

        if (originIdx == -1)
        {
            cout << "ERROR: Origin not found!" << endl;
            return connectingRoutes;
        }
        if (destIdx == -1)
        {
            cout << "ERROR: Destination not found!" << endl;
            return connectingRoutes;
        }

        int numPorts = portMapper.getSize();

        // BFS from destination backwards to find all ports that can reach destination
        Queue<int> queue;
        bool *canReachDest = new bool[numPorts];
        bool *visited = new bool[numPorts];

        for (int i = 0; i < numPorts; i++)
        {
            canReachDest[i] = false;
            visited[i] = false;
        }

        // Start BFS from destination (reverse direction)
        queue.enqueue(destIdx);
        visited[destIdx] = true;
        canReachDest[destIdx] = true;

        while (!queue.isEmpty())
        {
            int currentIdx = queue.getFront();
            queue.dequeue();

            string currentPort = portMapper.getName(currentIdx);

            // Find all ports that have routes TO current port (includes multi-day routes)
            for (int i = 0; i < allPorts.getSize(); i++)
            {
                string potentialPort = allPorts.get(i).name;
                // Get all routes from potential port (includes multi-day routes)
                LinkedList<Route> routes = graph->getRoutesFrom(potentialPort);

                for (int j = 0; j < routes.getSize(); j++)
                {
                    const Route &route = routes.get(j);

                    // Apply preference filter
                    if (!preferences.matchesRoute(route))
                    {
                        continue;
                    }

                    if (route.destination == currentPort)
                    {
                        int potentialIdx = portMapper.findIndex(potentialPort);
                        if (potentialIdx != -1 && !visited[potentialIdx])
                        {
                            visited[potentialIdx] = true;
                            canReachDest[potentialIdx] = true;
                            queue.enqueue(potentialIdx);
                        }
                    }
                }
            }
        }

        // Now BFS from origin to collect routes that can eventually reach destination
        bool *originVisited = new bool[numPorts];
        for (int i = 0; i < numPorts; i++)
        {
            originVisited[i] = false;
        }

        Queue<int> originQueue;
        originQueue.enqueue(originIdx);
        originVisited[originIdx] = true;

        while (!originQueue.isEmpty())
        {
            int currentIdx = originQueue.getFront();
            originQueue.dequeue();

            string currentPort = portMapper.getName(currentIdx);
            // Get all routes from current port (includes multi-day routes)
            LinkedList<Route> routes = graph->getRoutesFrom(currentPort);

            for (int i = 0; i < routes.getSize(); i++)
            {
                const Route &route = routes.get(i);

                // Apply preference filter
                if (!preferences.matchesRoute(route))
                {
                    continue;
                }

                int routeDestIdx = portMapper.findIndex(route.destination);

                // Only include routes that lead to ports that can reach destination
                if (routeDestIdx != -1 && canReachDest[routeDestIdx])
                {
                    // Check if already added (avoid duplicates)
                    bool found = false;
                    for (int j = 0; j < connectingRoutes.getSize(); j++)
                    {
                        const Route &existing = connectingRoutes.get(j);
                        if (existing.origin == route.origin &&
                            existing.destination == route.destination &&
                            existing.date == route.date &&
                            existing.departureTime == route.departureTime)
                        {
                            found = true;
                            break;
                        }
                    }

                    if (!found)
                    {
                        connectingRoutes.push_back(route);
                    }

                    if (!originVisited[routeDestIdx])
                    {
                        originVisited[routeDestIdx] = true;
                        originQueue.enqueue(routeDestIdx);
                    }
                }
            }
        }

        cout << "Found " << connectingRoutes.getSize()
             << " connecting routes that match preferences and can reach " << destination << endl;

        delete[] canReachDest;
        delete[] visited;
        delete[] originVisited;

        return connectingRoutes;
    }

    // Get all connecting routes that can eventually reach destination
    LinkedList<Route> getAllConnectingRoutes(const string &origin,
                                             const string &destination,
                                             const string &date)
    {
        LinkedList<Route> connectingRoutes;

        cout << "\n=== Getting ALL connecting routes ===" << endl;
        cout << "From: " << origin << " To: " << destination << endl;

        // Build port mapper
        PortMapper portMapper;
        LinkedList<Port> allPorts = graph->getAllPorts();

        for (int i = 0; i < allPorts.getSize(); i++)
        {
            portMapper.addPort(allPorts.get(i).name);
        }

        int originIdx = portMapper.findIndex(origin);
        int destIdx = portMapper.findIndex(destination);

        if (originIdx == -1)
        {
            cout << "ERROR: Origin not found!" << endl;
            return connectingRoutes;
        }
        if (destIdx == -1)
        {
            cout << "ERROR: Destination not found!" << endl;
            return connectingRoutes;
        }

        int numPorts = portMapper.getSize();

        // BFS from destination backwards to find all ports that can reach destination
        Queue<int> queue;
        bool *canReachDest = new bool[numPorts];
        bool *visited = new bool[numPorts];

        for (int i = 0; i < numPorts; i++)
        {
            canReachDest[i] = false;
            visited[i] = false;
        }

        // Start BFS from destination (reverse direction)
        queue.enqueue(destIdx);
        visited[destIdx] = true;
        canReachDest[destIdx] = true;

        while (!queue.isEmpty())
        {
            int currentIdx = queue.getFront();
            queue.dequeue();

            string currentPort = portMapper.getName(currentIdx);

            // Find all ports that have routes TO current port
            for (int i = 0; i < allPorts.getSize(); i++)
            {
                string potentialPort = allPorts.get(i).name;
                // Get all routes from potential port (includes multi-day routes)
                LinkedList<Route> allFromPotential = graph->getRoutesFrom(potentialPort);

                for (int j = 0; j < allFromPotential.getSize(); j++)
                {
                    const Route &route = allFromPotential.get(j);
                    // Check if this route leads to current port
                    if (route.destination == currentPort)
                    {
                        int potentialIdx = portMapper.findIndex(potentialPort);
                        if (potentialIdx != -1 && !visited[potentialIdx])
                        {
                            visited[potentialIdx] = true;
                            canReachDest[potentialIdx] = true;
                            queue.enqueue(potentialIdx);
                        }
                    }
                }
            }
        }

        // Now BFS from origin to collect routes that can eventually reach destination
        bool *originVisited = new bool[numPorts];
        for (int i = 0; i < numPorts; i++)
        {
            originVisited[i] = false;
        }

        Queue<int> originQueue;
        originQueue.enqueue(originIdx);
        originVisited[originIdx] = true;

        while (!originQueue.isEmpty())
        {
            int currentIdx = originQueue.getFront();
            originQueue.dequeue();

            string currentPort = portMapper.getName(currentIdx);
            // Get all routes from current port (includes multi-day routes)
            LinkedList<Route> allRoutes = graph->getRoutesFrom(currentPort);

            for (int i = 0; i < allRoutes.getSize(); i++)
            {
                const Route &route = allRoutes.get(i);
                int routeDestIdx = portMapper.findIndex(route.destination);

                // Only include routes that lead to ports that can reach destination
                if (routeDestIdx != -1 && canReachDest[routeDestIdx])
                {
                    // Check if already added (avoid duplicates)
                    bool found = false;
                    for (int j = 0; j < connectingRoutes.getSize(); j++)
                    {
                        const Route &existing = connectingRoutes.get(j);
                        if (existing.origin == route.origin &&
                            existing.destination == route.destination &&
                            existing.date == route.date &&
                            existing.departureTime == route.departureTime)
                        {
                            found = true;
                            break;
                        }
                    }

                    if (!found)
                    {
                        connectingRoutes.push_back(route);
                    }

                    if (!originVisited[routeDestIdx])
                    {
                        originVisited[routeDestIdx] = true;
                        originQueue.enqueue(routeDestIdx);
                    }
                }
            }
        }

        cout << "Found " << connectingRoutes.getSize()
             << " connecting routes that can reach " << destination << endl;

        delete[] canReachDest;
        delete[] visited;
        delete[] originVisited;

        return connectingRoutes;
    }

    // Find multi-leg route connecting origin -> intermediate ports -> destination
    PathResult findMultiLegRoute(const string &origin,
                                 const LinkedList<string> &intermediatePorts,
                                 const string &destination,
                                 const string &date)
    {
        PathResult result;

        cout << "\n=== Finding MULTI-LEG ROUTE ===" << endl;
        cout << "Origin: " << origin << endl;
        for (int i = 0; i < intermediatePorts.getSize(); i++)
        {
            cout << "Intermediate " << (i + 1) << ": " << intermediatePorts.get(i) << endl;
        }
        cout << "Destination: " << destination << endl;
        cout << "Date: " << date << endl;

        // Validate all ports exist
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
        for (int i = 0; i < intermediatePorts.getSize(); i++)
        {
            if (!graph->hasPort(intermediatePorts.get(i)))
            {
                cout << "ERROR: Intermediate port '" << intermediatePorts.get(i) << "' not found!" << endl;
                return result;
            }
        }

        // Build complete path sequence
        LinkedList<string> fullPath;
        fullPath.push_back(origin);
        for (int i = 0; i < intermediatePorts.getSize(); i++)
        {
            fullPath.push_back(intermediatePorts.get(i));
        }
        fullPath.push_back(destination);

        // Find route for each leg
        result.found = true;
        result.totalCost = 0;

        for (int i = 0; i < fullPath.getSize() - 1; i++)
        {
            string from = fullPath.get(i);
            string to = fullPath.get(i + 1);

            cout << "Finding route from " << from << " to " << to << "..." << endl;

            // Use Dijkstra to find cheapest path for this leg
            PathResult legResult = findCheapestPath(from, to, date);

            if (!legResult.found)
            {
                cout << "ERROR: No route found from " << from << " to " << to << "!" << endl;
                result.found = false;
                result.totalCost = 0;
                result.path.clear();
                result.routes.clear();
                return result;
            }

            // Calculate layover and apply port charge only if layover > 12 hours
            // (Note: findCheapestPath now handles port charges internally for layovers > 12h)
            // This section is kept for compatibility but port charges are handled in findCheapestPath
            // No additional port charge needed here as it's already calculated in the leg pathfinding

            // Add leg path to result (skip first port of each leg to avoid duplicates)
            if (i == 0)
            {
                // First leg: add all ports
                for (int j = 0; j < legResult.path.getSize(); j++)
                {
                    result.path.push_back(legResult.path.get(j));
                }
            }
            else
            {
                // Subsequent legs: skip first port (already in path)
                for (int j = 1; j < legResult.path.getSize(); j++)
                {
                    result.path.push_back(legResult.path.get(j));
                }
            }

            // Add all routes from this leg
            for (int j = 0; j < legResult.routes.getSize(); j++)
            {
                result.routes.push_back(legResult.routes.get(j));
            }

            result.totalCost += legResult.totalCost;
        }

        cout << "Multi-leg route found!" << endl;
        cout << "Complete path: ";
        for (int i = 0; i < result.path.getSize(); i++)
        {
            cout << result.path.get(i);
            if (i < result.path.getSize() - 1)
                cout << " -> ";
        }
        cout << endl;
        cout << "Total Cost: $" << result.totalCost << endl;
        cout << "Total Legs: " << (fullPath.getSize() - 1) << endl;

        return result;
    }

    // Bidirectional Dijkstra search for efficiency
    PathResult findCheapestPathBidirectional(const string &origin,
                                             const string &destination,
                                             const string &date)
    {
        PathResult result;

        cout << "\n=== Finding CHEAPEST path using BIDIRECTIONAL Dijkstra ===" << endl;

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
        int originIdx = portMapper.findIndex(origin);
        int destIdx = portMapper.findIndex(destination);

        if (originIdx == -1 || destIdx == -1)
        {
            cout << "ERROR: Could not find port indices!" << endl;
            return result;
        }

        // Forward search from origin
        int *forwardDist = new int[numPorts];
        int *forwardParent = new int[numPorts];
        bool *forwardVisited = new bool[numPorts];
        string *forwardArrivalDates = new string[numPorts];
        string *forwardArrivalTimes = new string[numPorts];

        // Backward search from destination
        int *backwardDist = new int[numPorts];
        int *backwardParent = new int[numPorts];
        bool *backwardVisited = new bool[numPorts];

        for (int i = 0; i < numPorts; i++)
        {
            forwardDist[i] = numeric_limits<int>::max();
            forwardParent[i] = -1;
            forwardVisited[i] = false;
            forwardArrivalDates[i] = "";
            forwardArrivalTimes[i] = "";
            backwardDist[i] = numeric_limits<int>::max();
            backwardParent[i] = -1;
            backwardVisited[i] = false;
        }

        forwardDist[originIdx] = 0;
        forwardArrivalDates[originIdx] = date;
        forwardArrivalTimes[originIdx] = "00:00";
        backwardDist[destIdx] = 0;

        int meetingPoint = -1;
        int bestDistance = numeric_limits<int>::max();

        cout << "Running bidirectional Dijkstra..." << endl;

        // Alternate between forward and backward search
        for (int iteration = 0; iteration < numPorts * 2; iteration++)
        {
            // Forward search step
            int forwardMinDist = numeric_limits<int>::max();
            int forwardMinIdx = -1;
            for (int i = 0; i < numPorts; i++)
            {
                if (!forwardVisited[i] && forwardDist[i] < forwardMinDist)
                {
                    forwardMinDist = forwardDist[i];
                    forwardMinIdx = i;
                }
            }

            if (forwardMinIdx != -1)
            {
                forwardVisited[forwardMinIdx] = true;
                string currentPort = portMapper.getName(forwardMinIdx);
                string currentArrivalDate = forwardArrivalDates[forwardMinIdx];
                string currentArrivalTime = forwardArrivalTimes[forwardMinIdx];

                // Get connecting routes with proper time tracking
                LinkedList<Route> routes = graph->getConnectingRoutes(currentPort, currentArrivalDate, currentArrivalTime);

                for (int i = 0; i < routes.getSize(); i++)
                {
                    const Route &route = routes.get(i);
                    int neighborIdx = portMapper.findIndex(route.destination);
                    if (neighborIdx == -1 || forwardVisited[neighborIdx])
                        continue;

                    // Calculate layover hours and port charge
                    int layoverHours = 0;
                    int portCharge = 0;

                    if (forwardParent[forwardMinIdx] != -1)
                    {
                        // Find previous route to calculate layover
                        string fromPort = portMapper.getName(forwardParent[forwardMinIdx]);
                        string toPort = currentPort;
                        string prevArrivalDate = forwardArrivalDates[forwardMinIdx];
                        string prevArrivalTime = forwardArrivalTimes[forwardMinIdx];

                        LinkedList<Route> prevRoutes = graph->getConnectingRoutes(fromPort, forwardArrivalDates[forwardParent[forwardMinIdx]], forwardArrivalTimes[forwardParent[forwardMinIdx]]);
                        for (int j = 0; j < prevRoutes.getSize(); j++)
                        {
                            if (prevRoutes.get(j).destination == toPort && prevRoutes.get(j).date == prevArrivalDate)
                            {
                                const Route &previousRoute = prevRoutes.get(j);
                                layoverHours = Route::calculateLayoverHours(previousRoute, route);

                                // Calculate port charge if layover > 12 hours
                                if (layoverHours > 12)
                                {
                                    Port portInfo;
                                    if (graph->getPort(currentPort, portInfo))
                                    {
                                        int days = (layoverHours + 11) / 24;
                                        if (days == 0)
                                            days = 1;
                                        portCharge = portInfo.dailyCharge * days;
                                    }
                                }
                                break;
                            }
                        }
                    }

                    // Calculate total cost including port charges
                    int newDist = forwardDist[forwardMinIdx] + route.cost + portCharge;
                    if (newDist < forwardDist[neighborIdx])
                    {
                        forwardDist[neighborIdx] = newDist;
                        forwardParent[neighborIdx] = forwardMinIdx;
                        forwardArrivalDates[neighborIdx] = route.date;
                        forwardArrivalTimes[neighborIdx] = route.arrivalTime;
                    }

                    // Check if this node was visited by backward search
                    if (backwardVisited[neighborIdx])
                    {
                        int totalDist = forwardDist[neighborIdx] + backwardDist[neighborIdx];
                        if (totalDist < bestDistance)
                        {
                            bestDistance = totalDist;
                            meetingPoint = neighborIdx;
                        }
                    }
                }
            }

            // Backward search step
            int backwardMinDist = numeric_limits<int>::max();
            int backwardMinIdx = -1;
            for (int i = 0; i < numPorts; i++)
            {
                if (!backwardVisited[i] && backwardDist[i] < backwardMinDist)
                {
                    backwardMinDist = backwardDist[i];
                    backwardMinIdx = i;
                }
            }

            if (backwardMinIdx != -1)
            {
                backwardVisited[backwardMinIdx] = true;
                string currentPort = portMapper.getName(backwardMinIdx);

                // Find routes TO this port (reverse direction) - include all dates
                for (int i = 0; i < allPorts.getSize(); i++)
                {
                    string potentialPort = allPorts.get(i).name;
                    LinkedList<Route> routes = graph->getRoutesFrom(potentialPort);

                    for (int j = 0; j < routes.getSize(); j++)
                    {
                        const Route &route = routes.get(j);
                        if (route.destination == currentPort)
                        {
                            int fromIdx = portMapper.findIndex(potentialPort);
                            if (fromIdx == -1 || backwardVisited[fromIdx])
                                continue;

                            // Don't apply port charges during search - will calculate properly after path reconstruction
                            // Port charges should only apply for layovers > 12 hours, which requires knowing the full path
                            int newDist = backwardDist[backwardMinIdx] + route.cost;
                            if (newDist < backwardDist[fromIdx])
                            {
                                backwardDist[fromIdx] = newDist;
                                backwardParent[fromIdx] = backwardMinIdx;
                            }

                            // Check if this node was visited by forward search
                            if (forwardVisited[fromIdx])
                            {
                                int totalDist = forwardDist[fromIdx] + backwardDist[fromIdx];
                                if (totalDist < bestDistance)
                                {
                                    bestDistance = totalDist;
                                    meetingPoint = fromIdx;
                                }
                            }
                        }
                    }
                }
            }

            // Stop if we found a meeting point
            if (meetingPoint != -1 && bestDistance < numeric_limits<int>::max())
            {
                break;
            }

            // Stop if both searches exhausted
            if (forwardMinIdx == -1 && backwardMinIdx == -1)
            {
                break;
            }
        }

        // Reconstruct path if found
        if (meetingPoint != -1)
        {
            result.found = true;
            result.totalCost = bestDistance;

            // Reconstruct forward path
            LinkedList<int> forwardPath;
            int current = meetingPoint;
            while (current != -1)
            {
                forwardPath.push_back(current);
                current = forwardParent[current];
            }

            // Reconstruct backward path (excluding meeting point)
            LinkedList<int> backwardPath;
            current = backwardParent[meetingPoint];
            while (current != -1)
            {
                backwardPath.push_back(current);
                current = backwardParent[current];
            }

            // Combine paths
            for (int i = forwardPath.getSize() - 1; i >= 0; i--)
            {
                result.path.push_back(portMapper.getName(forwardPath.get(i)));
            }
            for (int i = 0; i < backwardPath.getSize(); i++)
            {
                result.path.push_back(portMapper.getName(backwardPath.get(i)));
            }

            // Get routes for the path - support multi-day waiting
            if (result.path.getSize() >= 2)
            {
                for (int i = 0; i < result.path.getSize() - 1; i++)
                {
                    string from = result.path.get(i);
                    string to = result.path.get(i + 1);

                    // For first leg, use input date
                    // For subsequent legs, use arrival date from previous route
                    string departureDate = (i == 0) ? date : result.routes.get(i - 1).date;
                    string departureTime = (i == 0) ? "00:00" : result.routes.get(i - 1).arrivalTime;

                    LinkedList<Route> connectingRoutes = graph->getConnectingRoutes(from, departureDate, departureTime);
                    for (int j = 0; j < connectingRoutes.getSize(); j++)
                    {
                        if (connectingRoutes.get(j).destination == to)
                        {
                            result.routes.push_back(connectingRoutes.get(j));
                            break;
                        }
                    }
                }
            }

            // Recalculate total cost properly with layover-based port charges
            // (The bestDistance from bidirectional search only includes route costs)
            int routeCostTotal = 0;
            for (int i = 0; i < result.routes.getSize(); i++)
            {
                routeCostTotal += result.routes.get(i).cost;
            }

            // Calculate layover-based port charges
            int totalPortCharges = 0;
            for (int i = 0; i < result.routes.getSize() - 1; i++)
            {
                const Route &arrivingRoute = result.routes.get(i);
                const Route &departingRoute = result.routes.get(i + 1);

                string layoverPort = arrivingRoute.destination;
                int layoverHours = Route::calculateLayoverHours(arrivingRoute, departingRoute);

                // Apply port charge only for layovers > 12 hours
                if (layoverHours > 12)
                {
                    Port portInfo;
                    if (graph->getPort(layoverPort, portInfo))
                    {
                        int days = (layoverHours + 11) / 24; // Round up to nearest day
                        if (days == 0)
                            days = 1; // At least 1 day for layover > 12 hours
                        totalPortCharges += portInfo.dailyCharge * days;
                    }
                }
            }

            result.totalCost = routeCostTotal + totalPortCharges;

            // Calculate and store layover information (same as standard Dijkstra)
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

            cout << "Bidirectional path found!" << endl;
            cout << "\nOptimal Path: ";
            for (int i = 0; i < result.path.getSize(); i++)
            {
                cout << result.path.get(i);
                if (i < result.path.getSize() - 1)
                    cout << " -> ";
            }
            cout << endl;

            // Display route details
            cout << "\nRoute Details:" << endl;
            for (int i = 0; i < result.routes.getSize(); i++)
            {
                const Route &route = result.routes.get(i);
                cout << "  " << (i + 1) << ". " << route.origin << " -> " << route.destination
                     << " (Cost: $" << route.cost << ", " << route.date
                     << " " << route.departureTime << "-" << route.arrivalTime << ")" << endl;

                // Show layover information after each route (except the last one)
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
            cout << "  Route Costs: $" << routeCostTotal << endl;
            if (totalPortCharges > 0)
            {
                cout << "  Port Charges: $" << totalPortCharges << endl;
            }
            else
            {
                cout << "  Port Charges: $0 (no layovers > 12 hours)" << endl;
            }
            cout << "  Total Cost: $" << result.totalCost << endl;

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
            cout << "No path found using bidirectional search!" << endl;
        }

        delete[] forwardDist;
        delete[] forwardParent;
        delete[] forwardVisited;
        delete[] forwardArrivalDates;
        delete[] forwardArrivalTimes;
        delete[] backwardDist;
        delete[] backwardParent;
        delete[] backwardVisited;

        return result;
    }
};

#endif