
#pragma once
#ifndef ROUTEPARSER_H
#define ROUTEPARSER_H

#include "Graph.h"
#include "HashTable.h"
#include <fstream>
#include <sstream>
using namespace std;
class RouteParser {
private:
    // Port coordinates for visual representation (you can adjust these)
    static void assignPortCoordinates(Port& port) {
        // These are approximate positions on a world map (adjust based on your map image)
        // Format: {x, y} where (0,0) is top-left

    //    if (port.name == "Karachi") { port.x = 550; port.y = 380; }
    //    else if (port.name == "Dubai") { port.x = 520; port.y = 370; }
    //    else if (port.name == "AbuDhabi") { port.x = 525; port.y = 365; }
    //    else if (port.name == "Doha") { port.x = 515; port.y = 375; }
    //    else if (port.name == "Jeddah") { port.x = 480; port.y = 350; }
    //    else if (port.name == "Mumbai") { port.x = 560; port.y = 400; }
    //    else if (port.name == "Colombo") { port.x = 580; port.y = 420; }
    //    else if (port.name == "Singapore") { port.x = 650; port.y = 450; }
    //    else if (port.name == "HongKong") { port.x = 700; port.y = 360; }
    //    else if (port.name == "Shanghai") { port.x = 720; port.y = 340; }
    //    else if (port.name == "Tokyo") { port.x = 760; port.y = 320; }
    //    else if (port.name == "Osaka") { port.x = 755; port.y = 330; }
    //    else if (port.name == "Busan") { port.x = 730; port.y = 330; }
    //    else if (port.name == "Manila") { port.x = 710; port.y = 400; }
    //    else if (port.name == "Jakarta") { port.x = 670; port.y = 470; }
    //    else if (port.name == "Sydney") { port.x = 810; port.y = 550; }
    //    else if (port.name == "Melbourne") { port.x = 790; port.y = 560; }
    //    else if (port.name == "CapeTown") { port.x = 420; port.y = 550; }
    //    else if (port.name == "Durban") { port.x = 440; port.y = 530; }
    //    else if (port.name == "PortLouis") { port.x = 520; port.y = 510; }
    //    else if (port.name == "Alexandria") { port.x = 450; port.y = 320; }
    //    else if (port.name == "Istanbul") { port.x = 450; port.y = 300; }
    //    else if (port.name == "Athens") { port.x = 440; port.y = 310; }
    //    else if (port.name == "Genoa") { port.x = 420; port.y = 290; }
    //    else if (port.name == "Marseille") { port.x = 410; port.y = 290; }
    //    else if (port.name == "Lisbon") { port.x = 360; port.y = 310; }
    //    else if (port.name == "Rotterdam") { port.x = 400; port.y = 270; }
    //    else if (port.name == "Hamburg") { port.x = 410; port.y = 265; }
    //    else if (port.name == "Antwerp") { port.x = 405; port.y = 270; }
    //    else if (port.name == "London") { port.x = 390; port.y = 270; }
    //    else if (port.name == "Dublin") { port.x = 370; port.y = 265; }
    //    else if (port.name == "Oslo") { port.x = 410; port.y = 250; }
    //    else if (port.name == "Stockholm") { port.x = 420; port.y = 250; }
    //    else if (port.name == "Helsinki") { port.x = 430; port.y = 245; }
    //    else if (port.name == "Copenhagen") { port.x = 415; port.y = 260; }
    //    else if (port.name == "LosAngeles") { port.x = 150; port.y = 340; }
    //    else if (port.name == "Vancouver") { port.x = 140; port.y = 300; }
    //    else if (port.name == "NewYork") { port.x = 250; port.y = 300; }
    //    else if (port.name == "Montreal") { port.x = 260; port.y = 290; }
    //    else if (port.name == "Chittagong") { port.x = 610; port.y = 360; }
    //    else { port.x = 400; port.y = 400; }  // Default position
    //}
        // --- AMERICAS ---
        if (port.name == "Vancouver") { port.x = 185; port.y = 170; }
        else if (port.name == "LosAngeles") { port.x = 205; port.y = 250; }
        else if (port.name == "Montreal") { port.x = 350; port.y = 190; }
        else if (port.name == "NewYork") { port.x = 350; port.y = 225; }
        else if (port.name == "Rio") { port.x = 460; port.y = 480; }  // NEW PORT
        else if (port.name == "BuenosAires") { port.x = 425; port.y = 540; }  // NEW PORT

        // --- EUROPE (EXPLODED VIEW) ---
        else if (port.name == "Lisbon") { port.x = 550; port.y = 235; }
        else if (port.name == "Dublin") { port.x = 560; port.y = 150; }
        else if (port.name == "London") { port.x = 590; port.y = 175; }
        else if (port.name == "Rotterdam") { port.x = 615; port.y = 155; }
        else if (port.name == "Antwerp") { port.x = 610; port.y = 195; }
        else if (port.name == "Hamburg") { port.x = 645; port.y = 165; }
        else if (port.name == "Oslo") { port.x = 630; port.y = 125; }
        else if (port.name == "Copenhagen") { port.x = 650; port.y = 145; }
        else if (port.name == "Stockholm") { port.x = 670; port.y = 125; }
        else if (port.name == "Helsinki") { port.x = 700; port.y = 120; }
        else if (port.name == "Marseille") { port.x = 620; port.y = 220; }
        else if (port.name == "Genoa") { port.x = 650; port.y = 215; }
        else if (port.name == "Athens") { port.x = 680; port.y = 240; }
        else if (port.name == "Istanbul") { port.x = 710; port.y = 210; }

        // --- MIDDLE EAST & WEST ASIA (EXPLODED) ---
        else if (port.name == "Alexandria") { port.x = 700; port.y = 275; }
        else if (port.name == "Jeddah") { port.x = 730; port.y = 315; }
        else if (port.name == "Doha") { port.x = 760; port.y = 280; }
        else if (port.name == "AbuDhabi") { port.x = 780; port.y = 310; }
        else if (port.name == "Dubai") { port.x = 805; port.y = 280; }
        else if (port.name == "Karachi") { port.x = 830; port.y = 275; }
        else if (port.name == "Mumbai") { port.x = 845; port.y = 330; }
        else if (port.name == "Colombo") { port.x = 880; port.y = 390; }

        // --- ASIA & PACIFIC ---
        else if (port.name == "Chittagong") { port.x = 920; port.y = 300; }
        else if (port.name == "Singapore") { port.x = 955; port.y = 400; }
        else if (port.name == "Jakarta") { port.x = 960; port.y = 450; }
        else if (port.name == "Manila") { port.x = 1020; port.y = 360; }
        else if (port.name == "HongKong") { port.x = 990; port.y = 315; }
        else if (port.name == "Shanghai") { port.x = 1020; port.y = 275; }
        else if (port.name == "Busan") { port.x = 1045; port.y = 245; }
        else if (port.name == "Osaka") { port.x = 1070; port.y = 255; }
        else if (port.name == "Tokyo") { port.x = 1095; port.y = 235; }

        // --- AFRICA & INDIAN OCEAN ---
        else if (port.name == "CapeTown") { port.x = 660; port.y = 540; }
        else if (port.name == "Durban") { port.x = 710; port.y = 525; }
        else if (port.name == "PortLouis") { port.x = 800; port.y = 490; }

        // --- AUSTRALIA ---
        else if (port.name == "Melbourne") { port.x = 1000; port.y = 570; }
        else if (port.name == "Sydney") { port.x = 1000; port.y = 550; }

        else { port.x = 0; port.y = 0; }  // Default position for unknown ports
    }
public:
    // Parse PortCharges.txt
    static HashTable<int> parsePortCharges(const string& filename) {
        HashTable<int> portCharges;
        ifstream file(filename);

        if (!file.is_open()) {
            return portCharges;
        }

        string line;
        while (getline(file, line)) {
            istringstream iss(line);
            string portName;
            int charge;

            if (iss >> portName >> charge) {
                portCharges.insert(portName, charge);
            }
        }

        file.close();
        return portCharges;
    }

