// MenuSystem.h
#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include "Button.h"

// Changed from GameState to MenuGameState to avoid conflict
enum class MenuGameState {
    MENU,
    JOIN_MENU,
    SINGLE_PLAYER,
    MULTIPLAYER_HOST,
    MULTIPLAYER_CLIENT
};

class MenuSystem {
private:
    sf::RenderWindow& window;
    MenuGameState currentState;
    sf::Font& font;

    // Menu buttons
    std::vector<Button> menuButtons;
    std::vector<Button> joinMenuButtons;

    // Text elements
    sf::Text titleText;

    // Input fields for join screen
    std::string inputAddress;
    std::string inputPort;
    bool focusAddress;

    // Method to launch a separate host process
    void launchHostProcess();

public:
    MenuSystem(sf::RenderWindow& window, sf::Font& font);

    MenuGameState run();
    void handleEvents();
    void update(float deltaTime);
    void render();

    // Getters for multiplayer settings
    std::string getServerAddress() const { return inputAddress; }
    unsigned short getServerPort() const { return static_cast<unsigned short>(std::stoi(inputPort)); }
};