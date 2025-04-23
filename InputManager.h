// InputManager.h
#pragma once
#include <SFML/Window.hpp>
#include "VehicleManager.h"

class InputManager {
private:
    bool lKeyPressed;
    bool isMultiplayer;
    bool isHost;

public:
    InputManager(bool multiplayer, bool host);

    void processInput(VehicleManager* vehicleManager, float deltaTime);
    void handleKeyPressed(sf::Keyboard::Key key, VehicleManager* vehicleManager);
    void handleKeyReleased(sf::Keyboard::Key key);
};