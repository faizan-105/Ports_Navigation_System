
#ifndef BOOKINGVISUALIZER_H
#define BOOKINGVISUALIZER_H

#include <SFML/Graphics.hpp>
#include "Graph.h"
#include "PathFinder.h"
#include "LinkedList.h"
#include <cmath>
#include <sstream>
using namespace std;
class BookingVisualizer
{
private:
    Graph *graph;
    sf::Font *font;

    // Map background
    sf::Texture mapTexture;
    sf::Sprite mapSprite;
    bool hasMapBackground;

    // Animation
    sf::Clock animationClock;
    float animationProgress;
    bool isAnimating;

    // Current path being displayed
    PathResult currentPath;
    LinkedList<Route> displayRoutes;   // Connecting routes (highlighted)
    LinkedList<Route> allRoutes;       // All routes in graph
    LinkedList<string> importantPorts; // Ports that are part of connecting routes
    string origin;
    string destination;
    bool showAllRoutes; // Whether to show all routes or just connecting ones

    // Algorithm step visualization
    bool showAlgorithmSteps;
    LinkedList<string> visitedPorts;    // Ports visited during algorithm
    LinkedList<string> processingPorts; // Ports currently being processed
    string currentProcessingPort;       // Current port being processed

    const float PORT_RADIUS = 10.0f;
    const float ROUTE_THICKNESS = 3.0f;

public:
    BookingVisualizer(Graph *g, sf::Font *f)
        : graph(g), font(f), animationProgress(0.0f), isAnimating(false), hasMapBackground(false),
          showAllRoutes(false), showAlgorithmSteps(false)
    {
    }

    // Set algorithm step visualization data
    void setAlgorithmSteps(const LinkedList<string> &visited, const LinkedList<string> &processing,
                           const string &current)
    {
        visitedPorts = visited;
        processingPorts = processing;
        currentProcessingPort = current;
        showAlgorithmSteps = true;
    }

    void clearAlgorithmSteps()
    {
        showAlgorithmSteps = false;
        visitedPorts.clear();
        processingPorts.clear();
        currentProcessingPort = "";
    }
    // NEW: Load map background
    bool loadMapBackground(const string &filename)
    {
        if (!mapTexture.loadFromFile(filename))
        {
            hasMapBackground = false;
            return false;
        }

        mapSprite.setTexture(mapTexture);

        // Scale to fit window (1200x800)
        float scaleX = 1200.0f / mapTexture.getSize().x;
        float scaleY = 800.0f / mapTexture.getSize().y;
        mapSprite.setScale(scaleX, scaleY);

        hasMapBackground = true;
        return true;
    }

    void showConnectingRoutes(const string &orig, const string &dest,
                              const LinkedList<Route> &routes, const string &date = "")
    {
        origin = orig;
        destination = dest;
        displayRoutes = routes;
        isAnimating = false;
        showAllRoutes = false;

        // Get all routes from graph, filtered by date if provided
        if (!date.empty())
        {
            allRoutes.clear();
            LinkedList<Route> allGraphRoutes = graph->getAllRoutes();
            for (int i = 0; i < allGraphRoutes.getSize(); i++)
            {
                if (allGraphRoutes.get(i).date == date)
                {
                    allRoutes.push_back(allGraphRoutes.get(i));
                }
            }
        }
        else
        {
            allRoutes = graph->getAllRoutes();
        }

        // Build set of important ports (ports that are part of connecting routes)
        importantPorts.clear();
        for (int i = 0; i < displayRoutes.getSize(); i++)
        {
            const Route &route = displayRoutes.get(i);
            // Add origin port
            bool found = false;
            for (int j = 0; j < importantPorts.getSize(); j++)
            {
                if (importantPorts.get(j) == route.origin)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                importantPorts.push_back(route.origin);
            }
            // Add destination port
            found = false;
            for (int j = 0; j < importantPorts.getSize(); j++)
            {
                if (importantPorts.get(j) == route.destination)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                importantPorts.push_back(route.destination);
            }
        }
    }

