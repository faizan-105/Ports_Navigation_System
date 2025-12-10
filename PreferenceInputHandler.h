
#pragma once
#ifndef PREFERENCEINPUTHANDLER_H
#define PREFERENCEINPUTHANDLER_H

#include <SFML/Graphics.hpp>
#include "InputHandler.h"
#include "PreferenceFilter.h"
#include "LinkedList.h"
#include <string>
#include <sstream>
#include <cmath>
using namespace std;
class PreferenceInputHandler
{
private:
    InputHandler companyInput;       // For entering shipping companies (comma-separated)
    InputHandler portsInput;         // For entering required ports (comma-separated)
    InputHandler excludedPortsInput; // For entering excluded ports (comma-separated)
    InputHandler maxTimeInput;       // For entering maximum voyage time in hours

    bool companyInputActive;
    bool portsInputActive;
    bool excludedPortsInputActive;
    bool maxTimeInputActive;

    PreferenceFilter currentPreferences;

    // Available shipping companies from routes.txt
    LinkedList<string> availableCompanies;

    // Cursor blink clock for visual feedback
    sf::Clock cursorBlinkClock;

public:
    PreferenceInputHandler()
        : companyInputActive(false), portsInputActive(false),
          excludedPortsInputActive(false), maxTimeInputActive(false)
    {
        // Initialize available companies
        availableCompanies.push_back("Evergreen");
        availableCompanies.push_back("MSC");
        availableCompanies.push_back("ZIM");
        availableCompanies.push_back("COSCO");
        availableCompanies.push_back("MaerskLine");
        availableCompanies.push_back("HapagLloyd");
        availableCompanies.push_back("YangMing");
        availableCompanies.push_back("CMA_CGM");
        availableCompanies.push_back("PIL");
        availableCompanies.push_back("ONE");
    }

    void startInput()
    {
        companyInput.clear();
        portsInput.clear();
        excludedPortsInput.clear();
        maxTimeInput.clear();
        companyInput.activate();
        companyInputActive = true;
        portsInputActive = false;
        excludedPortsInputActive = false;
        maxTimeInputActive = false;
        currentPreferences = PreferenceFilter();
    }

    void handleEvent(const sf::Event &event)
    {
        if (companyInputActive)
        {
            companyInput.handleEvent(event);
        }
        else if (portsInputActive)
        {
            portsInput.handleEvent(event);
        }
        else if (excludedPortsInputActive)
        {
            excludedPortsInput.handleEvent(event);
        }
        else if (maxTimeInputActive)
        {
            maxTimeInput.handleEvent(event);
        }
    }

    // Process Enter key to move to next input
    bool processEnter()
    {
        if (companyInputActive)
        {
            // Company input is optional, can be skipped
            string companies = companyInput.getText();
            if (!companies.empty())
            {
                parseCompanies(companies);
            }
            companyInput.deactivate();
            companyInputActive = false;
            portsInput.activate();
            portsInputActive = true;
            return false; // Not done yet
        }
        else if (portsInputActive)
        {
            // Ports input is optional, can be skipped
            string ports = portsInput.getText();
            if (!ports.empty())
            {
                parseRequiredPorts(ports);
            }
            portsInput.deactivate();
            portsInputActive = false;
            excludedPortsInput.activate();
            excludedPortsInputActive = true;
            return false;
        }
        else if (excludedPortsInputActive)
        {
            // Excluded ports input is optional
            string ports = excludedPortsInput.getText();
            if (!ports.empty())
            {
                parseExcludedPorts(ports);
            }
            excludedPortsInput.deactivate();
            excludedPortsInputActive = false;
            maxTimeInput.activate();
            maxTimeInputActive = true;
            return false;
        }
        else if (maxTimeInputActive)
        {
            // Max time input is optional
            string timeStr = maxTimeInput.getText();
            if (!timeStr.empty())
            {
                parseMaxTime(timeStr);
            }
            maxTimeInput.deactivate();
            maxTimeInputActive = false;
            return true; // Done with all inputs
        }
        return false;
    }

