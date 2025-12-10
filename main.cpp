
#include <SFML/Graphics.hpp>
#include "Graph.h"
#include "RouteParser.h"
#include "MapVisualizer.h"
#include "PathFinder.h"
#include "ShortestPathFinder.h"
#include "InputHandler.h"
#include "BookingVisualizer.h"
#include "PreferenceInputHandler.h"
#include "PreferenceFilter.h"
#include "LinkedList.h"
#include <iostream>
#include <sstream>
using namespace std;
// Menu options
enum MenuOption
{
    NONE = 0,
    SHOW_MAP = 1,
    BOOK_WITHOUT_PREFERENCE = 2,
    BOOK_WITH_PREFERENCE = 3,
    MULTI_LEG_ROUTE = 4,
    DOCKING_LAYOVER = 5,
    EXIT = 6
};

// Booking states
enum BookingState
{
    INPUT_ORIGIN,
    INPUT_DESTINATION,
    INPUT_DATE,
    SELECTING_ROUTE_TYPE, // NEW: Choose between cheapest and shortest
    VALIDATING,
    FINDING_ALL_PATHS,

    SHOWING_ALL_ROUTES,
    CALCULATING_OPTIMAL,
    SHOWING_ALGORITHM_STEPS, // Step-by-step visualization
    SHOWING_OPTIMAL_PATH,
    BOOKING_COMPLETE
};

// Preference booking states
enum PreferenceBookingState
{
    PREF_INPUT_PREFERENCES, // NEW: Get preferences first (companies, ports, etc)
    PREF_FILTERING_MAP,     // NEW: Show map with live filtering as preferences are entered
    PREF_INPUT_ORIGIN,      // REORDERED: After preferences
    PREF_INPUT_DESTINATION,
    PREF_INPUT_DATE,
    PREF_SELECTING_ROUTE_TYPE, // NEW: Choose between cheapest and shortest
    PREF_VALIDATING,
    PREF_SHOWING_CONNECTING_ROUTES,
    PREF_VALIDATING_PREFERENCES,
    PREF_CALCULATING_OPTIMAL,
    PREF_SHOWING_OPTIMAL_PATH,
    PREF_BOOKING_COMPLETE
};

// Multi-leg route states
enum MultiLegState
{
    MULTI_INPUT_ORIGIN,
    MULTI_INPUT_INTERMEDIATE,
    MULTI_INPUT_DESTINATION,
    MULTI_INPUT_DATE,
    MULTI_VALIDATING,
    MULTI_CALCULATING,
    MULTI_SHOWING_ROUTE,
    MULTI_COMPLETE
};

// Docking and layover states
enum DockingState
{
    DOCKING_VIEW_QUEUES,
    DOCKING_ADD_SHIP,
    DOCKING_REMOVE_SHIP,
    DOCKING_VIEW_PORT
};