    // Update connecting routes (for real-time updates in option 3)
    void updateConnectingRoutes(const LinkedList<Route> &routes, const string &date = "")
    {
        displayRoutes = routes;

        // Update all routes if date is provided
        if (!date.empty())
        {
            allRoutes.clear();
            LinkedList<Route> allGraphRoutes = graph->getAllRoutes();
            for (int i = 0; i < allGraphRoutes.getSize(); i++)
            {
                if (allGraphRoutes.get(i).date == date)
                {
                    allRoutes.push_back(allGraphRoutes.get(i));
                }
            }
        }

        // Rebuild important ports
        importantPorts.clear();
        for (int i = 0; i < displayRoutes.getSize(); i++)
        {
            const Route &route = displayRoutes.get(i);
            bool found = false;
            for (int j = 0; j < importantPorts.getSize(); j++)
            {
                if (importantPorts.get(j) == route.origin)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                importantPorts.push_back(route.origin);
            }
            found = false;
            for (int j = 0; j < importantPorts.getSize(); j++)
            {
                if (importantPorts.get(j) == route.destination)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                importantPorts.push_back(route.destination);
            }
        }
    }

    void startPathAnimation(const PathResult &path)
    {
        currentPath = path;
        animationProgress = 0.0f;
        isAnimating = true;
        animationClock.restart();
    }

    bool isAnimationComplete() const
    {
        return !isAnimating || animationProgress >= 1.0f;
    }

    void update()
    {
        if (isAnimating)
        {
            float elapsed = animationClock.getElapsedTime().asSeconds();
            animationProgress = min(elapsed / 2.0f, 1.0f);

            if (animationProgress >= 1.0f)
            {
                isAnimating = false;
            }
        }
    }

    void render(sf::RenderWindow &window)
    {
        // Draw map background first
        if (hasMapBackground)
        {
            window.draw(mapSprite);
        }
        else
        {
            // Draw ocean blue background if no map
            sf::RectangleShape background(sf::Vector2f(1200, 800));
            background.setFillColor(sf::Color(30, 60, 100));
            window.draw(background);
        }

        if (showAllRoutes)
        {
            // Draw all routes (dimmed for non-important, highlighted for important)
            drawAllRoutes(window);

            // Draw all ports (dimmed for non-important, highlighted for important)
            drawAllPorts(window);
        }
        else
        {
            // Draw all connecting routes in red
            drawConnectingRoutes(window);

            // Draw origin and destination ports highlighted
            drawHighlightedPorts(window);
        }

        // Draw optimal path with animation
        if (currentPath.found)
        {
            drawOptimalPath(window);
        }
    }

private:
    void drawConnectingRoutes(sf::RenderWindow &window)
    {
        for (int i = 0; i < displayRoutes.getSize(); i++)
        {
            const Route &route = displayRoutes.get(i);

            Port originPort, destPort;
            if (!graph->getPort(route.origin, originPort) ||
                !graph->getPort(route.destination, destPort))
            {
                continue;
            }

            sf::Vector2f start(originPort.x, originPort.y);
            sf::Vector2f end(destPort.x, destPort.y);

            // Calculate line
            sf::Vector2f direction = end - start;
            float length = sqrt(direction.x * direction.x + direction.y * direction.y);
            float angle = atan2(direction.y, direction.x) * 180 / 3.14159f;

            // Check if this route is part of the optimal path
            bool isOptimal = isOptimalRoute(route);

            // Draw route line
            sf::RectangleShape line(sf::Vector2f(length, ROUTE_THICKNESS));
            line.setPosition(start);
            line.setRotation(angle);

            if (isOptimal && animationProgress > 0.0f)
            {
                // Transition from red to golden based on animation progress
                int red = 255;
                int green = static_cast<int>(215 * animationProgress);
                int blue = 0;
                line.setFillColor(sf::Color(red, green, blue, 150));
            }
            else
            {
                // Regular red route
                line.setFillColor(sf::Color(255, 0, 0, 150));
            }
            window.draw(line);

            // Draw arrow
            if (isOptimal && animationProgress > 0.0f)
            {
                int red = 255;
                int green = static_cast<int>(215 * animationProgress);
                int blue = 0;
                drawArrowHead(window, end, angle, sf::Color(red, green, blue, 150));
            }
            else
            {
                drawArrowHead(window, end, angle, sf::Color(100, 100, 100, 100));
            }
        }
    }

