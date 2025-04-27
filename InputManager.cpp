// InputManager.cpp
#include "InputManager.h"
#include <iostream>

InputManager::InputManager(bool multiplayer, bool host)
    : lKeyPressed(false),
    isMultiplayer(multiplayer),
    isHost(host)
{
}

void InputManager::processInput(VehicleManager* vehicleManager, float deltaTime)
{
    if (!vehicleManager) return;

    // Handle thrust level setting
    float thrustLevel = vehicleManager->getRocket()->getThrustLevel();

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num1))
        vehicleManager->getRocket()->setThrustLevel(0.1f);
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num2))
        vehicleManager->getRocket()->setThrustLevel(0.2f);
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num3))
        vehicleManager->getRocket()->setThrustLevel(0.3f);
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num4))
        vehicleManager->getRocket()->setThrustLevel(0.4f);
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num5))
        vehicleManager->getRocket()->setThrustLevel(0.5f);
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num6))
        vehicleManager->getRocket()->setThrustLevel(0.6f);
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num7))
        vehicleManager->getRocket()->setThrustLevel(0.7f);
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num8))
        vehicleManager->getRocket()->setThrustLevel(0.8f);
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num9))
        vehicleManager->getRocket()->setThrustLevel(0.9f);
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num0))
        vehicleManager->getRocket()->setThrustLevel(0.0f);
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Equal))
        vehicleManager->getRocket()->setThrustLevel(1.0f);

    // Apply thrust and rotation
    if (isMultiplayer && !isHost) {
        // Client controls use WASD
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W))
            vehicleManager->applyThrust(thrustLevel);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S))
            vehicleManager->applyThrust(-0.5f);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
            vehicleManager->rotate(-4.2f * deltaTime * 60.0f);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
            vehicleManager->rotate(4.2f * deltaTime * 60.0f);
    }
    else {
        // Host or single player uses arrow keys
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
            vehicleManager->applyThrust(thrustLevel);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
            vehicleManager->applyThrust(-0.5f);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
            vehicleManager->rotate(-4.2f * deltaTime * 60.0f);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
            vehicleManager->rotate(4.2f * deltaTime * 60.0f);
    }

    // Vehicle transformation
    //if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::L) && !lKeyPressed) {
    //    lKeyPressed = true;
    //   vehicleManager->switchVehicle();
    //}
}

void InputManager::handleKeyPressed(sf::Keyboard::Key key, VehicleManager* vehicleManager)
{
    //if (key == sf::Keyboard::Key::L) {
    //    lKeyPressed = true;
    //    if (vehicleManager) {
    //        vehicleManager->switchVehicle();
    //    }
    //}
}

void InputManager::handleKeyReleased(sf::Keyboard::Key key)
{
    //if (key == sf::Keyboard::Key::L) {
    //    lKeyPressed = false;
    //}
}