int main()
{
    // Create window
    sf::RenderWindow window(sf::VideoMode(1200, 800), "OceanRoute Nav");
    window.setFramerateLimit(60);

    // Load font
    sf::Font font;
    if (!font.loadFromFile("arial.ttf"))
    {
        cerr << "Error: Could not load arial.ttf" << endl;
        return -1;
    }

    // Build the graph
    cout << "Loading maritime data..." << endl;
    Graph maritimeGraph;
    RouteParser::buildGraphFromFile(maritimeGraph, "Routes.txt", "PortCharges.txt");

    cout << "Graph loaded successfully!" << endl;
    cout << "Total Ports: " << maritimeGraph.getVertexCount() << endl;
    cout << "Total Routes: " << maritimeGraph.getAllRoutes().getSize() << endl;

    // Create visualizers and utilities
    MapVisualizer mapVisualizer(&maritimeGraph, &font);
    if (!mapVisualizer.loadMapBackground("Images/maps.png"))
    {
        cout << "Warning: map image not found. Using default background." << endl;
    }

    BookingVisualizer bookingVisualizer(&maritimeGraph, &font);

    // Load map for BookingVisualizer
    if (!bookingVisualizer.loadMapBackground("Images/maps.png"))
    {
        cout << "Warning: map image not found for BookingVisualizer." << endl;
    }

    // Input handlers
    InputHandler originInput, destinationInput, dateInput;
    InputHandler routeTypeInput; // NEW: For choosing between cheapest and shortest

    // Preference input handler
    PreferenceInputHandler preferenceInput;

    // Pathfinders
    PathFinder pathFinder(&maritimeGraph);
    ShortestPathFinder shortestPathFinder(&maritimeGraph); // NEW: For shortest path queries

    // Main menu state
    bool showMenu = true;
    MenuOption selectedOption = NONE;

    // Booking state
    BookingState bookingState = INPUT_ORIGIN;
    string origin, destination, date;
    string errorMessage = "";
    PathResult currentPath;
    LinkedList<LinkedList<string>> allPaths;
    sf::Clock messageTimer;
    bool useBidirectional = false; // Option to use bidirectional search
    bool showStepByStep = false;   // Option to show step-by-step algorithm visualization
    int currentAlgorithmStep = 0;
    LinkedList<string> currentVisitedPorts;
    LinkedList<string> currentProcessingPorts;
    string currentProcessingPortName = "";
    bool useShortestPath = false; // NEW: Track if user chose shortest path

    // Preference booking state
    PreferenceBookingState prefBookingState = PREF_INPUT_ORIGIN;
    string prefOrigin, prefDestination, prefDate;
    string prefErrorMessage = "";
    PathResult prefCurrentPath;
    PreferenceFilter currentPreferences;
    LinkedList<Route> prefConnectingRoutes;
    sf::Clock prefMessageTimer;
    bool prefUseShortestPath = false; // NEW: Track if user chose shortest path for preferences

    // Multi-leg route state
    MultiLegState multiLegState = MULTI_INPUT_ORIGIN;
    string multiOrigin, multiDestination, multiDate;
    LinkedList<string> intermediatePorts;
    InputHandler multiPortInput;
    string multiErrorMessage = "";
    PathResult multiLegPath;
    sf::Clock multiMessageTimer;

    // Docking and layover state
    DockingState dockingState = DOCKING_VIEW_QUEUES;
    InputHandler dockingPortInput, dockingShipInput;
    string selectedDockingPort = "";
    string dockingErrorMessage = "";

    // Subgraph filtering state (for Option 1)
    bool showSubgraphMenu = false;
    InputHandler subgraphCompanyInput, subgraphDateInput;
    string subgraphFilterCompany = "";
    string subgraphFilterDate = "";

    // Main game loop
    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                window.close();
            }

            // Menu navigation - Click-based
            if (showMenu && event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
            {
                sf::Vector2i mousePos = sf::Mouse::getPosition(window);
                int x = mousePos.x;
                int y = mousePos.y;

                // Check if click is within menu option area (x: 250-950, y: 200-650)
                if (x >= 250 && x <= 950 && y >= 200 && y <= 650)
                {
                    int optionIndex = (y - 200) / 70;

                    if (optionIndex >= 0 && optionIndex < 6)
                    {
                        if (optionIndex == 0)
                        {
                            selectedOption = SHOW_MAP;
                            showMenu = false;
                            showSubgraphMenu = false;
                            mapVisualizer.clearFilters();
                            subgraphFilterCompany = "";
                            subgraphFilterDate = "";
                            cout << "Showing map..." << endl;
                        }
                        else if (optionIndex == 1)
                        {
                            selectedOption = BOOK_WITHOUT_PREFERENCE;
                            showMenu = false;
                            bookingState = INPUT_ORIGIN;
                            originInput.clear();
                            destinationInput.clear();
                            dateInput.clear();
                            originInput.activate();
                            errorMessage = "";
                            cout << "Starting booking process..." << endl;
                        }
                        else if (optionIndex == 2)
                        {
                            selectedOption = BOOK_WITH_PREFERENCE;
                            showMenu = false;
                            // NEW FLOW: Start with preferences, not origin!
                            prefBookingState = PREF_INPUT_PREFERENCES;
                            originInput.clear();
                            destinationInput.clear();
                            dateInput.clear();
                            mapVisualizer.clearFilters(); // Start with unfiltered map
                            preferenceInput.startInput(); // Initialize preference input handler
                            prefErrorMessage = "";
                            currentPreferences = PreferenceFilter(); // Reset preferences
                            cout << "Starting preference booking process..." << endl;
                            cout << "Please enter your shipping company preferences..." << endl;
                        }
                        else if (optionIndex == 3)
                        {
                            selectedOption = MULTI_LEG_ROUTE;
                            showMenu = false;
                            multiLegState = MULTI_INPUT_ORIGIN;
                            multiPortInput.clear();
                            multiOrigin = "";
                            multiDestination = "";
                            multiDate = "";
                            intermediatePorts.clear();
                            multiErrorMessage = "";
                            multiPortInput.activate();
                            cout << "Starting multi-leg route generation..." << endl;
                        }
                        else if (optionIndex == 4)
                        {
                            selectedOption = DOCKING_LAYOVER;
                            showMenu = false;
                            dockingState = DOCKING_VIEW_QUEUES;
                            dockingPortInput.clear();
                            dockingShipInput.clear();
                            selectedDockingPort = "";
                            dockingErrorMessage = "";
                            cout << "Opening Docking & Layover Management..." << endl;
                        }
                        else if (optionIndex == 5)
                        {
                            window.close();
                        }
                    }
                }
            }

            // Exit on Escape key in menu
            if (showMenu && event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)
            {
                window.close();
            }

            // Back to menu - Press Esc (only if NO input field is active)
            if (!showMenu && event.type == sf::Event::KeyPressed &&
                event.key.code == sf::Keyboard::Escape)
            {

                // CRITICAL FIX: Only return to menu if no input is active
                bool anyInputActive = originInput.getIsActive() ||
                                      destinationInput.getIsActive() ||
                                      dateInput.getIsActive() ||
                                      routeTypeInput.getIsActive() ||
                                      preferenceInput.isInputActive() ||
                                      multiPortInput.getIsActive() ||
                                      dockingPortInput.getIsActive() ||
                                      dockingShipInput.getIsActive() ||
                                      subgraphCompanyInput.getIsActive() ||
                                      subgraphDateInput.getIsActive();

                if (!anyInputActive)
                {
                    showMenu = true;
                    selectedOption = NONE;
                    originInput.deactivate();
                    destinationInput.deactivate();
                    dateInput.deactivate();
                    routeTypeInput.deactivate();
                    multiPortInput.deactivate();
                    dockingPortInput.deactivate();
                    dockingShipInput.deactivate();
                    subgraphCompanyInput.deactivate();
                    subgraphDateInput.deactivate();
                    cout << "Returning to menu..." << endl;
                }
            }

            // Handle subgraph filtering (Option 1)
            if (selectedOption == SHOW_MAP)
            {
                if (event.type == sf::Event::KeyPressed)
                {
                    if (event.key.code == sf::Keyboard::F)
                    {
                        // Toggle filter menu
                        showSubgraphMenu = !showSubgraphMenu;
                        if (!showSubgraphMenu)
                        {
                            subgraphCompanyInput.deactivate();
                            subgraphDateInput.deactivate();
                        }
                    }
                    else if (event.key.code == sf::Keyboard::C && showSubgraphMenu)
                    {
                        // Clear filters
                        mapVisualizer.clearFilters();
                        subgraphFilterCompany = "";
                        subgraphFilterDate = "";
                        subgraphCompanyInput.clear();
                        subgraphDateInput.clear();
                        cout << "Filters cleared" << endl;
                    }
                }

                if (showSubgraphMenu)
                {
                    // Handle mouse clicks on filter inputs
                    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
                    {
                        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
                        // Check if clicked on company input box
                        if (mousePos.x >= 840 && mousePos.x <= 1160 && mousePos.y >= 185 && mousePos.y <= 215)
                        {
                            if (!subgraphCompanyInput.getIsActive())
                            {
                                subgraphCompanyInput.activate();
                                subgraphDateInput.deactivate();
                            }
                        }
                        // Check if clicked on date input box
                        else if (mousePos.x >= 840 && mousePos.x <= 1160 && mousePos.y >= 255 && mousePos.y <= 285)
                        {
                            if (!subgraphDateInput.getIsActive())
                            {
                                subgraphDateInput.activate();
                                subgraphCompanyInput.deactivate();
                            }
                        }
                    }

                    subgraphCompanyInput.handleEvent(event);
                    subgraphDateInput.handleEvent(event);

                    if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Return)
                    {
                        if (subgraphCompanyInput.getIsActive() && !subgraphCompanyInput.getText().empty())
                        {
                            subgraphFilterCompany = subgraphCompanyInput.getText();
                            mapVisualizer.setCompanyFilter(subgraphFilterCompany);
                            subgraphCompanyInput.deactivate();
                            cout << "Company filter set: " << subgraphFilterCompany << endl;
                        }
                        else if (subgraphDateInput.getIsActive() && !subgraphDateInput.getText().empty())
                        {
                            subgraphFilterDate = subgraphDateInput.getText();
                            mapVisualizer.setDateFilter(subgraphFilterDate);
                            subgraphDateInput.deactivate();
                            cout << "Date filter set: " << subgraphFilterDate << endl;
                        }
                    }
                }
            }

            // Handle docking and layover input
            if (selectedOption == DOCKING_LAYOVER)
            {
                if (dockingState == DOCKING_ADD_SHIP || dockingState == DOCKING_REMOVE_SHIP || dockingState == DOCKING_VIEW_PORT)
                {
                    dockingPortInput.handleEvent(event);
                    if (dockingState == DOCKING_ADD_SHIP)
                    {
                        dockingShipInput.handleEvent(event);
                    }
                }

                if (event.type == sf::Event::KeyPressed)
                {
                    if (event.key.code == sf::Keyboard::Num1)
                    {
                        dockingState = DOCKING_VIEW_QUEUES;
                        dockingPortInput.deactivate();
                        dockingShipInput.deactivate();
                        dockingErrorMessage = "";
                    }
                    else if (event.key.code == sf::Keyboard::Num2)
                    {
                        dockingState = DOCKING_ADD_SHIP;
                        dockingPortInput.clear();
                        dockingShipInput.clear();
                        dockingPortInput.activate();
                        dockingErrorMessage = "";
                    }
                    else if (event.key.code == sf::Keyboard::Num3)
                    {
                        dockingState = DOCKING_REMOVE_SHIP;
                        dockingPortInput.clear();
                        dockingPortInput.activate();
                        dockingErrorMessage = "";
                    }
                    else if (event.key.code == sf::Keyboard::Num4)
                    {
                        dockingState = DOCKING_VIEW_PORT;
                        dockingPortInput.clear();
                        dockingPortInput.activate();
                        dockingErrorMessage = "";
                    }
                    else if (event.key.code == sf::Keyboard::Return)
                    {
                        if (dockingState == DOCKING_ADD_SHIP)
                        {
                            if (dockingPortInput.getIsActive() && !dockingPortInput.getText().empty())
                            {
                                selectedDockingPort = dockingPortInput.getText();
                                if (!maritimeGraph.hasPort(selectedDockingPort))
                                {
                                    dockingErrorMessage = "Port '" + selectedDockingPort + "' not found!";
                                    dockingPortInput.clear();
                                }
                                else
                                {
                                    dockingPortInput.deactivate();
                                    dockingShipInput.activate();
                                }
                            }
                            else if (dockingShipInput.getIsActive() && !dockingShipInput.getText().empty())
                            {
                                string shipName = dockingShipInput.getText();
                                maritimeGraph.addShipToQueue(selectedDockingPort, shipName);
                                cout << "Added ship '" << shipName << "' to queue at " << selectedDockingPort << endl;
                                dockingShipInput.clear();
                                dockingShipInput.deactivate();
                                dockingState = DOCKING_VIEW_QUEUES;
                                dockingErrorMessage = "";
                            }
                        }
                        else if (dockingState == DOCKING_REMOVE_SHIP)
                        {
                            if (!dockingPortInput.getText().empty())
                            {
                                string portName = dockingPortInput.getText();
                                if (!maritimeGraph.hasPort(portName))
                                {
                                    dockingErrorMessage = "Port '" + portName + "' not found!";
                                }
                                else if (maritimeGraph.getQueueSize(portName) == 0)
                                {
                                    dockingErrorMessage = "No ships in queue at " + portName + "!";
                                }
                                else
                                {
                                    bool removed = maritimeGraph.removeShipFromQueue(portName);
                                    if (removed)
                                    {
                                        cout << "Removed ship from queue at " << portName << endl;
                                        dockingErrorMessage = "";
                                    }
                                    dockingPortInput.clear();
                                    dockingPortInput.deactivate();
                                    dockingState = DOCKING_VIEW_QUEUES;
                                }
                            }
                        }
                        else if (dockingState == DOCKING_VIEW_PORT)
                        {
                            if (!dockingPortInput.getText().empty())
                            {
                                selectedDockingPort = dockingPortInput.getText();
                                if (!maritimeGraph.hasPort(selectedDockingPort))
                                {
                                    dockingErrorMessage = "Port '" + selectedDockingPort + "' not found!";
                                    dockingPortInput.clear();
                                }
                                else
                                {
                                    dockingPortInput.deactivate();
                                    dockingErrorMessage = "";
                                }
                            }
                        }
                    }
                }
            }

            // Handle multi-leg route input
            if (selectedOption == MULTI_LEG_ROUTE)
            {
                multiPortInput.handleEvent(event);

                if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Return)
                {
                    if (multiLegState == MULTI_INPUT_ORIGIN && !multiPortInput.getText().empty())
                    {
                        multiOrigin = multiPortInput.getText();
                        multiPortInput.clear();
                        multiLegState = MULTI_INPUT_INTERMEDIATE;
                        cout << "Origin: " << multiOrigin << endl;
                        cout << "Enter intermediate ports (press ENTER with empty to finish, or type port name)" << endl;
                    }
                    else if (multiLegState == MULTI_INPUT_INTERMEDIATE)
                    {
                        if (!multiPortInput.getText().empty())
                        {
                            // Add intermediate port
                            intermediatePorts.push_back(multiPortInput.getText());
                            cout << "Intermediate port added: " << multiPortInput.getText() << endl;
                            cout << "Enter another intermediate port (or press ENTER with empty to continue)" << endl;
                            multiPortInput.clear();
                        }
                        else
                        {
                            // No more intermediate ports, move to destination
                            multiLegState = MULTI_INPUT_DESTINATION;
                            cout << "Enter destination port..." << endl;
                        }
                    }
                    else if (multiLegState == MULTI_INPUT_DESTINATION && !multiPortInput.getText().empty())
                    {
                        multiDestination = multiPortInput.getText();
                        multiPortInput.clear();
                        multiLegState = MULTI_INPUT_DATE;
                        cout << "Destination: " << multiDestination << endl;
                        cout << "Enter date..." << endl;
                    }
                    else if (multiLegState == MULTI_INPUT_DATE && !multiPortInput.getText().empty())
                    {
                        multiDate = multiPortInput.getText();
                        multiPortInput.deactivate();
                        multiLegState = MULTI_VALIDATING;
                        multiMessageTimer.restart();
                        cout << "Date: " << multiDate << endl;
                        cout << "Validating..." << endl;
                    }
                }
            }

            // Handle booking input
            if (selectedOption == BOOK_WITHOUT_PREFERENCE)
            {
                originInput.handleEvent(event);
                destinationInput.handleEvent(event);
                dateInput.handleEvent(event);
                routeTypeInput.handleEvent(event); // NEW: Handle route type selection

                // Toggle bidirectional search
                if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::B)
                {
                    useBidirectional = !useBidirectional;
                    cout << "Bidirectional search: " << (useBidirectional ? "ON" : "OFF") << endl;
                }
                // Toggle step-by-step visualization
                if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::S)
                {
                    showStepByStep = !showStepByStep;
                    cout << "Step-by-step visualization: " << (showStepByStep ? "ON" : "OFF") << endl;
                }

                if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Return)
                {
                    if (bookingState == INPUT_ORIGIN && !originInput.getText().empty())
                    {
                        origin = originInput.getText();
                        originInput.deactivate();
                        destinationInput.activate();
                        bookingState = INPUT_DESTINATION;
                        cout << "Origin: " << origin << endl;
                    }
                    else if (bookingState == INPUT_DESTINATION && !destinationInput.getText().empty())
                    {
                        destination = destinationInput.getText();
                        destinationInput.deactivate();
                        dateInput.activate();
                        bookingState = INPUT_DATE;
                        cout << "Destination: " << destination << endl;
                    }
                    else if (bookingState == INPUT_DATE && !dateInput.getText().empty())
                    {
                        date = dateInput.getText();
                        dateInput.deactivate();
                        useShortestPath = false;             // Reset to cheapest path by default
                        bookingState = SELECTING_ROUTE_TYPE; // NEW: Ask user which path type
                        routeTypeInput.clear();
                        routeTypeInput.activate();
                        messageTimer.restart();
                        cout << "Date: " << date << endl;
                        cout << "Please choose route type (1 for Cheapest, 2 for Shortest): " << endl;
                    }
                    else if (bookingState == SELECTING_ROUTE_TYPE && !routeTypeInput.getText().empty())
                    {
                        string choice = routeTypeInput.getText();
                        routeTypeInput.deactivate();

                        if (choice == "1")
                        {
                            useShortestPath = false;
                            cout << "Route type: CHEAPEST PATH" << endl;
                        }
                        else if (choice == "2")
                        {
                            useShortestPath = true;
                            cout << "Route type: SHORTEST PATH" << endl;
                        }
                        else
                        {
                            cout << "Invalid choice. Using CHEAPEST PATH by default." << endl;
                            useShortestPath = false;
                        }

                        bookingState = VALIDATING;
                        messageTimer.restart();
                        cout << "Validating..." << endl;
                    }
                }
            }

            // Handle preference booking input - NEW FLOW: Preferences first, then inputs
            if (selectedOption == BOOK_WITH_PREFERENCE)
            {
                // Phase 1: Get preferences and show live map filtering
                if (prefBookingState == PREF_INPUT_PREFERENCES)
                {
                    preferenceInput.handleEvent(event);
                }
                // Phase 2: Get origin/destination/date after preferences are set
                else if (prefBookingState == PREF_FILTERING_MAP ||
                         prefBookingState == PREF_INPUT_ORIGIN ||
                         prefBookingState == PREF_INPUT_DESTINATION ||
                         prefBookingState == PREF_INPUT_DATE ||
                         prefBookingState == PREF_SELECTING_ROUTE_TYPE)
                {
                    originInput.handleEvent(event);
                    destinationInput.handleEvent(event);
                    dateInput.handleEvent(event);
                    routeTypeInput.handleEvent(event);  // NEW: Handle route type selection input
                }

                if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Return)
                {
                    // Phase 1: Processing preferences
                    if (prefBookingState == PREF_INPUT_PREFERENCES)
                    {
                        if (preferenceInput.processEnter())
                        {
                            currentPreferences = preferenceInput.getPreferences();
                            cout << "Preferences confirmed! Now enter route details..." << endl;

                            // Apply company filter to map
                            if (currentPreferences.hasCompanyPreference &&
                                currentPreferences.preferredCompanies.getSize() > 0)
                            {
                                mapVisualizer.setCompanyFilters(currentPreferences.preferredCompanies);
                                cout << "Map filtered to show only selected shipping companies" << endl;
                            }

                            prefBookingState = PREF_INPUT_ORIGIN;
                            originInput.clear();
                            originInput.activate();
                            cout << "Enter origin port..." << endl;
                        }
                        else
                        {
                            // Real-time map update as preferences are being entered
                            PreferenceFilter partialPrefs = preferenceInput.getPreferences();
                            if (partialPrefs.hasCompanyPreference &&
                                partialPrefs.preferredCompanies.getSize() > 0)
                            {
                                mapVisualizer.setCompanyFilters(partialPrefs.preferredCompanies);
                            }
                        }
                    }
                    // Phase 2: Get origin after preferences
                    else if (prefBookingState == PREF_INPUT_ORIGIN && !originInput.getText().empty())
                    {
                        prefOrigin = originInput.getText();
                        originInput.deactivate();
                        destinationInput.activate();
                        prefBookingState = PREF_INPUT_DESTINATION;
                        cout << "Preference Origin: " << prefOrigin << endl;
                    }
                    // Get destination
                    else if (prefBookingState == PREF_INPUT_DESTINATION && !destinationInput.getText().empty())
                    {
                        prefDestination = destinationInput.getText();
                        destinationInput.deactivate();
                        dateInput.activate();
                        prefBookingState = PREF_INPUT_DATE;
                        cout << "Preference Destination: " << prefDestination << endl;
                    }
                    // Get date and validate
                    else if (prefBookingState == PREF_INPUT_DATE && !dateInput.getText().empty())
                    {
                        prefDate = dateInput.getText();
                        dateInput.deactivate();
                        prefUseShortestPath = false;                  // Reset to cheapest path by default
                        prefBookingState = PREF_SELECTING_ROUTE_TYPE; // NEW: Ask user which path type
                        routeTypeInput.clear();
                        routeTypeInput.activate();
                        prefMessageTimer.restart();
                        cout << "Preference Date: " << prefDate << endl;
                        cout << "Please choose route type (1 for Cheapest, 2 for Shortest): " << endl;
                    }
                    else if (prefBookingState == PREF_SELECTING_ROUTE_TYPE && !routeTypeInput.getText().empty())
                    {
                        string choice = routeTypeInput.getText();
                        routeTypeInput.deactivate();

                        if (choice == "1")
                        {
                            prefUseShortestPath = false;
                            cout << "Route type: CHEAPEST PATH" << endl;
                        }
                        else if (choice == "2")
                        {
                            prefUseShortestPath = true;
                            cout << "Route type: SHORTEST PATH" << endl;
                        }
                        else
                        {
                            cout << "Invalid choice. Using CHEAPEST PATH by default." << endl;
                            prefUseShortestPath = false;
                        }

                        prefBookingState = PREF_VALIDATING;
                        prefMessageTimer.restart();
                        cout << "Validating..." << endl;
                    }
                }
            }
        }

        // Update logic
        if (selectedOption == SHOW_MAP)
        {
            mapVisualizer.update(window);
        }
        else if (selectedOption == BOOK_WITHOUT_PREFERENCE)
        {
            if (bookingState == VALIDATING)
            {
                errorMessage = "";

                if (!maritimeGraph.hasPort(origin))
                {
                    errorMessage = "Error: Origin port '" + origin + "' not found!";
                    cout << errorMessage << endl;
                    bookingState = INPUT_ORIGIN;
                    originInput.clear();
                    originInput.activate();
                }
                else if (!maritimeGraph.hasPort(destination))
                {
                    errorMessage = "Error: Destination port '" + destination + "' not found!";
                    cout << errorMessage << endl;
                    bookingState = INPUT_DESTINATION;
                    destinationInput.clear();
                    destinationInput.activate();
                }
                else
                {
                    cout << "Validation successful!" << endl;
                    bookingState = FINDING_ALL_PATHS;
                    messageTimer.restart();
                }
            }
            else if (bookingState == FINDING_ALL_PATHS)
            {
                cout << "Finding all possible paths..." << endl;
                allPaths = pathFinder.findAllPaths(origin, destination, date);

                LinkedList<Route> connectingRoutes =
                    pathFinder.getAllConnectingRoutes(origin, destination, date);
                bookingVisualizer.showConnectingRoutes(origin, destination, connectingRoutes, date);

                bookingState = SHOWING_ALL_ROUTES;
                messageTimer.restart();
            }
            else if (bookingState == SHOWING_ALL_ROUTES)
            {
                if (messageTimer.getElapsedTime().asSeconds() > 3.0f)
                {
                    if (showStepByStep)
                    {
                        bookingState = SHOWING_ALGORITHM_STEPS;
                        currentAlgorithmStep = 0;
                        currentVisitedPorts.clear();
                        currentProcessingPorts.clear();
                        currentProcessingPortName = origin; // Start with origin
                        messageTimer.restart();
                        cout << "Starting step-by-step algorithm visualization..." << endl;
                    }
                    else
                    {
                        bookingState = CALCULATING_OPTIMAL;
                        messageTimer.restart();
                        cout << "Calculating optimal path using Dijkstra..." << endl;
                    }
                }
            }
            else if (bookingState == SHOWING_ALGORITHM_STEPS)
            {
                // Simulate algorithm steps for visualization
                if (messageTimer.getElapsedTime().asSeconds() > 0.8f)
                {
                    messageTimer.restart();
                    currentAlgorithmStep++;

                    // Simulate algorithm progress (simplified)
                    if (currentAlgorithmStep == 1)
                    {
                        currentVisitedPorts.push_back(origin);
                        currentProcessingPortName = origin;
                    }
                    else if (currentAlgorithmStep <= 5)
                    {
                        // Simulate processing neighbors
                        LinkedList<Route> routes = maritimeGraph.getRoutesFromOnDate(currentProcessingPortName, date);
                        if (routes.getSize() > 0 && currentAlgorithmStep < routes.getSize() + 2)
                        {
                            currentProcessingPorts.push_back(routes.get(currentAlgorithmStep - 2).destination);
                        }
                    }
                    else if (currentAlgorithmStep > 8)
                    {
                        // Move to calculation after showing steps
                        bookingState = CALCULATING_OPTIMAL;
                        bookingVisualizer.clearAlgorithmSteps();
                        currentVisitedPorts.clear();
                        currentProcessingPorts.clear();
                        currentProcessingPortName = "";
                    }
                }
            }
            else if (bookingState == CALCULATING_OPTIMAL)
            {
                // Use appropriate pathfinder based on user's choice
                if (useShortestPath)
                {
                    cout << "Finding SHORTEST path using Dijkstra..." << endl;
                    currentPath = shortestPathFinder.findShortestPath(origin, destination, date);
                }
                else
                {
                    // Use bidirectional search if enabled for cheapest path
                    if (useBidirectional)
                    {
                        cout << "Finding CHEAPEST path using Bidirectional Dijkstra..." << endl;
                        currentPath = pathFinder.findCheapestPathBidirectional(origin, destination, date);
                    }
                    else
                    {
                        cout << "Finding CHEAPEST path using Dijkstra..." << endl;
                        currentPath = pathFinder.findCheapestPath(origin, destination, date);
                    }
                }

                if (currentPath.found)
                {
                    cout << "Optimal path found! Total cost: $" << currentPath.totalCost << endl;
                    if (!useShortestPath && useBidirectional)
                    {
                        cout << "Used bidirectional Dijkstra for efficiency!" << endl;
                    }
                    bookingVisualizer.startPathAnimation(currentPath);
                    bookingState = SHOWING_OPTIMAL_PATH;
                }
                else
                {
                    cout << "No complete path found!" << endl;

                    if (allPaths.getSize() > 0)
                    {
                        cout << "But " << allPaths.getSize() << " partial paths exist" << endl;
                    }

                    bookingState = SHOWING_OPTIMAL_PATH;
                    currentPath.totalCost = 0;
                }
            }
            else if (bookingState == SHOWING_OPTIMAL_PATH)
            {
                bookingVisualizer.update();

                if (bookingVisualizer.isAnimationComplete())
                {
                    bookingState = BOOKING_COMPLETE;
                    cout << "Booking visualization complete!" << endl;
                }
            }
        }
        else if (selectedOption == MULTI_LEG_ROUTE)
        {
            if (multiLegState == MULTI_VALIDATING)
            {
                multiErrorMessage = "";

                if (!maritimeGraph.hasPort(multiOrigin))
                {
                    multiErrorMessage = "Error: Origin port '" + multiOrigin + "' not found!";
                    cout << multiErrorMessage << endl;
                    multiLegState = MULTI_INPUT_ORIGIN;
                    multiPortInput.clear();
                    multiPortInput.activate();
                }
                else if (!maritimeGraph.hasPort(multiDestination))
                {
                    multiErrorMessage = "Error: Destination port '" + multiDestination + "' not found!";
                    cout << multiErrorMessage << endl;
                    multiLegState = MULTI_INPUT_DESTINATION;
                    multiPortInput.clear();
                    multiPortInput.activate();
                }
                else
                {
                    // Validate intermediate ports
                    bool allValid = true;
                    for (int i = 0; i < intermediatePorts.getSize(); i++)
                    {
                        if (!maritimeGraph.hasPort(intermediatePorts.get(i)))
                        {
                            multiErrorMessage = "Error: Intermediate port '" + intermediatePorts.get(i) + "' not found!";
                            cout << multiErrorMessage << endl;
                            allValid = false;
                            break;
                        }
                    }
                    if (allValid)
                    {
                        cout << "Validation successful!" << endl;
                        multiLegState = MULTI_CALCULATING;
                        multiMessageTimer.restart();
                    }
                    else
                    {
                        multiLegState = MULTI_INPUT_INTERMEDIATE;
                        multiPortInput.clear();
                        multiPortInput.activate();
                    }
                }
            }
            else if (multiLegState == MULTI_CALCULATING)
            {
                cout << "Calculating multi-leg route..." << endl;
                multiLegPath = pathFinder.findMultiLegRoute(multiOrigin, intermediatePorts, multiDestination, multiDate);

                if (multiLegPath.found)
                {
                    cout << "Multi-leg route found! Total cost: $" << multiLegPath.totalCost << endl;

                    // Show all routes on map with multi-leg route highlighted
                    LinkedList<Route> allConnectingRoutes;
                    for (int i = 0; i < multiLegPath.routes.getSize(); i++)
                    {
                        allConnectingRoutes.push_back(multiLegPath.routes.get(i));
                    }
                    bookingVisualizer.showConnectingRoutes(multiOrigin, multiDestination, allConnectingRoutes, multiDate);
                    bookingVisualizer.startPathAnimation(multiLegPath);
                    multiLegState = MULTI_SHOWING_ROUTE;
                }
                else
                {
                    cout << "No complete multi-leg route found!" << endl;
                    // Still show map with all routes
                    LinkedList<Route> emptyRoutes;
                    bookingVisualizer.showConnectingRoutes(multiOrigin, multiDestination, emptyRoutes, multiDate);
                    multiLegState = MULTI_SHOWING_ROUTE;
                    multiLegPath.totalCost = 0;
                }
            }
            else if (multiLegState == MULTI_SHOWING_ROUTE)
            {
                bookingVisualizer.update();
                if (bookingVisualizer.isAnimationComplete())
                {
                    multiLegState = MULTI_COMPLETE;
                    cout << "Multi-leg route visualization complete!" << endl;
                }
            }
        }
        else if (selectedOption == BOOK_WITH_PREFERENCE)
        {
            // NEW FLOW: Preferences are entered FIRST, then route details, then Dijkstra
            if (prefBookingState == PREF_VALIDATING)
            {
                prefErrorMessage = "";

                if (!maritimeGraph.hasPort(prefOrigin))
                {
                    prefErrorMessage = "Error: Origin port '" + prefOrigin + "' not found!";
                    cout << prefErrorMessage << endl;
                    prefBookingState = PREF_INPUT_ORIGIN;
                    originInput.clear();
                    originInput.activate();
                }
                else if (!maritimeGraph.hasPort(prefDestination))
                {
                    prefErrorMessage = "Error: Destination port '" + prefDestination + "' not found!";
                    cout << prefErrorMessage << endl;
                    prefBookingState = PREF_INPUT_DESTINATION;
                    destinationInput.clear();
                    destinationInput.activate();
                }
                else
                {
                    cout << "Validation successful! Showing filtered routes..." << endl;
                    // Show connecting routes filtered by preferences
                    prefConnectingRoutes = pathFinder.getAllConnectingRoutesWithPreferences(
                        prefOrigin, prefDestination, prefDate, currentPreferences);
                    bookingVisualizer.showConnectingRoutes(prefOrigin, prefDestination, prefConnectingRoutes, prefDate);
                    prefBookingState = PREF_SHOWING_CONNECTING_ROUTES;
                    prefMessageTimer.restart();
                }
            }
            else if (prefBookingState == PREF_SHOWING_CONNECTING_ROUTES)
            {
                if (prefMessageTimer.getElapsedTime().asSeconds() > 2.0f)
                {
                    prefBookingState = PREF_CALCULATING_OPTIMAL;
                    cout << "Proceeding to calculate optimal path..." << endl;
                }
            }
            else if (prefBookingState == PREF_CALCULATING_OPTIMAL)
            {
                if (prefUseShortestPath)
                {
                    cout << "Calculating SHORTEST path with preferences using Dijkstra..." << endl;
                    prefCurrentPath = shortestPathFinder.findShortestPathWithPreferences(
                        prefOrigin, prefDestination, prefDate, currentPreferences);
                }
                else
                {
                    cout << "Calculating CHEAPEST path with preferences using Dijkstra..." << endl;
                    prefCurrentPath = pathFinder.findCheapestPathWithPreferences(
                        prefOrigin, prefDestination, prefDate, currentPreferences);
                }

                if (prefCurrentPath.found)
                {
                    cout << "Optimal path found! Total cost: $" << prefCurrentPath.totalCost << endl;
                    bookingVisualizer.startPathAnimation(prefCurrentPath);
                    prefBookingState = PREF_SHOWING_OPTIMAL_PATH;
                }
                else
                {
                    cout << "No path found with given preferences!" << endl;
                    prefBookingState = PREF_SHOWING_OPTIMAL_PATH;
                    prefCurrentPath.totalCost = 0;
                }
            }
            else if (prefBookingState == PREF_SHOWING_OPTIMAL_PATH)
            {
                bookingVisualizer.update();

                if (bookingVisualizer.isAnimationComplete())
                {
                    prefBookingState = PREF_BOOKING_COMPLETE;
                    cout << "Preference booking visualization complete!" << endl;
                }
            }
        }

        window.clear(sf::Color(20, 30, 50));

        if (showMenu)
        {
            // ==================== MAIN MENU ====================

            sf::RectangleShape menuBg(sf::Vector2f(1200, 800));
            menuBg.setFillColor(sf::Color(15, 25, 40));
            window.draw(menuBg);

            sf::RectangleShape headerBar(sf::Vector2f(1200, 120));
            headerBar.setFillColor(sf::Color(20, 40, 70));
            window.draw(headerBar);

            sf::Text title("OCEANROUTE NAV", font, 60);
            title.setFillColor(sf::Color(100, 200, 255));
            title.setStyle(sf::Text::Bold);
            title.setPosition(300, 30);
            window.draw(title);

            sf::Text subtitle("Maritime Navigation & Logistics System", font, 20);
            subtitle.setFillColor(sf::Color(150, 200, 255));
            subtitle.setPosition(400, 100);
            window.draw(subtitle);

            string options[] = {
                "1. Show Map - View All Ports & Routes",
                "2. Book A Ship (No Preference)",
                "3. Book A Ship (With Preference)",
                "4. Multi-Leg Route Generation",
                "5. Docking & Layover Management",
                "6. Exit Application"};

            // Get mouse position for hover effect
            sf::Vector2i mousePos = sf::Mouse::getPosition(window);
            int hoveredIndex = -1;
            if (mousePos.x >= 250 && mousePos.x <= 950 && mousePos.y >= 200 && mousePos.y <= 650)
            {
                hoveredIndex = (mousePos.y - 200) / 70;
                if (hoveredIndex < 0 || hoveredIndex >= 6)
                    hoveredIndex = -1;
            }

            for (int i = 0; i < 6; i++)
            {
                sf::RectangleShape optionBg(sf::Vector2f(700, 50));
                optionBg.setPosition(250, 200 + i * 70);

                // Hover effect
                if (i == hoveredIndex)
                {
                    optionBg.setFillColor(sf::Color(50, 80, 120, 200));
                    optionBg.setOutlineThickness(3);
                    optionBg.setOutlineColor(sf::Color(100, 200, 255));
                }
                else
                {
                    optionBg.setFillColor(sf::Color(30, 50, 80, 150));
                    optionBg.setOutlineThickness(2);
                    optionBg.setOutlineColor(sf::Color(50, 100, 150));
                }
                window.draw(optionBg);

                sf::Text optionText(options[i], font, 24);
                optionText.setFillColor(i == hoveredIndex ? sf::Color(200, 240, 255) : sf::Color::White);
                optionText.setPosition(270, 210 + i * 70);
                window.draw(optionText);
            }

            sf::Text instruction("Click on an option to select, or press ESC to exit", font, 18);
            instruction.setFillColor(sf::Color(200, 200, 200));
            instruction.setPosition(350, 730);
            window.draw(instruction);
        }
        else
        {
            // ==================== OPTION VIEWS ====================

            // Back button - Only show when inputs are inactive
            bool anyInputActive = originInput.getIsActive() ||
                                  destinationInput.getIsActive() ||
                                  dateInput.getIsActive() ||
                                  preferenceInput.isInputActive() ||
                                  multiPortInput.getIsActive() ||
                                  dockingPortInput.getIsActive() ||
                                  dockingShipInput.getIsActive() ||
                                  subgraphCompanyInput.getIsActive() ||
                                  subgraphDateInput.getIsActive();

            if (!anyInputActive)
            {
                sf::RectangleShape backButton(sf::Vector2f(250, 40));
                backButton.setPosition(20, 20);
                backButton.setFillColor(sf::Color(100, 50, 50));
                backButton.setOutlineThickness(2);
                backButton.setOutlineColor(sf::Color::Red);
                window.draw(backButton);

                sf::Text backText("Press ESC - Return to Menu", font, 16);
                backText.setFillColor(sf::Color::White);
                backText.setPosition(35, 28);
                window.draw(backText);
            }

            switch (selectedOption)
            {
            case SHOW_MAP:
            {
                mapVisualizer.update(window);
                mapVisualizer.render(window);

                sf::RectangleShape titleBar(sf::Vector2f(400, 50));
                titleBar.setPosition(400, 20);
                titleBar.setFillColor(sf::Color(0, 0, 0, 180));
                titleBar.setOutlineThickness(2);
                titleBar.setOutlineColor(sf::Color::Cyan);
                window.draw(titleBar);

                sf::Text mapTitle("Maritime Route Map", font, 24);
                mapTitle.setFillColor(sf::Color::Cyan);
                mapTitle.setPosition(450, 30);
                window.draw(mapTitle);

                // Subgraph filter menu
                if (showSubgraphMenu)
                {
                    sf::RectangleShape filterMenu(sf::Vector2f(350, 300));
                    filterMenu.setPosition(820, 80);
                    filterMenu.setFillColor(sf::Color(25, 35, 55, 240));
                    filterMenu.setOutlineThickness(3);
                    filterMenu.setOutlineColor(sf::Color(100, 200, 255));
                    window.draw(filterMenu);

                    sf::Text filterTitle("Subgraph Filter", font, 20);
                    filterTitle.setFillColor(sf::Color(100, 200, 255));
                    filterTitle.setStyle(sf::Text::Bold);
                    filterTitle.setPosition(830, 90);
                    window.draw(filterTitle);

                    sf::Text instruction1("Press F to toggle", font, 12);
                    instruction1.setFillColor(sf::Color(200, 200, 200));
                    instruction1.setPosition(830, 115);
                    window.draw(instruction1);

                    sf::Text instruction2("Press C to clear", font, 12);
                    instruction2.setFillColor(sf::Color(200, 200, 200));
                    instruction2.setPosition(830, 130);
                    window.draw(instruction2);

                    // Company filter
                    sf::Text companyLabel("Filter by Company:", font, 14);
                    companyLabel.setFillColor(sf::Color::White);
                    companyLabel.setPosition(840, 160);
                    window.draw(companyLabel);

                    // Company filter input box
                    sf::RectangleShape companyBox(sf::Vector2f(320, 30));
                    companyBox.setPosition(840, 185);
                    companyBox.setFillColor(sf::Color(20, 30, 45));
                    companyBox.setOutlineThickness(subgraphCompanyInput.getIsActive() ? 2 : 1);
                    companyBox.setOutlineColor(subgraphCompanyInput.getIsActive() ? sf::Color(100, 200, 255) : sf::Color(100, 100, 120));
                    window.draw(companyBox);

                    if (subgraphCompanyInput.getIsActive())
                    {
                        subgraphCompanyInput.render(window, font, 840, 185, "");
                    }
                    else
                    {
                        sf::Text companyValue(subgraphFilterCompany.empty() ? "Enter company name" : subgraphFilterCompany, font, 13);
                        companyValue.setFillColor(subgraphFilterCompany.empty() ? sf::Color(150, 150, 150) : sf::Color(100, 255, 100));
                        companyValue.setPosition(845, 190);
                        window.draw(companyValue);
                    }

                    // Date filter
                    sf::Text dateLabel("Filter by Date:", font, 14);
                    dateLabel.setFillColor(sf::Color::White);
                    dateLabel.setPosition(840, 230);
                    window.draw(dateLabel);

                    // Date filter input
                    sf::RectangleShape dateBox(sf::Vector2f(320, 30));
                    dateBox.setPosition(840, 255);
                    dateBox.setFillColor(sf::Color(20, 30, 45));
                    dateBox.setOutlineThickness(subgraphDateInput.getIsActive() ? 2 : 1);
                    dateBox.setOutlineColor(subgraphDateInput.getIsActive() ? sf::Color(100, 200, 255) : sf::Color(100, 100, 120));
                    window.draw(dateBox);

                    if (subgraphDateInput.getIsActive())
                    {
                        subgraphDateInput.render(window, font, 840, 255, "");
                    }
                    else
                    {
                        sf::Text dateValue(subgraphFilterDate.empty() ? "DD/MM/YYYY" : subgraphFilterDate, font, 13);
                        dateValue.setFillColor(subgraphFilterDate.empty() ? sf::Color(150, 150, 150) : sf::Color(100, 255, 100));
                        dateValue.setPosition(845, 260);
                        window.draw(dateValue);
                    }
                }
                else
                {
                    // Show filter toggle hint
                    sf::Text filterHint("Press F to filter subgraph", font, 14);
                    filterHint.setFillColor(sf::Color(200, 200, 200));
                    filterHint.setPosition(820, 80);
                    window.draw(filterHint);
                }

                sf::RectangleShape legendBox(sf::Vector2f(250, 150));
                legendBox.setPosition(930, 20);
                legendBox.setFillColor(sf::Color(0, 0, 0, 180));
                legendBox.setOutlineThickness(2);
                legendBox.setOutlineColor(sf::Color::White);
                window.draw(legendBox);

                sf::Text legendTitle("Legend", font, 18);
                legendTitle.setFillColor(sf::Color::White);
                legendTitle.setStyle(sf::Text::Bold);
                legendTitle.setPosition(945, 30);
                window.draw(legendTitle);

                sf::CircleShape portDot(5);
                portDot.setPosition(945, 65);
                portDot.setFillColor(sf::Color(255, 200, 50));
                portDot.setOutlineThickness(1);
                portDot.setOutlineColor(sf::Color::White);
                window.draw(portDot);

                sf::Text portLabel("Ports", font, 14);
                portLabel.setPosition(960, 63);
                portLabel.setFillColor(sf::Color::White);
                window.draw(portLabel);

                sf::RectangleShape cheapRoute(sf::Vector2f(20, 3));
                cheapRoute.setPosition(945, 95);
                cheapRoute.setFillColor(sf::Color(0, 255, 0));
                window.draw(cheapRoute);

                sf::Text cheapLabel("Cheap Routes", font, 14);
                cheapLabel.setPosition(970, 90);
                cheapLabel.setFillColor(sf::Color::White);
                window.draw(cheapLabel);

                sf::RectangleShape expensiveRoute(sf::Vector2f(20, 3));
                expensiveRoute.setPosition(945, 120);
                expensiveRoute.setFillColor(sf::Color(255, 0, 0));
                window.draw(expensiveRoute);

                sf::Text expensiveLabel("Expensive Routes", font, 14);
                expensiveLabel.setPosition(970, 115);
                expensiveLabel.setFillColor(sf::Color::White);
                window.draw(expensiveLabel);

                sf::Text hoverText("Hover over routes for details", font, 12);
                hoverText.setPosition(945, 145);
                hoverText.setFillColor(sf::Color(200, 200, 200));
                window.draw(hoverText);

                break;
            }

            case BOOK_WITHOUT_PREFERENCE:
            {
                sf::Text title("Book Ship Without Preference", font, 30);
                title.setFillColor(sf::Color::Cyan);
                title.setPosition(350, 80);
                window.draw(title);

                if (bookingState == INPUT_ORIGIN || bookingState == INPUT_DESTINATION ||
                    bookingState == INPUT_DATE || bookingState == SELECTING_ROUTE_TYPE)
                {
                    sf::RectangleShape formBg(sf::Vector2f(600, 300));
                    formBg.setPosition(280, 200);
                    formBg.setFillColor(sf::Color(30, 40, 60, 200));
                    formBg.setOutlineThickness(2);
                    formBg.setOutlineColor(sf::Color::Cyan);
                    window.draw(formBg);

                    if (bookingState == SELECTING_ROUTE_TYPE)
                    {
                        // Show route type selection
                        sf::Text routeTypeTitle("SELECT ROUTE TYPE", font, 20);
                        routeTypeTitle.setFillColor(sf::Color::Yellow);
                        routeTypeTitle.setPosition(350, 220);
                        window.draw(routeTypeTitle);

                        sf::Text option1("1. CHEAPEST PATH - Minimize total cost", font, 14);
                        option1.setFillColor(sf::Color::White);
                        option1.setPosition(320, 280);
                        window.draw(option1);

                        sf::Text option2("2. SHORTEST PATH - Minimize number of routes", font, 14);
                        option2.setFillColor(sf::Color::White);
                        option2.setPosition(320, 320);
                        window.draw(option2);

                        sf::Text instrText("Enter your choice (1 or 2):", font, 14);
                        instrText.setFillColor(sf::Color::Cyan);
                        instrText.setPosition(320, 380);
                        window.draw(instrText);

                        routeTypeInput.render(window, font, 320, 420, "Choice:");
                    }
                    else
                    {
                        originInput.render(window, font, 320, 240, "Origin:");
                        destinationInput.render(window, font, 320, 300, "Destination:");
                        dateInput.render(window, font, 320, 360, "Date (DD/MM/YYYY):");

                        // Bidirectional search option
                        sf::RectangleShape bidirBox(sf::Vector2f(25, 25));
                        bidirBox.setPosition(320, 410);
                        bidirBox.setFillColor(useBidirectional ? sf::Color(100, 255, 100) : sf::Color(50, 50, 50));
                        bidirBox.setOutlineThickness(2);
                        bidirBox.setOutlineColor(sf::Color::White);
                        window.draw(bidirBox);

                        if (useBidirectional)
                        {
                            sf::Text checkmark("✓", font, 20);
                            checkmark.setFillColor(sf::Color::White);
                            checkmark.setPosition(325, 405);
                            window.draw(checkmark);
                        }

                        sf::Text bidirLabel("Use Bidirectional Search (Press B to toggle)", font, 14);
                        bidirLabel.setFillColor(sf::Color::White);
                        bidirLabel.setPosition(355, 413);
                        window.draw(bidirLabel);

                        // Step-by-step visualization option
                        sf::RectangleShape stepBox(sf::Vector2f(25, 25));
                        stepBox.setPosition(320, 445);
                        stepBox.setFillColor(showStepByStep ? sf::Color(100, 255, 100) : sf::Color(50, 50, 50));
                        stepBox.setOutlineThickness(2);
                        stepBox.setOutlineColor(sf::Color::White);
                        window.draw(stepBox);

                        if (showStepByStep)
                        {
                            sf::Text checkmark2("✓", font, 20);
                            checkmark2.setFillColor(sf::Color::White);
                            checkmark2.setPosition(325, 440);
                            window.draw(checkmark2);
                        }

                        sf::Text stepLabel("Show Step-by-Step Algorithm (Press S to toggle)", font, 14);
                        stepLabel.setFillColor(sf::Color::White);
                        stepLabel.setPosition(355, 448);
                        window.draw(stepLabel);

                        sf::Text instruction("Press ENTER to confirm each field", font, 16);
                        instruction.setFillColor(sf::Color(200, 200, 200));
                        instruction.setPosition(380, 485);
                        window.draw(instruction);

                        if (!errorMessage.empty())
                        {
                            sf::Text error(errorMessage, font, 18);
                            error.setFillColor(sf::Color::Red);
                            error.setPosition(320, 470);
                            window.draw(error);
                        }
                    }
                }
                else if (bookingState == FINDING_ALL_PATHS)
                {
                        sf::Text message("Finding all possible paths...", font, 28);
                        message.setFillColor(sf::Color::Yellow);
                        message.setPosition(380, 400);
                        window.draw(message);
                    }

                    else if (bookingState == SHOWING_ALL_ROUTES)
                    {
                        bookingVisualizer.render(window);

                        sf::Text message("Showing all possible connecting routes...", font, 24);
                        message.setFillColor(sf::Color::Yellow);
                        message.setPosition(350, 750);
                        window.draw(message);

                        stringstream pathCount;
                        pathCount << "Found " << allPaths.getSize() << " possible paths";
                        sf::Text pathCountText(pathCount.str(), font, 20);
                        pathCountText.setFillColor(sf::Color::White);
                        pathCountText.setPosition(450, 720);
                        window.draw(pathCountText);
                    }
                    else if (bookingState == SHOWING_ALGORITHM_STEPS)
                    {
                        // Update algorithm step visualization
                        bookingVisualizer.setAlgorithmSteps(currentVisitedPorts, currentProcessingPorts, currentProcessingPortName);
                        bookingVisualizer.render(window);

                        sf::Text message("Visualizing Dijkstra Algorithm Steps...", font, 24);
                        message.setFillColor(sf::Color::Yellow);
                        message.setPosition(350, 750);
                        window.draw(message);

                        sf::Text stepInfo("Step: " + to_string(currentAlgorithmStep), font, 18);
                        stepInfo.setFillColor(sf::Color::White);
                        stepInfo.setPosition(500, 720);
                        window.draw(stepInfo);

                        // Legend for step visualization
                        sf::RectangleShape legendBox(sf::Vector2f(250, 120));
                        legendBox.setPosition(15, 650);
                        legendBox.setFillColor(sf::Color(0, 0, 0, 200));
                        legendBox.setOutlineThickness(2);
                        legendBox.setOutlineColor(sf::Color::Cyan);
                        window.draw(legendBox);

                        sf::Text legendTitle("Algorithm Steps", font, 14);
                        legendTitle.setFillColor(sf::Color::Cyan);
                        legendTitle.setStyle(sf::Text::Bold);
                        legendTitle.setPosition(25, 655);
                        window.draw(legendTitle);

                        sf::CircleShape redDot(6);
                        redDot.setFillColor(sf::Color(255, 100, 100));
                        redDot.setPosition(25, 675);
                        window.draw(redDot);
                        sf::Text redLabel("Current", font, 11);
                        redLabel.setPosition(35, 673);
                        redLabel.setFillColor(sf::Color::White);
                        window.draw(redLabel);

                        sf::CircleShape yellowDot(6);
                        yellowDot.setFillColor(sf::Color(255, 255, 100));
                        yellowDot.setPosition(25, 695);
                        window.draw(yellowDot);
                        sf::Text yellowLabel("Processing", font, 11);
                        yellowLabel.setPosition(35, 693);
                        yellowLabel.setFillColor(sf::Color::White);
                        window.draw(yellowLabel);

                        sf::CircleShape blueDot(6);
                        blueDot.setFillColor(sf::Color(100, 150, 255));
                        blueDot.setPosition(25, 715);
                        window.draw(blueDot);
                        sf::Text blueLabel("Visited", font, 11);
                        blueLabel.setPosition(35, 713);
                        blueLabel.setFillColor(sf::Color::White);
                        window.draw(blueLabel);
                    }
                    else if (bookingState == CALCULATING_OPTIMAL)
                    {
                        bookingVisualizer.render(window);

                        string algoText = useBidirectional ? "Calculating path with BIDIRECTIONAL Dijkstra..." : "Calculating path with LEAST COST using Dijkstra...";
                        sf::Text message(algoText, font, 24);
                        message.setFillColor(sf::Color::Cyan);
                        message.setPosition(300, 750);
                        window.draw(message);
                    }
                    else if (bookingState == SHOWING_OPTIMAL_PATH || bookingState == BOOKING_COMPLETE)
                    {
                        bookingVisualizer.render(window);

                        // Calculate dynamic height based on layover count
                        int boxHeight = 250;
                        if (currentPath.found && currentPath.layovers.getSize() > 0)
                        {
                            boxHeight = 250 + (currentPath.layovers.getSize() * 25); // Add 25px per layover
                        }
                        sf::RectangleShape infoBox(sf::Vector2f(400, boxHeight));
                        infoBox.setPosition(15, 530);
                        infoBox.setFillColor(sf::Color(0, 0, 0, 200));
                        infoBox.setOutlineThickness(3);
                        infoBox.setOutlineColor(currentPath.found ? sf::Color(255, 215, 0) : sf::Color(255, 0, 0));
                        window.draw(infoBox);

                        string titleText = currentPath.found ? (useBidirectional ? "OPTIMAL ROUTE (Bidirectional Dijkstra)" : "OPTIMAL ROUTE (Dijkstra)") : "No Complete Path Found";
                        sf::Text infoTitle(titleText, font, 20);
                        infoTitle.setFillColor(currentPath.found ? sf::Color(255, 215, 0) : sf::Color(255, 0, 0));
                        infoTitle.setStyle(sf::Text::Bold);
                        infoTitle.setPosition(25, 540);
                        window.draw(infoTitle);

                        stringstream pathInfo;
                        pathInfo << "Origin: " << origin << "\n"
                                 << "Destination: " << destination << "\n"
                                 << "Date: " << date << "\n";

                        if (currentPath.found)
                        {
                            pathInfo << "Total Cost: $" << currentPath.totalCost << "\n"
                                     << "Travel Time: " << currentPath.totalTravelTime << " hours\n"
                                     << "  (" << (currentPath.totalTravelTime / 24) << " days " 
                                     << (currentPath.totalTravelTime % 24) << " hours)\n"
                                     << "Stops: " << (currentPath.path.getSize() - 1) << "\n"
                                     << "Total Paths Found: " << allPaths.getSize() << "\n\n"
                                     << "Optimal Route:\n";

                            for (int i = 0; i < currentPath.path.getSize(); i++)
                            {
                                pathInfo << currentPath.path.get(i);
                                if (i < currentPath.path.getSize() - 1)
                                {
                                    pathInfo << " -> ";
                                }
                            }

                            // Display layover information
                            if (currentPath.layovers.getSize() > 0)
                            {
                                pathInfo << "\n\nDocking at Ports:";
                                for (int i = 0; i < currentPath.layovers.getSize(); i++)
                                {
                                    const LayoverInfo &layover = currentPath.layovers.get(i);
                                    pathInfo << "\n  " << layover.portName << ":\n"
                                             << "    Arrived: " << layover.arrivalDate << " " << layover.arrivalTime << "\n"
                                             << "    Departed: " << layover.departureDate << " " << layover.departureTime << "\n"
                                             << "    Docked: " << layover.layoverHours << " hours";
                                    if (layover.layoverHours > 12)
                                    {
                                        pathInfo << " (Charge: $" << layover.portCharge << ")";
                                    }
                                    else
                                    {
                                        pathInfo << " (No charge)";
                                    }
                                }
                            }
                        }
                        else
                        {
                            pathInfo << "Status: No complete path\n";
                            if (allPaths.getSize() > 0)
                            {
                                pathInfo << "Partial paths: " << allPaths.getSize() << "\n";
                            }
                            else
                            {
                                pathInfo << "No routes available\n";
                            }
                        }

                        sf::Text pathText(pathInfo.str(), font, 14);
                        pathText.setFillColor(sf::Color::White);
                        pathText.setPosition(25, 575);
                        window.draw(pathText);
                    }

                    break;
                }

            case BOOK_WITH_PREFERENCE:
            {
                sf::Text title("Book Ship With Preference", font, 30);
                title.setFillColor(sf::Color::Cyan);
                title.setPosition(350, 80);
                window.draw(title);

                // NEW FLOW: Phase 1 - Input preferences first and show live map filtering
                if (prefBookingState == PREF_INPUT_PREFERENCES)
                {
                    // Draw the map in background
                    mapVisualizer.render(window);

                    // Title bar
                    sf::RectangleShape titleBg(sf::Vector2f(1200, 60));
                    titleBg.setFillColor(sf::Color(15, 25, 40, 230));
                    titleBg.setPosition(0, 0);
                    window.draw(titleBg);
                    title.setPosition(24, 16);
                    window.draw(title);

                    // Right rail for preferences
                    sf::RectangleShape rightPanel(sf::Vector2f(380, 840));
                    rightPanel.setPosition(800, 20);
                    rightPanel.setFillColor(sf::Color(15, 25, 45, 220));
                    rightPanel.setOutlineThickness(2);
                    rightPanel.setOutlineColor(sf::Color(80, 150, 220));
                    window.draw(rightPanel);

                    // Legend (top-left, compact)
                    sf::RectangleShape legendBox(sf::Vector2f(240, 110));
                    legendBox.setPosition(20, 80);
                    legendBox.setFillColor(sf::Color(0, 0, 0, 190));
                    legendBox.setOutlineThickness(2);
                    legendBox.setOutlineColor(sf::Color(100, 200, 255));
                    window.draw(legendBox);

                    sf::Text legendTitle("Map Legend", font, 15);
                    legendTitle.setFillColor(sf::Color(120, 220, 255));
                    legendTitle.setStyle(sf::Text::Bold);
                    legendTitle.setPosition(30, 88);
                    window.draw(legendTitle);

                    sf::CircleShape highlightedDot(6);
                    highlightedDot.setFillColor(sf::Color(255, 200, 50));
                    highlightedDot.setPosition(32, 118);
                    window.draw(highlightedDot);
                    sf::Text highlightLabel("Selected Companies", font, 12);
                    highlightLabel.setPosition(48, 114);
                    highlightLabel.setFillColor(sf::Color::White);
                    window.draw(highlightLabel);

                    sf::CircleShape dimmedDot(6);
                    dimmedDot.setFillColor(sf::Color(110, 110, 110, 140));
                    dimmedDot.setPosition(32, 144);
                    window.draw(dimmedDot);
                    sf::Text dimmedLabel("Other Routes", font, 12);
                    dimmedLabel.setPosition(48, 140);
                    dimmedLabel.setFillColor(sf::Color(180, 180, 180));
                    window.draw(dimmedLabel);

                    // Instruction (bottom-left)
                    sf::RectangleShape instructionBox(sf::Vector2f(520, 70));
                    instructionBox.setPosition(20, 700);
                    instructionBox.setFillColor(sf::Color(25, 40, 70, 210));
                    instructionBox.setOutlineThickness(1);
                    instructionBox.setOutlineColor(sf::Color(110, 170, 230, 130));
                    window.draw(instructionBox);

                    sf::Text instruction("Map updates live as you type preferences\nPress ENTER after each field to confirm", font, 14);
                    instruction.setFillColor(sf::Color(200, 220, 255));
                    instruction.setPosition(32, 712);
                    window.draw(instruction);

                    // Preference input panel (renders into right rail)
                    preferenceInput.render(window, font);

                    // Error message (bottom of right rail)
                    if (!prefErrorMessage.empty())
                    {
                        sf::RectangleShape errorBox(sf::Vector2f(340, 44));
                        errorBox.setPosition(820, 800);
                        errorBox.setFillColor(sf::Color(90, 25, 25, 230));
                        errorBox.setOutlineThickness(2);
                        errorBox.setOutlineColor(sf::Color(255, 120, 120));
                        window.draw(errorBox);

                        sf::Text error("⚠ " + prefErrorMessage, font, 14);
                        error.setFillColor(sf::Color(255, 180, 180));
                        error.setStyle(sf::Text::Bold);
                        error.setPosition(830, 728);
                        window.draw(error);
                    }
                }
                // Phase 2: After preferences, get origin/destination/date (with map still filtered)
                else if (prefBookingState == PREF_INPUT_ORIGIN ||
                    prefBookingState == PREF_INPUT_DESTINATION ||
                    prefBookingState == PREF_INPUT_DATE ||
                    prefBookingState == PREF_SELECTING_ROUTE_TYPE)
                {
                    // Show filtered map in background
                    mapVisualizer.render(window);

                    // Draw form on top
                    sf::RectangleShape formBg(sf::Vector2f(640, 420));
                    formBg.setPosition(260, 140);
                    formBg.setFillColor(sf::Color(30, 40, 60, 220));
                    formBg.setOutlineThickness(2);
                    formBg.setOutlineColor(sf::Color::Cyan);
                    window.draw(formBg);

                    if (prefBookingState == PREF_SELECTING_ROUTE_TYPE)
                    {
                        // Show route type selection for preference booking
                        sf::Text formTitle("SELECT ROUTE TYPE", font, 24);
                        formTitle.setFillColor(sf::Color(120, 220, 255));
                        formTitle.setStyle(sf::Text::Bold);
                        formTitle.setPosition(310, 180);
                        window.draw(formTitle);

                        sf::Text option1("1. CHEAPEST PATH - Minimize total cost", font, 16);
                        option1.setFillColor(sf::Color::White);
                        option1.setPosition(300, 250);
                        window.draw(option1);

                        sf::Text option2("2. SHORTEST PATH - Minimize number of routes", font, 16);
                        option2.setFillColor(sf::Color::White);
                        option2.setPosition(300, 290);
                        window.draw(option2);

                        sf::Text instrText("Enter your choice (1 or 2):", font, 16);
                        instrText.setFillColor(sf::Color::Cyan);
                        instrText.setPosition(300, 350);
                        window.draw(instrText);

                        routeTypeInput.render(window, font, 300, 390, "Choice:");
                    }
                    else
                    {
                        sf::Text formTitle("Enter Route Details", font, 24);
                        formTitle.setFillColor(sf::Color(120, 220, 255));
                        formTitle.setStyle(sf::Text::Bold);
                        formTitle.setPosition(300, 158);
                        window.draw(formTitle);

                        originInput.render(window, font, 300, 220, "Origin:");
                        destinationInput.render(window, font, 300, 280, "Destination:");
                        dateInput.render(window, font, 300, 340, "Date (DD/MM/YYYY):");

                        sf::Text instruction("Press ENTER to confirm each field", font, 16);
                        instruction.setFillColor(sf::Color(210, 220, 240));
                        instruction.setPosition(300, 410);
                        window.draw(instruction);

                        if (!prefErrorMessage.empty())
                        {
                            sf::Text error(prefErrorMessage, font, 18);
                            error.setFillColor(sf::Color::Red);
                            error.setPosition(300, 440);
                            window.draw(error);
                        }

                        // Show applied preferences in corner
                        sf::RectangleShape prefBox(sf::Vector2f(380, 220));
                        prefBox.setPosition(820, 560);
                        prefBox.setFillColor(sf::Color(10, 20, 35, 220));
                        prefBox.setOutlineThickness(2);
                        prefBox.setOutlineColor(sf::Color(120, 200, 140));
                        window.draw(prefBox);

                        sf::Text prefTitle("Applied Preferences", font, 16);
                        prefTitle.setFillColor(sf::Color(140, 230, 170));
                        prefTitle.setStyle(sf::Text::Bold);
                        prefTitle.setPosition(834, 572);
                        window.draw(prefTitle);

                        stringstream prefInfo;
                        if (currentPreferences.hasCompanyPreference && currentPreferences.preferredCompanies.getSize() > 0)
                        {
                            prefInfo << "Companies:\n";
                            for (int i = 0; i < currentPreferences.preferredCompanies.getSize(); i++)
                            {
                                prefInfo << "  • " << currentPreferences.preferredCompanies.get(i) << "\n";
                            }
                        }

                        sf::Text prefText(prefInfo.str(), font, 13);
                        prefText.setFillColor(sf::Color::White);
                        prefText.setPosition(834, 600);
                        window.draw(prefText);
                    }
                }
                else if (prefBookingState == PREF_VALIDATING)
                {
                    mapVisualizer.render(window);

                    sf::Text message("Validating route details...", font, 28);
                    message.setFillColor(sf::Color::Yellow);
                    message.setPosition(380, 400);
                    window.draw(message);
                }
                else if (prefBookingState == PREF_SHOWING_CONNECTING_ROUTES)
                {
                    bookingVisualizer.render(window);

                    sf::Text message("Showing connecting routes between origin and destination...", font, 24);
                    message.setFillColor(sf::Color::Yellow);
                    message.setPosition(250, 750);
                    window.draw(message);
                }
                else if (prefBookingState == PREF_VALIDATING_PREFERENCES)
                {
                    bookingVisualizer.render(window);

                    sf::Text message("Validating preferences...", font, 24);
                    message.setFillColor(sf::Color::Yellow);
                    message.setPosition(450, 400);
                    window.draw(message);
                }
                else if (prefBookingState == PREF_CALCULATING_OPTIMAL)
                {
                    bookingVisualizer.render(window);

                    sf::Text message("Calculating path with LEAST COST using Dijkstra (with preferences)...", font, 24);
                    message.setFillColor(sf::Color::Cyan);
                    message.setPosition(200, 750);
                    window.draw(message);
                }
                else if (prefBookingState == PREF_SHOWING_OPTIMAL_PATH || prefBookingState == PREF_BOOKING_COMPLETE)
                {
                    bookingVisualizer.render(window);

                    // Calculate dynamic height based on layover count
                    int prefBoxHeight = 280;
                    if (prefCurrentPath.found && prefCurrentPath.layovers.getSize() > 0)
                    {
                        prefBoxHeight = 280 + (prefCurrentPath.layovers.getSize() * 25);
                    }
                    sf::RectangleShape infoBox(sf::Vector2f(400, prefBoxHeight));
                    infoBox.setPosition(15, 510);
                    infoBox.setFillColor(sf::Color(0, 0, 0, 200));
                    infoBox.setOutlineThickness(3);
                    infoBox.setOutlineColor(prefCurrentPath.found ? sf::Color(255, 215, 0) : sf::Color(255, 0, 0));
                    window.draw(infoBox);

                    string titleText = prefCurrentPath.found ? "OPTIMAL ROUTE (With Preferences)" : "No Path Found";
                    sf::Text infoTitle(titleText, font, 20);
                    infoTitle.setFillColor(prefCurrentPath.found ? sf::Color(255, 215, 0) : sf::Color(255, 0, 0));
                    infoTitle.setStyle(sf::Text::Bold);
                    infoTitle.setPosition(25, 520);
                    window.draw(infoTitle);

                    stringstream pathInfo;
                    pathInfo << "Origin: " << prefOrigin << "\n"
                        << "Destination: " << prefDestination << "\n"
                        << "Date: " << prefDate << "\n";

                    if (prefCurrentPath.found)
                    {
                        pathInfo << "Total Cost: $" << prefCurrentPath.totalCost << "\n"
                            << "Travel Time: " << prefCurrentPath.totalTravelTime << " hours\n"
                            << "  (" << (prefCurrentPath.totalTravelTime / 24) << " days " 
                            << (prefCurrentPath.totalTravelTime % 24) << " hours)\n"
                            << "Stops: " << (prefCurrentPath.path.getSize() - 1) << "\n\n"
                            << "Optimal Route:\n";

                        for (int i = 0; i < prefCurrentPath.path.getSize(); i++)
                        {
                            pathInfo << prefCurrentPath.path.get(i);
                            if (i < prefCurrentPath.path.getSize() - 1)
                            {
                                pathInfo << " -> ";
                            }
                        }

                        // Display layover information
                        if (prefCurrentPath.layovers.getSize() > 0)
                        {
                            pathInfo << "\n\nDocking at Ports:";
                            for (int i = 0; i < prefCurrentPath.layovers.getSize(); i++)
                            {
                                const LayoverInfo& layover = prefCurrentPath.layovers.get(i);
                                pathInfo << "\n  " << layover.portName << ":\n"
                                    << "    Arrived: " << layover.arrivalDate << " " << layover.arrivalTime << "\n"
                                    << "    Departed: " << layover.departureDate << " " << layover.departureTime << "\n"
                                    << "    Docked: " << layover.layoverHours << " hours";
                                if (layover.layoverHours > 12)
                                {
                                    pathInfo << " (Charge: $" << layover.portCharge << ")";
                                }
                                else
                                {
                                    pathInfo << " (No charge)";
                                }
                            }
                        }

                        pathInfo << "\n\nPreferences Applied:";
                        if (currentPreferences.hasCompanyPreference)
                        {
                            pathInfo << "\nCompanies: ";
                            for (int i = 0; i < currentPreferences.preferredCompanies.getSize(); i++)
                            {
                                pathInfo << currentPreferences.preferredCompanies.get(i);
                                if (i < currentPreferences.preferredCompanies.getSize() - 1)
                                    pathInfo << ", ";
                            }
                        }
                    }
                    else
                    {
                        pathInfo << "Status: No path found with given preferences\n";
                        pathInfo << "Try adjusting your preferences.";
                    }

                    sf::Text pathText(pathInfo.str(), font, 13);
                    pathText.setFillColor(sf::Color::White);
                    pathText.setPosition(25, 555);
                    window.draw(pathText);
                }

                break;
                
                }
            
            case MULTI_LEG_ROUTE:
            {
                sf::Text title("Multi-Leg Route Generation", font, 30);
                title.setFillColor(sf::Color::Cyan);
                title.setPosition(350, 80);
                window.draw(title);

                if (multiLegState == MULTI_INPUT_ORIGIN ||
                    multiLegState == MULTI_INPUT_INTERMEDIATE ||
                    multiLegState == MULTI_INPUT_DESTINATION ||
                    multiLegState == MULTI_INPUT_DATE)
                {

                    sf::RectangleShape formBg(sf::Vector2f(600, 400));
                    formBg.setPosition(280, 150);
                    formBg.setFillColor(sf::Color(30, 40, 60, 200));
                    formBg.setOutlineThickness(2);
                    formBg.setOutlineColor(sf::Color::Cyan);
                    window.draw(formBg);

                    float yPos = 190;

                    // Origin input
                    if (multiLegState == MULTI_INPUT_ORIGIN)
                    {
                        multiPortInput.render(window, font, 320, yPos, "Origin Port:");
                    }
                    else
                    {
                        sf::Text originLabel("Origin: " + multiOrigin, font, 16);
                        originLabel.setFillColor(sf::Color(150, 255, 150));
                        originLabel.setPosition(320, yPos);
                        window.draw(originLabel);
                    }

                    yPos += 60;

                    // Intermediate ports
                    if (multiLegState == MULTI_INPUT_INTERMEDIATE)
                    {
                        string label = "Intermediate Port " + to_string(intermediatePorts.getSize() + 1) + ":";
                        multiPortInput.render(window, font, 320, yPos, label);

                        // Show already entered intermediate ports
                        if (intermediatePorts.getSize() > 0)
                        {
                            sf::Text hint("Entered: ", font, 14);
                            hint.setFillColor(sf::Color(200, 200, 200));
                            hint.setPosition(320, yPos + 35);
                            window.draw(hint);

                            stringstream portsList;
                            for (int i = 0; i < intermediatePorts.getSize(); i++)
                            {
                                portsList << intermediatePorts.get(i);
                                if (i < intermediatePorts.getSize() - 1)
                                    portsList << ", ";
                            }
                            sf::Text portsText(portsList.str(), font, 14);
                            portsText.setFillColor(sf::Color(150, 255, 150));
                            portsText.setPosition(400, yPos + 35);
                            window.draw(portsText);
                        }

                        sf::Text instruction("Press ENTER with empty field to finish intermediate ports", font, 14);
                        instruction.setFillColor(sf::Color(200, 200, 200));
                        instruction.setPosition(320, yPos + 55);
                        window.draw(instruction);
                    }
                    else if (intermediatePorts.getSize() > 0)
                    {
                        stringstream portsList;
                        portsList << "Intermediate Ports: ";
                        for (int i = 0; i < intermediatePorts.getSize(); i++)
                        {
                            portsList << intermediatePorts.get(i);
                            if (i < intermediatePorts.getSize() - 1)
                                portsList << ", ";
                        }
                        sf::Text portsText(portsList.str(), font, 16);
                        portsText.setFillColor(sf::Color(150, 255, 150));
                        portsText.setPosition(320, yPos);
                        window.draw(portsText);
                    }

                    yPos += 60;

                    // Destination input
                    if (multiLegState == MULTI_INPUT_DESTINATION)
                    {
                        multiPortInput.render(window, font, 320, yPos, "Destination Port:");
                    }
                    else if (!multiDestination.empty())
                    {
                        sf::Text destLabel("Destination: " + multiDestination, font, 16);
                        destLabel.setFillColor(sf::Color(150, 255, 150));
                        destLabel.setPosition(320, yPos);
                        window.draw(destLabel);
                    }

                    yPos += 60;

                    // Date input
                    if (multiLegState == MULTI_INPUT_DATE)
                    {
                        multiPortInput.render(window, font, 320, yPos, "Date (DD/MM/YYYY):");
                    }
                    else if (!multiDate.empty())
                    {
                        sf::Text dateLabel("Date: " + multiDate, font, 16);
                        dateLabel.setFillColor(sf::Color(150, 255, 150));
                        dateLabel.setPosition(320, yPos);
                        window.draw(dateLabel);
                    }

                    // Error message
                    if (!multiErrorMessage.empty())
                    {
                        sf::Text error(multiErrorMessage, font, 18);
                        error.setFillColor(sf::Color::Red);
                        error.setPosition(320, yPos + 50);
                        window.draw(error);
                    }

                    // Instruction
                    sf::Text instruction("Press ENTER to confirm each field", font, 16);
                    instruction.setFillColor(sf::Color(200, 200, 200));
                    instruction.setPosition(380, 520);
                    window.draw(instruction);
                }
                else if (multiLegState == MULTI_VALIDATING)
                {
                    sf::Text message("Validating ports...", font, 28);
                    message.setFillColor(sf::Color::Yellow);
                    message.setPosition(450, 400);
                    window.draw(message);
                }
                else if (multiLegState == MULTI_CALCULATING)
                {
                    sf::Text message("Calculating multi-leg route...", font, 28);
                    message.setFillColor(sf::Color::Cyan);
                    message.setPosition(400, 400);
                    window.draw(message);
                }
                else if (multiLegState == MULTI_SHOWING_ROUTE || multiLegState == MULTI_COMPLETE)
                {
                    // Show map with route
                    bookingVisualizer.render(window);

                    // Show route information
                    sf::RectangleShape infoBox(sf::Vector2f(450, 350));
                    infoBox.setPosition(15, 430);
                    infoBox.setFillColor(sf::Color(0, 0, 0, 200));
                    infoBox.setOutlineThickness(3);
                    infoBox.setOutlineColor(multiLegPath.found ? sf::Color(255, 215, 0) : sf::Color(255, 0, 0));
                    window.draw(infoBox);

                    string titleText = multiLegPath.found ? "MULTI-LEG ROUTE" : "No Route Found";
                    sf::Text infoTitle(titleText, font, 20);
                    infoTitle.setFillColor(multiLegPath.found ? sf::Color(255, 215, 0) : sf::Color(255, 0, 0));
                    infoTitle.setStyle(sf::Text::Bold);
                    infoTitle.setPosition(25, 440);
                    window.draw(infoTitle);

                    stringstream routeInfo;
                    routeInfo << "Origin: " << multiOrigin << "\n";

                    if (intermediatePorts.getSize() > 0)
                    {
                        routeInfo << "Intermediate Ports:\n";
                        for (int i = 0; i < intermediatePorts.getSize(); i++)
                        {
                            routeInfo << "  " << (i + 1) << ". " << intermediatePorts.get(i) << "\n";
                        }
                    }

                    routeInfo << "Destination: " << multiDestination << "\n"
                              << "Date: " << multiDate << "\n\n";

                    if (multiLegPath.found)
                    {
                        routeInfo << "Total Cost: $" << multiLegPath.totalCost << "\n"
                                  << "Total Legs: " << (intermediatePorts.getSize() + 1) << "\n"
                                  << "Total Stops: " << multiLegPath.path.getSize() << "\n\n"
                                  << "Complete Route:\n";

                        for (int i = 0; i < multiLegPath.path.getSize(); i++)
                        {
                            routeInfo << multiLegPath.path.get(i);
                            if (i < multiLegPath.path.getSize() - 1)
                            {
                                routeInfo << " -> ";
                            }
                        }

                        routeInfo << "\n\nRoute Details:\n";
                        int legNum = 1;
                        for (int i = 0; i < multiLegPath.routes.getSize(); i++)
                        {
                            const Route &route = multiLegPath.routes.get(i);
                            routeInfo << "Leg " << legNum << ": " << route.origin
                                      << " -> " << route.destination << "\n";
                            routeInfo << "  Cost: $" << route.cost << "\n";
                            routeInfo << "  Company: " << route.shippingCompany << "\n";
                            routeInfo << "  Time: " << route.departureTime << " - " << route.arrivalTime << "\n";

                            // Check if this route reaches an intermediate port or destination
                            bool isLegEnd = false;
                            if (route.destination == multiDestination)
                            {
                                isLegEnd = true;
                            }
                            else
                            {
                                for (int j = 0; j < intermediatePorts.getSize(); j++)
                                {
                                    if (route.destination == intermediatePorts.get(j))
                                    {
                                        isLegEnd = true;
                                        break;
                                    }
                                }
                            }

                            if (isLegEnd && i < multiLegPath.routes.getSize() - 1)
                            {
                                legNum++;
                                routeInfo << "\n";
                            }
                            else
                            {
                                routeInfo << "\n";
                            }
                        }
                    }
                    else
                    {
                        routeInfo << "Status: No complete route found\n";
                        routeInfo << "Unable to connect all ports in sequence.";
                    }

                    sf::Text routeText(routeInfo.str(), font, 12);
                    routeText.setFillColor(sf::Color::White);
                    routeText.setPosition(25, 475);
                    window.draw(routeText);
                }

                break;
            }

            case DOCKING_LAYOVER:
            {
                sf::Text title("Docking & Layover Management", font, 30);
                title.setFillColor(sf::Color::Cyan);
                title.setPosition(300, 20);
                window.draw(title);

                // Menu options
                sf::RectangleShape menuBox(sf::Vector2f(500, 200));
                menuBox.setPosition(50, 70);
                menuBox.setFillColor(sf::Color(30, 40, 60, 200));
                menuBox.setOutlineThickness(2);
                menuBox.setOutlineColor(sf::Color::Cyan);
                window.draw(menuBox);

                sf::Text menuTitle("Options (Press number key):", font, 18);
                menuTitle.setFillColor(sf::Color::White);
                menuTitle.setStyle(sf::Text::Bold);
                menuTitle.setPosition(60, 80);
                window.draw(menuTitle);

                string options[] = {
                    "1. View All Port Queues",
                    "2. Add Ship to Queue",
                    "3. Remove Ship from Queue",
                    "4. View Port Details"};

                sf::Color optionColors[] = {
                    dockingState == DOCKING_VIEW_QUEUES ? sf::Color(100, 255, 100) : sf::Color::White,
                    dockingState == DOCKING_ADD_SHIP ? sf::Color(100, 255, 100) : sf::Color::White,
                    dockingState == DOCKING_REMOVE_SHIP ? sf::Color(100, 255, 100) : sf::Color::White,
                    dockingState == DOCKING_VIEW_PORT ? sf::Color(100, 255, 100) : sf::Color::White};

                for (int i = 0; i < 4; i++)
                {
                    sf::Text option(options[i], font, 16);
                    option.setFillColor(optionColors[i]);
                    option.setPosition(70, 110 + i * 35);
                    window.draw(option);
                }

                // Main content area
                sf::RectangleShape contentBox(sf::Vector2f(1100, 480));
                contentBox.setPosition(50, 280);
                contentBox.setFillColor(sf::Color(25, 35, 55, 220));
                contentBox.setOutlineThickness(2);
                contentBox.setOutlineColor(sf::Color::Cyan);
                window.draw(contentBox);

                if (dockingState == DOCKING_VIEW_QUEUES)
                {
                    sf::Text sectionTitle("All Port Docking Queues", font, 22);
                    sectionTitle.setFillColor(sf::Color(100, 200, 255));
                    sectionTitle.setStyle(sf::Text::Bold);
                    sectionTitle.setPosition(60, 290);
                    window.draw(sectionTitle);

                    LinkedList<Port> allPorts = maritimeGraph.getAllPorts();
                    float yPos = 330;
                    int portsWithShips = 0;
                    int totalShips = 0;

                    // Display ports with ships in queue
                    for (int i = 0; i < allPorts.getSize(); i++)
                    {
                        const Port &port = allPorts.get(i);
                        int queueSize = maritimeGraph.getQueueSize(port.name);

                        if (queueSize > 0)
                        {
                            portsWithShips++;
                            totalShips += queueSize;

                            // Port name and queue size
                            stringstream portInfo;
                            portInfo << port.name << " - " << queueSize << " ship(s) waiting";
                            sf::Text portText(portInfo.str(), font, 14);
                            portText.setFillColor(sf::Color(255, 200, 100));
                            portText.setPosition(70, yPos);
                            window.draw(portText);

                            // Show ships in queue
                            LinkedList<string> ships = maritimeGraph.getQueueShips(port.name);
                            stringstream shipsList;
                            shipsList << "  Ships: ";
                            for (int j = 0; j < ships.getSize(); j++)
                            {
                                shipsList << ships.get(j);
                                if (j < ships.getSize() - 1)
                                    shipsList << ", ";
                            }
                            sf::Text shipsText(shipsList.str(), font, 12);
                            shipsText.setFillColor(sf::Color(200, 200, 200));
                            shipsText.setPosition(90, yPos + 20);
                            window.draw(shipsText);

                            yPos += 50;
                        }
                    }

                    if (portsWithShips == 0)
                    {
                        sf::Text noShips("No ships currently waiting at any port.", font, 18);
                        noShips.setFillColor(sf::Color(150, 150, 150));
                        noShips.setPosition(70, 330);
                        window.draw(noShips);
                    }

                    // Summary
                    stringstream summary;
                    summary << "Summary: " << portsWithShips << " port(s) with ships, "
                            << totalShips << " total ship(s) waiting";
                    sf::Text summaryText(summary.str(), font, 14);
                    summaryText.setFillColor(sf::Color(150, 255, 150));
                    summaryText.setPosition(60, 720);
                    window.draw(summaryText);
                }
                else if (dockingState == DOCKING_ADD_SHIP)
                {
                    sf::Text sectionTitle("Add Ship to Docking Queue", font, 22);
                    sectionTitle.setFillColor(sf::Color(100, 200, 255));
                    sectionTitle.setStyle(sf::Text::Bold);
                    sectionTitle.setPosition(60, 290);
                    window.draw(sectionTitle);

                    if (dockingPortInput.getIsActive())
                    {
                        dockingPortInput.render(window, font, 70, 340, "Port Name:");
                        sf::Text instruction("Enter port name, then press ENTER", font, 14);
                        instruction.setFillColor(sf::Color(200, 200, 200));
                        instruction.setPosition(70, 400);
                        window.draw(instruction);
                    }
                    else if (dockingShipInput.getIsActive())
                    {
                        sf::Text portLabel("Port: " + selectedDockingPort, font, 16);
                        portLabel.setFillColor(sf::Color(150, 255, 150));
                        portLabel.setPosition(70, 340);
                        window.draw(portLabel);

                        dockingShipInput.render(window, font, 70, 380, "Ship Name:");
                        sf::Text instruction("Enter ship name, then press ENTER", font, 14);
                        instruction.setFillColor(sf::Color(200, 200, 200));
                        instruction.setPosition(70, 440);
                        window.draw(instruction);
                    }
                    else
                    {
                        sf::Text success("Ship added successfully! Press 1 to view queues.", font, 16);
                        success.setFillColor(sf::Color(100, 255, 100));
                        success.setPosition(70, 340);
                        window.draw(success);
                    }

                    if (!dockingErrorMessage.empty())
                    {
                        sf::Text error(dockingErrorMessage, font, 16);
                        error.setFillColor(sf::Color::Red);
                        error.setPosition(70, 500);
                        window.draw(error);
                    }
                }
                else if (dockingState == DOCKING_REMOVE_SHIP)
                {
                    sf::Text sectionTitle("Remove Ship from Docking Queue", font, 22);
                    sectionTitle.setFillColor(sf::Color(100, 200, 255));
                    sectionTitle.setStyle(sf::Text::Bold);
                    sectionTitle.setPosition(60, 290);
                    window.draw(sectionTitle);

                    if (dockingPortInput.getIsActive())
                    {
                        dockingPortInput.render(window, font, 70, 340, "Port Name:");
                        sf::Text instruction("Enter port name, then press ENTER to remove first ship", font, 14);
                        instruction.setFillColor(sf::Color(200, 200, 200));
                        instruction.setPosition(70, 400);
                        window.draw(instruction);
                    }
                    else
                    {
                        sf::Text success("Ship removed successfully! Press 1 to view queues.", font, 16);
                        success.setFillColor(sf::Color(100, 255, 100));
                        success.setPosition(70, 340);
                        window.draw(success);
                    }

                    if (!dockingErrorMessage.empty())
                    {
                        sf::Text error(dockingErrorMessage, font, 16);
                        error.setFillColor(sf::Color::Red);
                        error.setPosition(70, 500);
                        window.draw(error);
                    }
                }
                else if (dockingState == DOCKING_VIEW_PORT)
                {
                    sf::Text sectionTitle("Port Details", font, 22);
                    sectionTitle.setFillColor(sf::Color(100, 200, 255));
                    sectionTitle.setStyle(sf::Text::Bold);
                    sectionTitle.setPosition(60, 290);
                    window.draw(sectionTitle);

                    if (dockingPortInput.getIsActive())
                    {
                        dockingPortInput.render(window, font, 70, 340, "Port Name:");
                        sf::Text instruction("Enter port name, then press ENTER", font, 14);
                        instruction.setFillColor(sf::Color(200, 200, 200));
                        instruction.setPosition(70, 400);
                        window.draw(instruction);
                    }
                    else if (!selectedDockingPort.empty())
                    {
                        Port port;
                        if (maritimeGraph.getPort(selectedDockingPort, port))
                        {
                            // Port information
                            stringstream portInfo;
                            portInfo << "Port: " << port.name << "\n"
                                     << "Location: (" << port.x << ", " << port.y << ")\n"
                                     << "Daily Charge: $" << port.dailyCharge << "\n\n";

                            int queueSize = maritimeGraph.getQueueSize(port.name);
                            portInfo << "Docking Queue:\n"
                                     << "  Ships Waiting: " << queueSize << "\n";

                            if (queueSize > 0)
                            {
                                LinkedList<string> ships = maritimeGraph.getQueueShips(port.name);
                                portInfo << "  Ships in Queue:\n";
                                for (int i = 0; i < ships.getSize(); i++)
                                {
                                    portInfo << "    " << (i + 1) << ". " << ships.get(i) << "\n";
                                }
                            }
                            else
                            {
                                portInfo << "  No ships waiting\n";
                            }

                            // Get routes from this port
                            LinkedList<Route> routes = maritimeGraph.getRoutesFrom(port.name);
                            portInfo << "\nRoutes from this port: " << routes.getSize();

                            sf::Text portText(portInfo.str(), font, 14);
                            portText.setFillColor(sf::Color::White);
                            portText.setPosition(70, 340);
                            window.draw(portText);
                        }
                    }

                    if (!dockingErrorMessage.empty())
                    {
                        sf::Text error(dockingErrorMessage, font, 16);
                        error.setFillColor(sf::Color::Red);
                        error.setPosition(70, 650);
                        window.draw(error);
                    }
                }

                break;
            }

            default:
                break;
            }
            }

                window.display();
            }

            return 0;
        }
     
     
