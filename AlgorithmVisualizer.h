#pragma once
#ifndef ALGORITHMVISUALIZER_H
#define ALGORITHMVISUALIZER_H

#include <SFML/Graphics.hpp>
#include "Graph.h"
#include "PathFinder.h"
#include "LinkedList.h"
#include <cmath>
#include <sstream>
#include <string>
using namespace std;
struct AlgorithmStep {
    int currentPortIdx;
    LinkedList<int> visitedPorts;
    LinkedList<int> updatedPorts;
    int* distances;
    int* parents;
    bool* visited;
    int numPorts;
    string message;
};

class AlgorithmVisualizer {
private:
    Graph* graph;
    sf::Font* font;
    
    // Map background
    sf::Texture mapTexture;
    sf::Sprite mapSprite;
    bool hasMapBackground;

    // Algorithm visualization state
    bool isVisualizing;
    bool isPaused;
    int currentStep;
    LinkedList<AlgorithmStep> steps;
    PortMapper* portMapper;
    string origin;
    string destination;
    string date;
    bool useBidirectional;

    // Visual settings
    const float PORT_RADIUS = 10.0f;
    const float ROUTE_THICKNESS = 3.0f;

public:
    AlgorithmVisualizer(Graph* g, sf::Font* f)
        : graph(g), font(f), isVisualizing(false), isPaused(false), 
          currentStep(0), hasMapBackground(false), portMapper(nullptr), 
          useBidirectional(false) {
    }

    bool loadMapBackground(const string& filename) {
        if (!mapTexture.loadFromFile(filename)) {
            hasMapBackground = false;
            return false;
        }
        mapSprite.setTexture(mapTexture);
        float scaleX = 1200.0f / mapTexture.getSize().x;
        float scaleY = 800.0f / mapTexture.getSize().y;
        mapSprite.setScale(scaleX, scaleY);
        hasMapBackground = true;
        return true;
    }

    void startVisualization(const string& orig, const string& dest, 
                           const string& dateStr, bool bidirectional = false) {
        origin = orig;
        destination = dest;
        date = dateStr;
        useBidirectional = bidirectional;
        isVisualizing = true;
        isPaused = false;
        currentStep = 0;
        steps.clear();
        
        // Generate steps (this would be called from PathFinder)
        // For now, we'll generate them on the fly
    }

    void addStep(const AlgorithmStep& step) {
        steps.push_back(step);
    }

    void nextStep() {
        if (currentStep < steps.getSize() - 1) {
            currentStep++;
        }
    }

    void previousStep() {
        if (currentStep > 0) {
            currentStep--;
        }
    }

    void pause() {
        isPaused = !isPaused;
    }

    bool isComplete() const {
        return isVisualizing && currentStep >= steps.getSize() - 1;
    }

    void render(sf::RenderWindow& window) {
        // Draw map background
        if (hasMapBackground) {
            window.draw(mapSprite);
        }
        else {
            sf::RectangleShape background(sf::Vector2f(1200, 800));
            background.setFillColor(sf::Color(30, 60, 100));
            window.draw(background);
        }

        if (isVisualizing && steps.getSize() > 0 && currentStep < steps.getSize()) {
            const AlgorithmStep& step = steps.get(currentStep);
            drawStepVisualization(window, step);
        }
    }

private:
    void drawStepVisualization(sf::RenderWindow& window, const AlgorithmStep& step) {
        // Draw all routes (dimmed)
        drawAllRoutesDimmed(window);

        // Draw visited ports in blue
        for (int i = 0; i < step.visitedPorts.getSize(); i++) {
            int portIdx = step.visitedPorts.get(i);
            string portName = portMapper->getName(portIdx);
            Port port;
            if (graph->getPort(portName, port)) {
                drawPort(window, port, sf::Color(100, 150, 255), true);  // Blue for visited
            }
        }

        // Draw updated ports in yellow
        for (int i = 0; i < step.updatedPorts.getSize(); i++) {
            int portIdx = step.updatedPorts.get(i);
            string portName = portMapper->getName(portIdx);
            Port port;
            if (graph->getPort(portName, port)) {
                drawPort(window, port, sf::Color(255, 255, 100), true);  // Yellow for updated
            }
        }

        // Draw current port in red
        if (step.currentPortIdx >= 0) {
            string currentPortName = portMapper->getName(step.currentPortIdx);
            Port port;
            if (graph->getPort(currentPortName, port)) {
                drawPort(window, port, sf::Color(255, 100, 100), true);  // Red for current
            }
        }

        // Draw origin and destination
        Port origPort, destPort;
        if (graph->getPort(origin, origPort)) {
            drawPort(window, origPort, sf::Color::Green, true);
        }
        if (graph->getPort(destination, destPort)) {
            drawPort(window, destPort, sf::Color::Magenta, true);
        }

        // Draw step info
        drawStepInfo(window, step);
    }

