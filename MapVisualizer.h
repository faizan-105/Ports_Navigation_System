
#pragma once
#ifndef MAPVISUALIZER_H
#define MAPVISUALIZER_H

#include <SFML/Graphics.hpp>
#include "Graph.h"
#include "LinkedList.h"
#include <string>
#include <sstream>
#include <cmath>
using namespace std;
class MapVisualizer
{
private:
    Graph *graph;
    sf::Font *font;
    sf::Texture mapTexture;
    sf::Sprite mapSprite;

    // Hover detection
    int hoveredRouteIndex;
    Route hoveredRoute;
    bool isHoveringRoute;

    // Subgraph filtering
    LinkedList<string> filterCompanies; // Multiple companies filter
    string filterDate;
    bool isFiltered;
    LinkedList<string> activePorts; // Ports that are part of filtered subgraph

    // Visual settings
    const float PORT_RADIUS = 8.0f;
    const float ROUTE_THICKNESS = 2.0f;
    const float HOVER_DETECTION_DISTANCE = 10.0f;

    // Helper: Calculate distance from point to line segment
    float pointToLineDistance(sf::Vector2f point, sf::Vector2f lineStart, sf::Vector2f lineEnd)
    {
        float A = point.x - lineStart.x;
        float B = point.y - lineStart.y;
        float C = lineEnd.x - lineStart.x;
        float D = lineEnd.y - lineStart.y;

        float dot = A * C + B * D;
        float lenSq = C * C + D * D;
        float param = -1;

        if (lenSq != 0)
        {
            param = dot / lenSq;
        }

        float xx, yy;

        if (param < 0)
        {
            xx = lineStart.x;
            yy = lineStart.y;
        }
        else if (param > 1)
        {
            xx = lineEnd.x;
            yy = lineEnd.y;
        }
        else
        {
            xx = lineStart.x + param * C;
            yy = lineStart.y + param * D;
        }

        float dx = point.x - xx;
        float dy = point.y - yy;
        return sqrt(dx * dx + dy * dy);
    }

public:
    MapVisualizer(Graph *g, sf::Font *f)
        : graph(g), font(f), hoveredRouteIndex(-1), isHoveringRoute(false),
          isFiltered(false), filterDate("")
    {
    }

    // Set single company filter for subgraph
    void setCompanyFilter(const string &company)
    {
        filterCompanies.clear();
        if (!company.empty())
        {
            filterCompanies.push_back(company);
        }
        isFiltered = !company.empty();
        updateActivePorts();
    }

    // Set multiple companies filter
    void setCompanyFilters(const LinkedList<string> &companies)
    {
        filterCompanies = companies;
        isFiltered = companies.getSize() > 0;
        updateActivePorts();
    }

    // Add a company to the filter
    void addCompanyFilter(const string &company)
    {
        // Check if already exists
        for (int i = 0; i < filterCompanies.getSize(); i++)
        {
            if (filterCompanies.get(i) == company)
            {
                return; // Already in filter
            }
        }
        filterCompanies.push_back(company);
        isFiltered = true;
        updateActivePorts();
    }

    // Set date filter for subgraph
    void setDateFilter(const string &date)
    {
        filterDate = date;
        isFiltered = !date.empty();
        updateActivePorts();
    }

    // Clear all filters
    void clearFilters()
    {
        filterCompanies.clear();
        filterDate = "";
        isFiltered = false;
        activePorts.clear();
    }

    // Check if route matches current filters
    bool routeMatchesFilter(const Route &route)
    {
        if (!isFiltered)
            return true;

        // Check company filter (if any companies are selected, route must match one)
        if (filterCompanies.getSize() > 0)
        {
            bool companyMatches = false;
            for (int i = 0; i < filterCompanies.getSize(); i++)
            {
                if (route.shippingCompany == filterCompanies.get(i))
                {
                    companyMatches = true;
                    break;
                }
            }
            if (!companyMatches)
            {
                return false;
            }
        }

        // Check date filter
        if (!filterDate.empty() && route.date != filterDate)
        {
            return false;
        }
        return true;
    }