    void drawHighlightedPorts(sf::RenderWindow &window)
    {
        // Use simple array to track drawn ports (NO HASHTABLE!)
        LinkedList<string> portsDrawn;

        for (int i = 0; i < displayRoutes.getSize(); i++)
        {
            const Route &route = displayRoutes.get(i);

            // Check if origin port already drawn
            bool originDrawn = false;
            for (int j = 0; j < portsDrawn.getSize(); j++)
            {
                if (portsDrawn.get(j) == route.origin)
                {
                    originDrawn = true;
                    break;
                }
            }

            if (!originDrawn)
            {
                Port port;
                if (graph->getPort(route.origin, port))
                {
                    drawPort(window, port, route.origin == origin || route.origin == destination);
                    portsDrawn.push_back(route.origin);
                }
            }

            // Check if destination port already drawn
            bool destDrawn = false;
            for (int j = 0; j < portsDrawn.getSize(); j++)
            {
                if (portsDrawn.get(j) == route.destination)
                {
                    destDrawn = true;
                    break;
                }
            }

            if (!destDrawn)
            {
                Port port;
                if (graph->getPort(route.destination, port))
                {
                    drawPort(window, port, route.destination == origin || route.destination == destination);
                    portsDrawn.push_back(route.destination);
                }
            }
        }
    }

    void drawPort(sf::RenderWindow &window, const Port &port, bool isEndpoint)
    {
        sf::CircleShape circle(PORT_RADIUS);
        circle.setPosition(port.x - PORT_RADIUS, port.y - PORT_RADIUS);

        if (isEndpoint)
        {
            // Highlight origin/destination in yellow
            circle.setFillColor(sf::Color::Yellow);
            circle.setOutlineThickness(3);
            circle.setOutlineColor(sf::Color::White);
        }
        else if (isOptimalPort(port.name))
        {
            // Highlight intermediate ports in optimal path as orange
            circle.setFillColor(sf::Color(255, 165, 0)); // Orange
            circle.setOutlineThickness(3);
            circle.setOutlineColor(sf::Color::White);
        }
        else
        {
            // Regular port in gray
            circle.setFillColor(sf::Color(150, 150, 150));
            circle.setOutlineThickness(2);
            circle.setOutlineColor(sf::Color::White);
        }

        window.draw(circle);

        // Draw port name
        sf::Text name(port.name, *font, 12);
        name.setPosition(port.x + PORT_RADIUS + 5, port.y - 6);
        name.setFillColor(sf::Color::White);
        name.setOutlineThickness(1);
        name.setOutlineColor(sf::Color::Black);
        window.draw(name);
    }

    void drawOptimalPath(sf::RenderWindow &window)
    {
        for (int i = 0; i < currentPath.routes.getSize(); i++)
        {
            const Route &route = currentPath.routes.get(i);

            Port originPort, destPort;
            if (!graph->getPort(route.origin, originPort) ||
                !graph->getPort(route.destination, destPort))
            {
                continue;
            }

            sf::Vector2f start(originPort.x, originPort.y);
            sf::Vector2f end(destPort.x, destPort.y);

            // Calculate progress for this segment
            float segmentStart = static_cast<float>(i) / currentPath.routes.getSize();
            float segmentEnd = static_cast<float>(i + 1) / currentPath.routes.getSize();

            if (animationProgress < segmentStart)
            {
                continue;
            }

            // Calculate how much of this segment to draw
            float segmentProgress = 1.0f;
            if (animationProgress < segmentEnd)
            {
                segmentProgress = (animationProgress - segmentStart) / (segmentEnd - segmentStart);
            }

            sf::Vector2f animEnd = start + (end - start) * segmentProgress;

            // Calculate line
            sf::Vector2f direction = animEnd - start;
            float length = sqrt(direction.x * direction.x + direction.y * direction.y);
            float angle = atan2(direction.y, direction.x) * 180 / 3.14159f;

            // Draw golden route
            sf::RectangleShape line(sf::Vector2f(length, ROUTE_THICKNESS + 2));
            line.setPosition(start);
            line.setRotation(angle);
            line.setFillColor(sf::Color(255, 215, 0));
            line.setOutlineThickness(1);
            line.setOutlineColor(sf::Color::White);
            window.draw(line);

            // Draw arrow at end if segment is complete
            if (segmentProgress >= 1.0f)
            {
                drawArrowHead(window, animEnd, angle, sf::Color(255, 215, 0));
            }
        }
    }