    void drawAllRoutesDimmed(sf::RenderWindow& window) {
        LinkedList<Route> allRoutes = graph->getAllRoutes();
        for (int i = 0; i < allRoutes.getSize(); i++) {
            const Route& route = allRoutes.get(i);
            Port originPort, destPort;
            if (graph->getPort(route.origin, originPort) &&
                graph->getPort(route.destination, destPort)) {
                
                sf::Vector2f start(originPort.x, originPort.y);
                sf::Vector2f end(destPort.x, destPort.y);
                sf::Vector2f direction = end - start;
                float length = sqrt(direction.x * direction.x + direction.y * direction.y);
                float angle = atan2(direction.y, direction.x) * 180 / 3.14159f;

                sf::RectangleShape line(sf::Vector2f(length, ROUTE_THICKNESS));
                line.setPosition(start);
                line.setRotation(angle);
                line.setFillColor(sf::Color(50, 50, 50, 80));  // Very dim
                window.draw(line);
            }
        }
    }

    void drawPort(sf::RenderWindow& window, const Port& port, sf::Color color, bool highlight) {
        sf::CircleShape circle(PORT_RADIUS);
        circle.setPosition(port.x - PORT_RADIUS, port.y - PORT_RADIUS);
        circle.setFillColor(color);
        circle.setOutlineThickness(highlight ? 3 : 2);
        circle.setOutlineColor(sf::Color::White);
        window.draw(circle);

        sf::Text name(port.name, *font, 12);
        name.setPosition(port.x + PORT_RADIUS + 5, port.y - 6);
        name.setFillColor(sf::Color::White);
        name.setOutlineThickness(1);
        name.setOutlineColor(sf::Color::Black);
        window.draw(name);
    }

    void drawStepInfo(sf::RenderWindow& window, const AlgorithmStep& step) {
        sf::RectangleShape infoBox(sf::Vector2f(400, 200));
        infoBox.setPosition(15, 580);
        infoBox.setFillColor(sf::Color(0, 0, 0, 220));
        infoBox.setOutlineThickness(3);
        infoBox.setOutlineColor(sf::Color::Cyan);
        window.draw(infoBox);

        stringstream info;
        info << "Dijkstra Algorithm Step: " << (currentStep + 1) << " / " << steps.getSize() << "\n";
        info << step.message << "\n\n";
        
        if (step.currentPortIdx >= 0) {
            info << "Current Port: " << portMapper->getName(step.currentPortIdx) << "\n";
        }
        info << "Visited: " << step.visitedPorts.getSize() << " ports\n";
        info << "Updated: " << step.updatedPorts.getSize() << " ports\n";
        
        if (useBidirectional) {
            info << "\nMode: Bidirectional Search";
        }

        sf::Text infoText(info.str(), *font, 14);
        infoText.setFillColor(sf::Color::White);
        infoText.setPosition(25, 590);
        window.draw(infoText);

        // Navigation hints
        sf::Text navHint("Press SPACE for next step, LEFT/RIGHT arrows to navigate", font, 12);
        navHint.setFillColor(sf::Color(200, 200, 200));
        navHint.setPosition(25, 760);
        window.draw(navHint);
    }
};

#endif