    // Update list of active ports in filtered subgraph
    void updateActivePorts()
    {
        activePorts.clear();
        if (!isFiltered)
            return;

        LinkedList<Route> allRoutes = graph->getAllRoutes();
        for (int i = 0; i < allRoutes.getSize(); i++)
        {
            const Route &route = allRoutes.get(i);
            if (routeMatchesFilter(route))
            {
                // Add origin port
                bool found = false;
                for (int j = 0; j < activePorts.getSize(); j++)
                {
                    if (activePorts.get(j) == route.origin)
                    {
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    activePorts.push_back(route.origin);
                }
                // Add destination port
                found = false;
                for (int j = 0; j < activePorts.getSize(); j++)
                {
                    if (activePorts.get(j) == route.destination)
                    {
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    activePorts.push_back(route.destination);
                }
            }
        }
    }

    bool isPortActive(const string &portName)
    {
        if (!isFiltered)
            return true;
        for (int i = 0; i < activePorts.getSize(); i++)
        {
            if (activePorts.get(i) == portName)
            {
                return true;
            }
        }
        return false;
    }

    bool loadMapBackground(const string &filename)
    {
        if (!mapTexture.loadFromFile(filename))
        {
            // If map image not found, we'll use a solid background
            return false;
        }
        mapSprite.setTexture(mapTexture);

        // Scale to fit window (1200x800)
        float scaleX = 1200.0f / mapTexture.getSize().x;
        float scaleY = 800.0f / mapTexture.getSize().y;
        mapSprite.setScale(scaleX, scaleY);

        return true;
    }

    void update(sf::RenderWindow &window)
    {
        // Get mouse position
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        sf::Vector2f mousePosF(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y));

        // Check if hovering over any route
        isHoveringRoute = false;
        LinkedList<Route> allRoutes = graph->getAllRoutes();

        for (int i = 0; i < allRoutes.getSize(); i++)
        {
            const Route &route = allRoutes.get(i);

            Port originPort, destPort;
            if (graph->getPort(route.origin, originPort) &&
                graph->getPort(route.destination, destPort))
            {

                sf::Vector2f start(originPort.x, originPort.y);
                sf::Vector2f end(destPort.x, destPort.y);

                float distance = pointToLineDistance(mousePosF, start, end);

                if (distance < HOVER_DETECTION_DISTANCE)
                {
                    isHoveringRoute = true;
                    hoveredRoute = route;
                    hoveredRouteIndex = i;
                    break;
                }
            }
        }
    }

    void render(sf::RenderWindow &window)
    {
        // Draw map background
        if (mapTexture.getSize().x > 0)
        {
            window.draw(mapSprite);
        }
        else
        {
            // Draw ocean blue background if no map image
            sf::RectangleShape background(sf::Vector2f(1200, 800));
            background.setFillColor(sf::Color(30, 60, 100));
            window.draw(background);
        }

        // Draw all routes (edges) - filtered if subgraph is active
        drawRoutes(window);

        // Draw all ports (nodes) - dimmed if not in subgraph
        drawPorts(window);

        // Draw hover info
        if (isHoveringRoute)
        {
            drawRouteInfo(window);
        }

        // Draw filter info if subgraph is active
        if (isFiltered)
        {
            drawFilterInfo(window);
        }
    }

    void drawFilterInfo(sf::RenderWindow &window)
    {
        sf::RectangleShape filterBox(sf::Vector2f(300, 100));
        filterBox.setPosition(890, 20);
        filterBox.setFillColor(sf::Color(0, 0, 0, 200));
        filterBox.setOutlineThickness(2);
        filterBox.setOutlineColor(sf::Color(100, 200, 255));
        window.draw(filterBox);

        sf::Text filterTitle("Subgraph Filter", *font, 16);
        filterTitle.setFillColor(sf::Color(100, 200, 255));
        filterTitle.setStyle(sf::Text::Bold);
        filterTitle.setPosition(900, 25);
        window.draw(filterTitle);

        stringstream filterInfo;
        if (filterCompanies.getSize() > 0)
        {
            filterInfo << "Companies: ";
            for (int i = 0; i < filterCompanies.getSize(); i++)
            {
                filterInfo << filterCompanies.get(i);
                if (i < filterCompanies.getSize() - 1)
                {
                    filterInfo << ", ";
                }
            }
            filterInfo << "\n";
        }
        if (!filterDate.empty())
        {
            filterInfo << "Date: " << filterDate << "\n";
        }
        filterInfo << "Active Ports: " << activePorts.getSize();

        sf::Text filterText(filterInfo.str(), *font, 12);
        filterText.setFillColor(sf::Color::White);
        filterText.setPosition(900, 50);
        window.draw(filterText);
    }

private:
    void drawRoutes(sf::RenderWindow &window)
    {
        LinkedList<Route> allRoutes = graph->getAllRoutes();

        for (int i = 0; i < allRoutes.getSize(); i++)
        {
            const Route &route = allRoutes.get(i);

            // Apply subgraph filter
            if (!routeMatchesFilter(route))
            {
                continue; // Skip routes that don't match filter
            }

            Port originPort, destPort;
            if (!graph->getPort(route.origin, originPort) ||
                !graph->getPort(route.destination, destPort))
            {
                continue;
            }

            sf::Vector2f start(originPort.x, originPort.y);
            sf::Vector2f end(destPort.x, destPort.y);

            // Calculate direction and length
            sf::Vector2f direction = end - start;
            float length = sqrt(direction.x * direction.x + direction.y * direction.y);
            float angle = atan2(direction.y, direction.x) * 180 / 3.14159f;

            // Draw route line
            sf::RectangleShape line(sf::Vector2f(length, ROUTE_THICKNESS));
            line.setPosition(start);
            line.setRotation(angle);

            // Color based on cost (cheaper = green, expensive = red)
            float costRatio = min(route.cost / 50000.0f, 1.0f);
            sf::Color routeColor(
                static_cast<sf::Uint8>(255 * costRatio),
                static_cast<sf::Uint8>(255 * (1 - costRatio)),
                100,
                150 // Semi-transparent
            );

            // Highlight if hovered
            if (isHoveringRoute && hoveredRouteIndex == i)
            {
                routeColor = sf::Color::Yellow;
                line.setOutlineThickness(1);
                line.setOutlineColor(sf::Color::White);
            }

            line.setFillColor(routeColor);
            window.draw(line);

            // Draw arrow head
            drawArrowHead(window, end, angle, routeColor);
        }
    }

    void drawArrowHead(sf::RenderWindow &window, sf::Vector2f position,
                       float angle, sf::Color color)
    {
        sf::ConvexShape arrow(3);
        arrow.setPoint(0, sf::Vector2f(0, 0));
        arrow.setPoint(1, sf::Vector2f(-10, -5));
        arrow.setPoint(2, sf::Vector2f(-10, 5));

        arrow.setFillColor(color);
        arrow.setPosition(position);
        arrow.setRotation(angle);

        window.draw(arrow);
    }

    void drawPorts(sf::RenderWindow &window)
    {
        LinkedList<Port> allPorts = graph->getAllPorts();

        for (int i = 0; i < allPorts.getSize(); i++)
        {
            const Port &port = allPorts.get(i);

            // Check if port is active in filtered subgraph
            bool isActive = isPortActive(port.name);

            // Draw port circle
            sf::CircleShape portCircle(PORT_RADIUS);
            portCircle.setPosition(port.x - PORT_RADIUS, port.y - PORT_RADIUS);

            if (isActive)
            {
                // Active port - highlighted
                portCircle.setFillColor(sf::Color(255, 200, 50));
                portCircle.setOutlineThickness(2);
                portCircle.setOutlineColor(sf::Color::White);
            }
            else
            {
                // Inactive port - dimmed
                portCircle.setFillColor(sf::Color(100, 100, 100, 100));
                portCircle.setOutlineThickness(1);
                portCircle.setOutlineColor(sf::Color(150, 150, 150, 100));
            }

            window.draw(portCircle);

            // Draw port name (only if active or not filtered)
            if (isActive || !isFiltered)
            {
                sf::Text portName(port.name, *font, 12);
                portName.setPosition(port.x + PORT_RADIUS + 5, port.y - 6);
                portName.setFillColor(isActive ? sf::Color::White : sf::Color(150, 150, 150));
                portName.setOutlineThickness(1);
                portName.setOutlineColor(sf::Color::Black);
                window.draw(portName);
            }
        }
    }

    void drawRouteInfo(sf::RenderWindow &window)
    {
        // Create info box
        sf::RectangleShape infoBox(sf::Vector2f(350, 180));
        infoBox.setPosition(10, 600);
        infoBox.setFillColor(sf::Color(0, 0, 0, 200));
        infoBox.setOutlineThickness(2);
        infoBox.setOutlineColor(sf::Color::Cyan);

        window.draw(infoBox);

        // Draw route information
        stringstream info;
        info << "Route Information:\n"
             << "Origin: " << hoveredRoute.origin << "\n"
             << "Destination: " << hoveredRoute.destination << "\n"
             << "Date: " << hoveredRoute.date << "\n"
             << "Departure: " << hoveredRoute.departureTime << "\n"
             << "Arrival: " << hoveredRoute.arrivalTime << "\n"
             << "Cost: $" << hoveredRoute.cost << "\n"
             << "Company: " << hoveredRoute.shippingCompany;

        sf::Text infoText(info.str(), *font, 14);
        infoText.setPosition(20, 610);
        infoText.setFillColor(sf::Color::White);

        window.draw(infoText);
    }
};

#endif