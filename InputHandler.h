
#pragma once
#ifndef INPUTHANDLER_H
#define INPUTHANDLER_H
#include <SFML/Graphics.hpp>
#include <string>
using namespace std;
class InputHandler
{
private:
    string currentInput;
    bool isActive;
    sf::Clock cursorBlink;

public:
    InputHandler() : currentInput(""), isActive(false) {}
    void activate()
    {
        isActive = true;
        currentInput = "";
    }
    void deactivate()
    {
        isActive = false;
    }
    bool getIsActive() const
    {
        return isActive;
    }
    string getText() const
    {
        return currentInput;
    }
    void clear()
    {
        currentInput = "";
    }
    void handleEvent(const sf::Event &event)
    {
        if (!isActive)
            return;
        if (event.type == sf::Event::TextEntered)
        {
            if (event.text.unicode == 8)
            { // Backspace
                if (!currentInput.empty())
                {
                    currentInput.pop_back();
                }
            }
            else if (event.text.unicode == 13)
            { // Enter
                // Handle in main logic
            }
            else if (event.text.unicode < 128 && event.text.unicode >= 32)
            {
                currentInput += static_cast<char>(event.text.unicode);
            }
        }
    }
    void render(sf::RenderWindow &window, sf::Font &font,
                float x, float y, const string &label)
    {
        // Draw label
        sf::Text labelText(label, font, 18);
        // changes x->300
        labelText.setPosition(300, y);
        labelText.setFillColor(sf::Color::White);
        window.draw(labelText);

        // Calculate input box position - use larger offset for long labels
        float inputBoxX = x + 450; // Increased from 150 to 450 to avoid overlap with long labels
        if (inputBoxX + 300 > 1200)
        { // If it would go off screen, use smaller offset
            inputBoxX = x + 150;
        }

        // Draw input box
        sf::RectangleShape inputBox(sf::Vector2f(300, 35));
        // inputBox-> 350
        inputBox.setPosition(470, y - 5);
        inputBox.setFillColor(sf::Color(50, 50, 50));
        inputBox.setOutlineThickness(2);
        inputBox.setOutlineColor(isActive ? sf::Color::Cyan : sf::Color(100, 100, 100));
        window.draw(inputBox);
        // Draw current text
        sf::Text inputText(currentInput, font, 18);
        // inputBoxX->350
        inputText.setPosition(470 + 10, y);
        inputText.setFillColor(sf::Color::White);
        window.draw(inputText);
        // Draw blinking cursor
        if (isActive && cursorBlink.getElapsedTime().asSeconds() < 0.5f)
        {
            sf::RectangleShape cursor(sf::Vector2f(2, 20));
            // inputBoxX + 10 + inputText.getGlobalBounds().width + 2-> 470
            cursor.setPosition(470 + 10 + inputText.getGlobalBounds().width + 2, y + 5);
            cursor.setFillColor(sf::Color::White);
            window.draw(cursor);
        }
        if (cursorBlink.getElapsedTime().asSeconds() > 1.0f)
        {
            cursorBlink.restart();
        }
    }
};
#endif