    void drawArrowHead(sf::RenderWindow &window, sf::Vector2f position,
                       float angle, sf::Color color)
    {
        sf::ConvexShape arrow(3);
        arrow.setPoint(0, sf::Vector2f(0, 0));
        arrow.setPoint(1, sf::Vector2f(-12, -6));
        arrow.setPoint(2, sf::Vector2f(-12, 6));

        arrow.setFillColor(color);
        arrow.setPosition(position);
        arrow.setRotation(angle);

        window.draw(arrow);
    }

    // Helper: Check if a route is in the connecting routes
    bool isRouteImportant(const Route &route)
    {
        for (int i = 0; i < displayRoutes.getSize(); i++)
        {
            const Route &connRoute = displayRoutes.get(i);
            if (connRoute.origin == route.origin &&
                connRoute.destination == route.destination &&
                connRoute.date == route.date)
            {
                return true;
            }
        }
        return false;
    }

    // Helper: Check if a route is part of the optimal path
    bool isOptimalRoute(const Route &route)
    {
        if (!currentPath.found)
            return false;
        for (int i = 0; i < currentPath.routes.getSize(); i++)
        {
            const Route &optRoute = currentPath.routes.get(i);
            if (optRoute.origin == route.origin &&
                optRoute.destination == route.destination &&
                optRoute.date == route.date &&
                optRoute.departureTime == route.departureTime)
            {
                return true;
            }
        }
        return false;
    }

    // Helper: Check if a port is part of the optimal path (intermediate port, not origin/destination)
    bool isOptimalPort(const string &portName)
    {
        if (!currentPath.found)
            return false;
        if (portName == origin || portName == destination)
            return false; // Not endpoints

        for (int i = 0; i < currentPath.path.getSize(); i++)
        {
            if (currentPath.path.get(i) == portName)
            {
                return true;
            }
        }
        return false;
    }

    // Helper: Check if a port is in the connecting routes
    bool isPortImportant(const string &portName)
    {
        if (portName == origin || portName == destination)
        {
            return true;
        }
        for (int i = 0; i < importantPorts.getSize(); i++)
        {
            if (importantPorts.get(i) == portName)
            {
                return true;
            }
        }
        return false;
    }

    // Draw all routes with highlighting
    void drawAllRoutes(sf::RenderWindow &window)
    {
        for (int i = 0; i < allRoutes.getSize(); i++)
        {
            const Route &route = allRoutes.get(i);

            Port originPort, destPort;
            if (!graph->getPort(route.origin, originPort) ||
                !graph->getPort(route.destination, destPort))
            {
                continue;
            }

            sf::Vector2f start(originPort.x, originPort.y);
            sf::Vector2f end(destPort.x, destPort.y);

            // Calculate line
            sf::Vector2f direction = end - start;
            float length = sqrt(direction.x * direction.x + direction.y * direction.y);
            float angle = atan2(direction.y, direction.x) * 180 / 3.14159f;

            // Determine if route is important (in connecting routes)
            bool isImportant = isRouteImportant(route);

            // Determine if route is part of optimal path
            bool isOptimal = isOptimalRoute(route);

            // Draw route line
            sf::RectangleShape line(sf::Vector2f(length, ROUTE_THICKNESS));
            line.setPosition(start);
            line.setRotation(angle);

            if (isImportant)
            {
                if (isOptimal && animationProgress > 0.0f)
                {
                    // Transition from red to golden based on animation progress
                    // Interpolate between red (255,0,0) and golden (255,215,0)
                    int red = 255;
                    int green = static_cast<int>(215 * animationProgress);
                    int blue = 0;

                    line.setFillColor(sf::Color(red, green, blue, 200));
                    line.setOutlineThickness(1);
                    line.setOutlineColor(sf::Color(255, 150, 150));
                }
                else
                {
                    // Highlight important routes in red
                    line.setFillColor(sf::Color(255, 0, 0, 200));
                    line.setOutlineThickness(1);
                    line.setOutlineColor(sf::Color(255, 150, 150));
                }
            }
            else
            {
                // Dim non-important routes
                line.setFillColor(sf::Color(100, 100, 100, 80));
            }

            window.draw(line);

            // Draw arrow
            if (isImportant)
            {
                if (isOptimal && animationProgress > 0.0f)
                {
                    int red = 255;
                    int green = static_cast<int>(215 * animationProgress);
                    int blue = 0;
                    drawArrowHead(window, end, angle, sf::Color(red, green, blue, 200));
                }
                else
                {
                    drawArrowHead(window, end, angle, sf::Color(255, 0, 0, 200));
                }
            }
            else
            {
                drawArrowHead(window, end, angle, sf::Color(100, 100, 100, 80));
            }
        }
    }