    bool isInputActive() const
    {
        return companyInputActive || portsInputActive || excludedPortsInputActive || maxTimeInputActive;
    }

    PreferenceFilter getPreferences() const
    {
        return currentPreferences;
    }

    void render(sf::RenderWindow &window, sf::Font &font)
    {
        // Compact corner panel - positioned at top-right
        float panelX = 800.0f;
        float panelY = 20.0f;
        float panelWidth = 380.0f;
        float panelHeight = 840.0f;

        float companiesY = panelY + 60.0f;
        float startY = panelY + 210.0f;
        float spacing = 110.0f;
        float labelX = panelX + 18.0f;
        float inputBoxX = panelX + 18.0f;
        float inputBoxWidth = 344.0f;

        // Draw main background panel (semi-transparent so map shows through)
        sf::RectangleShape mainPanel(sf::Vector2f(panelWidth, panelHeight));
        mainPanel.setPosition(panelX, panelY);
        mainPanel.setFillColor(sf::Color(25, 35, 55, 220));
        mainPanel.setOutlineThickness(3);
        mainPanel.setOutlineColor(sf::Color(100, 200, 255));
        window.draw(mainPanel);

        // Draw section header
        sf::Text sectionTitle("Route Preferences", font, 20);
        sectionTitle.setFillColor(sf::Color(100, 200, 255));
        sectionTitle.setStyle(sf::Text::Bold);
        sectionTitle.setPosition(panelX + 15, panelY + 15);
        window.draw(sectionTitle);

        // Draw decorative line under title
        sf::RectangleShape titleLine(sf::Vector2f(panelWidth - 30, 2));
        titleLine.setPosition(panelX + 15, panelY + 40);
        titleLine.setFillColor(sf::Color(100, 200, 255, 150));
        window.draw(titleLine);

        // Draw available companies hint (only for company input) - smaller and positioned differently
        if (companyInputActive)
        {
            renderAvailableCompanies(window, font, panelX + 15, companiesY);
        }

        // Company input with compact styling
        renderInputField(window, font, labelX, startY, inputBoxX, inputBoxWidth,
                         "Companies",
                         "MSC, MaerskLine, COSCO...",
                         companyInput, companyInputActive, 1);

        // Required ports input
        renderInputField(window, font, labelX, startY + spacing, inputBoxX, inputBoxWidth,
                         "Required Ports (Optional)",
                         "Must include",
                         portsInput, portsInputActive, 2);

        // Excluded ports input
        renderInputField(window, font, labelX, startY + spacing * 2, inputBoxX, inputBoxWidth,
                         "Excluded Ports (Optional)",
                         "Avoid these",
                         excludedPortsInput, excludedPortsInputActive, 3);

        // Max voyage time input
        renderInputField(window, font, labelX, startY + spacing * 3, inputBoxX, inputBoxWidth,
                         "Max Time (Optional)",
                         "Hours (e.g., 48)",
                         maxTimeInput, maxTimeInputActive, 4);

        // Show current preferences summary (compact)
        if (!isInputActive())
        {
            renderPreferencesSummary(window, font, labelX, startY + spacing * 4 + 20);
        }
    }

private:
    void renderInputField(sf::RenderWindow &window, sf::Font &font,
                          float labelX, float y, float inputBoxX, float inputBoxWidth,
                          const string &title, const string &hint,
                          InputHandler &inputHandler, bool isActive, int fieldNumber)
    {

        float fieldBoxHeight = 82.0f;

        // Draw field background box with more breathing room
        sf::RectangleShape fieldBox(sf::Vector2f(inputBoxWidth, fieldBoxHeight));
        fieldBox.setPosition(inputBoxX, y - 10);
        if (isActive)
        {
            fieldBox.setFillColor(sf::Color(40, 60, 90, 200));
            fieldBox.setOutlineThickness(2);
            fieldBox.setOutlineColor(sf::Color(100, 200, 255));
        }
        else
        {
            fieldBox.setFillColor(sf::Color(30, 45, 70, 120));
            fieldBox.setOutlineThickness(1);
            fieldBox.setOutlineColor(sf::Color(80, 120, 160, 100));
        }
        window.draw(fieldBox);

        // Draw field number indicator (smaller)
        sf::CircleShape numberCircle(12);
        numberCircle.setPosition(inputBoxX + 5, y + 6);
        if (isActive)
        {
            numberCircle.setFillColor(sf::Color(100, 200, 255));
        }
        else
        {
            numberCircle.setFillColor(sf::Color(80, 120, 160));
        }
        numberCircle.setOutlineThickness(1);
        numberCircle.setOutlineColor(sf::Color::White);
        window.draw(numberCircle);

        sf::Text numberText(to_string(fieldNumber), font, 12);
        numberText.setFillColor(sf::Color::White);
        numberText.setStyle(sf::Text::Bold);
        sf::FloatRect numberBounds = numberText.getLocalBounds();
        numberText.setPosition(inputBoxX + 5 + 12 - numberBounds.width / 2, y + 4);
        window.draw(numberText);

        // Draw field title (compact)
        sf::Text fieldTitle(title, font, 16);
        fieldTitle.setFillColor(isActive ? sf::Color(150, 220, 255) : sf::Color(180, 200, 220));
        fieldTitle.setStyle(sf::Text::Bold);
        fieldTitle.setPosition(inputBoxX + 30, y);
        window.draw(fieldTitle);

        // Draw hint text (smaller)
        sf::Text hintText(hint, font, 12);
        hintText.setFillColor(sf::Color(150, 150, 150));
        hintText.setPosition(inputBoxX + 30, y + 24);
        window.draw(hintText);

        // Draw input box (compact)
        sf::RectangleShape inputBox(sf::Vector2f(inputBoxWidth - 36, 36));
        inputBox.setPosition(inputBoxX + 18, y + 40);
        if (isActive)
        {
            inputBox.setFillColor(sf::Color(20, 30, 45));
            inputBox.setOutlineThickness(2);
            inputBox.setOutlineColor(sf::Color(100, 200, 255));
        }
        else
        {
            inputBox.setFillColor(sf::Color(40, 50, 65));
            inputBox.setOutlineThickness(1);
            inputBox.setOutlineColor(sf::Color(100, 100, 120));
        }
        window.draw(inputBox);

        // Draw input text (larger for readability)
        sf::Text inputText(inputHandler.getText(), font, 15);
        inputText.setFillColor(sf::Color::White);
        inputText.setPosition(inputBoxX + 26, y + 46);
        window.draw(inputText);

        // Draw blinking cursor for active field
        if (isActive)
        {
            float elapsed = cursorBlinkClock.getElapsedTime().asSeconds();
            float cycleTime = fmod(elapsed, 1.2f);
            if (cycleTime < 0.6f)
            {
                sf::RectangleShape cursor(sf::Vector2f(2, 20));
                cursor.setPosition(inputBoxX + 26 + inputText.getGlobalBounds().width + 2, y + 44);
                cursor.setFillColor(sf::Color(100, 200, 255));
                window.draw(cursor);
            }
        }

        // Draw active indicator arrow (smaller)
        if (isActive)
        {
            sf::ConvexShape arrow(3);
            arrow.setPoint(0, sf::Vector2f(0, 0));
            arrow.setPoint(1, sf::Vector2f(6, -4));
            arrow.setPoint(2, sf::Vector2f(6, 4));
            arrow.setFillColor(sf::Color(100, 200, 255));
            arrow.setPosition(inputBoxX + 22, y + 18);
            window.draw(arrow);
        }
    }

private:
    void parseCompanies(const string &input)
    {
        currentPreferences.preferredCompanies.clear();
        currentPreferences.hasCompanyPreference = false;

        istringstream iss(input);
        string company;
        while (getline(iss, company, ','))
        {
            // Trim whitespace
            company.erase(0, company.find_first_not_of(" \t"));
            company.erase(company.find_last_not_of(" \t") + 1);

            if (!company.empty())
            {
                // Check if company is valid
                bool isValid = false;
                for (int i = 0; i < availableCompanies.getSize(); i++)
                {
                    if (availableCompanies.get(i) == company)
                    {
                        isValid = true;
                        break;
                    }
                }

                if (isValid)
                {
                    currentPreferences.preferredCompanies.push_back(company);
                    currentPreferences.hasCompanyPreference = true;
                }
            }
        }
    }