    // Parse Routes.txt and build graph
    static void buildGraphFromFile(Graph& graph, const string& routeFile,
        const string& chargeFile) {
        // First, parse port charges
        HashTable<int> portCharges = parsePortCharges(chargeFile);

        // Parse routes and collect unique ports
        ifstream file(routeFile);
        if (!file.is_open()) {
            return;
        }

        HashTable<bool> uniquePorts;  // Track which ports we've seen
        LinkedList<Route> routes;

        string line;
        while (getline(file, line)) {
            istringstream iss(line);
            string origin, destination, date, depTime, arrTime, company;
            int cost;

            if (iss >> origin >> destination >> date >> depTime >> arrTime >> cost >> company) {
                Route route(origin, destination, date, depTime, arrTime, cost, company);
                routes.push_back(route);

                // Mark ports as seen
                uniquePorts.insert(origin, true);
                uniquePorts.insert(destination, true);
            }
        }
        file.close();

        // Add all unique ports to graph
        for (int i = 0; i < routes.getSize(); i++) {
            const Route& route = routes.get(i);

            // Add origin port if not already added
            if (!graph.hasPort(route.origin)) {
                int charge = 0;
                portCharges.find(route.origin, charge);
                Port originPort(route.origin, 0, 0, charge);
                assignPortCoordinates(originPort);
                graph.addPort(originPort);
            }

            // Add destination port if not already added
            if (!graph.hasPort(route.destination)) {
                int charge = 0;
                portCharges.find(route.destination, charge);
                Port destPort(route.destination, 0, 0, charge);
                assignPortCoordinates(destPort);
                graph.addPort(destPort);
            }
        }

        // Add all routes to graph
        for (int i = 0; i < routes.getSize(); i++) {
            graph.addRoute(routes.get(i));
        }
    }
};

#endif