    // Draw all ports with highlighting
    void drawAllPorts(sf::RenderWindow &window)
    {
        LinkedList<Port> allPorts = graph->getAllPorts();

        for (int i = 0; i < allPorts.getSize(); i++)
        {
            const Port &port = allPorts.get(i);
            bool isImportant = isPortImportant(port.name);

            // Check algorithm step visualization
            bool isVisited = false;
            bool isProcessing = false;
            bool isCurrent = false;

            if (showAlgorithmSteps)
            {
                for (int j = 0; j < visitedPorts.getSize(); j++)
                {
                    if (visitedPorts.get(j) == port.name)
                    {
                        isVisited = true;
                        break;
                    }
                }
                for (int j = 0; j < processingPorts.getSize(); j++)
                {
                    if (processingPorts.get(j) == port.name)
                    {
                        isProcessing = true;
                        break;
                    }
                }
                if (port.name == currentProcessingPort)
                {
                    isCurrent = true;
                }
            }

            // Draw port with appropriate highlighting
            sf::CircleShape circle(PORT_RADIUS);
            circle.setPosition(port.x - PORT_RADIUS, port.y - PORT_RADIUS);

            if (port.name == origin || port.name == destination)
            {
                // Highlight origin/destination
                circle.setFillColor(sf::Color::Yellow);
                circle.setOutlineThickness(3);
                circle.setOutlineColor(sf::Color::White);
            }
            else if (showAlgorithmSteps && isCurrent)
            {
                // Current port being processed (red)
                circle.setFillColor(sf::Color(255, 100, 100));
                circle.setOutlineThickness(3);
                circle.setOutlineColor(sf::Color::White);
            }
            else if (showAlgorithmSteps && isProcessing)
            {
                // Ports being updated (yellow)
                circle.setFillColor(sf::Color(255, 255, 100));
                circle.setOutlineThickness(2);
                circle.setOutlineColor(sf::Color::White);
            }
            else if (showAlgorithmSteps && isVisited)
            {
                // Visited ports (blue)
                circle.setFillColor(sf::Color(100, 150, 255));
                circle.setOutlineThickness(2);
                circle.setOutlineColor(sf::Color::White);
            }
            else if (isImportant)
            {
                // Highlight important ports (part of connecting routes)
                circle.setFillColor(sf::Color(255, 150, 50)); // Orange
                circle.setOutlineThickness(2);
                circle.setOutlineColor(sf::Color::White);
            }
            else
            {
                // Dim non-important ports
                circle.setFillColor(sf::Color(100, 100, 100, 150));
                circle.setOutlineThickness(1);
                circle.setOutlineColor(sf::Color(150, 150, 150));
            }

            window.draw(circle);

            // Draw port name
            sf::Text name(port.name, *font, 12);
            name.setPosition(port.x + PORT_RADIUS + 5, port.y - 6);
            if (isImportant || port.name == origin || port.name == destination ||
                isVisited || isProcessing || isCurrent)
            {
                name.setFillColor(sf::Color::White);
            }
            else
            {
                name.setFillColor(sf::Color(150, 150, 150));
            }
            name.setOutlineThickness(1);
            name.setOutlineColor(sf::Color::Black);
            window.draw(name);
        }
    }
};

#endif