    void parseRequiredPorts(const string &input)
    {
        currentPreferences.requiredPorts.clear();
        currentPreferences.hasPortPreference = false;

        istringstream iss(input);
        string port;
        while (getline(iss, port, ','))
        {
            // Trim whitespace
            port.erase(0, port.find_first_not_of(" \t"));
            port.erase(port.find_last_not_of(" \t") + 1);

            if (!port.empty())
            {
                currentPreferences.requiredPorts.push_back(port);
                currentPreferences.hasPortPreference = true;
            }
        }
    }

    void parseExcludedPorts(const string &input)
    {
        currentPreferences.excludedPorts.clear();
        if (!currentPreferences.hasPortPreference)
        {
            currentPreferences.hasPortPreference = true;
        }

        istringstream iss(input);
        string port;
        while (getline(iss, port, ','))
        {
            // Trim whitespace
            port.erase(0, port.find_first_not_of(" \t"));
            port.erase(port.find_last_not_of(" \t") + 1);

            if (!port.empty())
            {
                currentPreferences.excludedPorts.push_back(port);
            }
        }
    }

    void parseMaxTime(const string &input)
    {
        try
        {
            int time = stoi(input);
            if (time > 0)
            {
                currentPreferences.maxVoyageTime = time;
                currentPreferences.hasTimeLimit = true;
            }
        }
        catch (...)
        {
            // Invalid input, ignore
        }
    }

    void renderPreferencesSummary(sf::RenderWindow &window, sf::Font &font, float x, float y)
    {
        // Compact summary box
        float summaryWidth = 344.0f;
        float summaryHeight = 170.0f;
        sf::RectangleShape summaryBox(sf::Vector2f(summaryWidth, summaryHeight));
        summaryBox.setPosition(x, y);
        summaryBox.setFillColor(sf::Color(25, 45, 75, 220));
        summaryBox.setOutlineThickness(2);
        summaryBox.setOutlineColor(sf::Color(100, 200, 255));
        window.draw(summaryBox);

        // Header section
        sf::RectangleShape headerBar(sf::Vector2f(summaryWidth, 25));
        headerBar.setPosition(x, y);
        headerBar.setFillColor(sf::Color(50, 100, 150, 180));
        window.draw(headerBar);

        // Title
        sf::Text title("✓ Summary", font, 14);
        title.setFillColor(sf::Color(150, 220, 255));
        title.setStyle(sf::Text::Bold);
        title.setPosition(x + 5, y + 3);
        window.draw(title);

        float currentY = y + 30;
        float lineSpacing = 22.0f;

        // Companies section
        renderSummaryItem(window, font, x + 5, currentY, "Companies:",
                          currentPreferences.hasCompanyPreference && currentPreferences.preferredCompanies.getSize() > 0,
                          currentPreferences.preferredCompanies, "Any", sf::Color(100, 200, 255));

        // Required Ports section
        currentY += lineSpacing;
        renderSummaryItem(window, font, x + 5, currentY, "Required:",
                          currentPreferences.hasPortPreference && currentPreferences.requiredPorts.getSize() > 0,
                          currentPreferences.requiredPorts, "None", sf::Color(150, 255, 150));

        // Excluded Ports section
        currentY += lineSpacing;
        renderSummaryItem(window, font, x + 5, currentY, "Excluded:",
                          currentPreferences.excludedPorts.getSize() > 0,
                          currentPreferences.excludedPorts, "None", sf::Color(255, 150, 150));

        // Max Voyage Time section
        currentY += lineSpacing;
        sf::Text timeLabel("Max Time:", font, 12);
        timeLabel.setFillColor(sf::Color(200, 200, 200));
        timeLabel.setPosition(x + 5, currentY);
        window.draw(timeLabel);

        sf::Text timeValue("", font, 12);
        timeValue.setStyle(sf::Text::Bold);
        if (currentPreferences.hasTimeLimit)
        {
            timeValue.setString(to_string(currentPreferences.maxVoyageTime) + "h");
            timeValue.setFillColor(sf::Color(255, 200, 100));
        }
        else
        {
            timeValue.setString("No limit");
            timeValue.setFillColor(sf::Color(150, 150, 150));
        }
        timeValue.setPosition(x + 80, currentY);
        window.draw(timeValue);
    }

    void renderSummaryItem(sf::RenderWindow &window, sf::Font &font, float x, float y,
                           const string &label, bool hasValue, const LinkedList<string> &values,
                           const string &defaultValue, sf::Color valueColor)
    {

        // Label
        sf::Text labelText(label, font, 12);
        labelText.setFillColor(sf::Color(200, 200, 200));
        labelText.setPosition(x, y);
        window.draw(labelText);

        // Value (compact, may wrap)
        sf::Text valueText("", font, 11);
        valueText.setStyle(sf::Text::Bold);
        valueText.setFillColor(valueColor);

        if (hasValue && values.getSize() > 0)
        {
            stringstream valueStr;
            int maxItems = 3; // Limit display to 3 items for compactness
            for (int i = 0; i < values.getSize() && i < maxItems; i++)
            {
                valueStr << values.get(i);
                if (i < values.getSize() - 1 && i < maxItems - 1)
                    valueStr << ", ";
            }
            if (values.getSize() > maxItems)
            {
                valueStr << "...";
            }
            valueText.setString(valueStr.str());
        }
        else
        {
            valueText.setString(defaultValue);
            valueText.setFillColor(sf::Color(150, 150, 150));
        }

        valueText.setPosition(x + 80, y);
        window.draw(valueText);
    }

    void renderAvailableCompanies(sf::RenderWindow &window, sf::Font &font, float x, float y)
    {
        // Compact background box for available companies
        sf::RectangleShape companiesBox(sf::Vector2f(344, 120));
        companiesBox.setPosition(x, y);
        companiesBox.setFillColor(sf::Color(20, 40, 60, 240));
        companiesBox.setOutlineThickness(2);
        companiesBox.setOutlineColor(sf::Color(100, 200, 255));
        window.draw(companiesBox);

        // Title
        sf::Text title("Available Companies:", font, 12);
        title.setFillColor(sf::Color(150, 220, 255));
        title.setStyle(sf::Text::Bold);
        title.setPosition(x + 10, y + 5);
        window.draw(title);

        // List companies in two columns (compact)
        float currentY = y + 26;
        float columnWidth = 165.0f;
        int halfSize = (availableCompanies.getSize() + 1) / 2;

        for (int i = 0; i < availableCompanies.getSize(); i++)
        {
            float colX = x + 10 + (i < halfSize ? 0 : columnWidth);
            float colY = currentY + (i < halfSize ? (i * 16) : ((i - halfSize) * 16));

            // Company name (smaller)
            sf::Text companyText(availableCompanies.get(i), font, 10);
            companyText.setFillColor(sf::Color(200, 220, 255));
            companyText.setPosition(colX, colY);
            window.draw(companyText);

            // Bullet point (smaller)
            sf::CircleShape bullet(2);
            bullet.setFillColor(sf::Color(100, 200, 255));
            bullet.setPosition(colX - 6, colY + 3);
            window.draw(bullet);
        }
    }
};

#endif
