// UIManager.cpp
#include "UIManager.h"
#include "OrbitalMechanics.h"
#include "GameConstants.h"
#include <sstream>
#include <iomanip>
#include <limits>
#include <cmath>

UIManager::UIManager(sf::RenderWindow& window, sf::Font& font, sf::View& uiView, bool multiplayer, bool host)
    : window(window),
    font(font),
    uiView(uiView),
    isMultiplayer(multiplayer),
    isHost(host),
    rocketInfoPanel(font, 12, sf::Vector2f(10, 10), sf::Vector2f(250, 150)),
    planetInfoPanel(font, 12, sf::Vector2f(10, 170), sf::Vector2f(250, 120)),
    orbitInfoPanel(font, 12, sf::Vector2f(10, 300), sf::Vector2f(250, 100)),
    controlsPanel(font, 12, sf::Vector2f(10, 410), sf::Vector2f(250, 120)),
    thrustMetricsPanel(font, 12, sf::Vector2f(10, 530), sf::Vector2f(250, 80)),
    multiplayerPanel(font, 12, sf::Vector2f(10, 620), sf::Vector2f(250, 90))
{
    // Set up controls info panel
    std::string controlsText;
    if (isMultiplayer && !isHost) {
        // Client controls
        controlsText =
            "CONTROLS:\n"
            "WAD: Move/Steer\n"
            "1-9: Set thrust level\n"
            "L: Transform vehicle\n"
            "Z: Zoom out\n"
            "X: Auto-zoom\n"
            "C: Focus planet 2";
    }
    else {
        // Server or single player
        controlsText =
            "CONTROLS:\n"
            "Arrows: Move/Steer\n"
            "1-9: Set thrust level\n"
            "L: Transform vehicle\n"
            "Z: Zoom out\n"
            "X: Auto-zoom\n"
            "C: Focus planet 2";
    }

    if (isMultiplayer) {
        controlsText += "\n\nMULTIPLAYER MODE: ";
        controlsText += isHost ? "SERVER" : "CLIENT";
    }

    controlsPanel.setText(controlsText);
}

void UIManager::update(VehicleManager* vehicleManager, const std::vector<Planet*>& planets, float deltaTime)
{
    updateRocketInfo(vehicleManager);
    updatePlanetInfo(vehicleManager, planets);
    updateOrbitInfo(vehicleManager, planets);
    updateThrustMetrics(vehicleManager, planets);

    if (isMultiplayer) {
        updateMultiplayerInfo(0, true, 0, 0); // Placeholder values
    }
}

void UIManager::render()
{
    // Switch to UI view
    window.setView(uiView);

    // Draw all panels
    rocketInfoPanel.draw(window);
    planetInfoPanel.draw(window);
    orbitInfoPanel.draw(window);
    controlsPanel.draw(window);
    thrustMetricsPanel.draw(window);

    if (isMultiplayer) {
        multiplayerPanel.draw(window);
    }
}

void UIManager::updateRocketInfo(VehicleManager* vehicleManager)
{
    std::stringstream ss;
    if (vehicleManager->getActiveVehicleType() == VehicleType::ROCKET) {
        Rocket* rocket = vehicleManager->getRocket();
        float speed = std::sqrt(rocket->getVelocity().x * rocket->getVelocity().x +
            rocket->getVelocity().y * rocket->getVelocity().y);

        ss << "ROCKET INFO\n"
            << "Mass: " << rocket->getMass() << " units\n"
            << "Speed: " << std::fixed << std::setprecision(1) << speed << " units/s\n"
            << "Velocity: (" << std::setprecision(1) << rocket->getVelocity().x << ", "
            << rocket->getVelocity().y << ")\n";

        // Calculate total gravity force from all planets
        float totalForce = 0.0f;

        // Can't calculate forces here without planets, so this will be updated in the full method
    }
    else {
        Car* car = vehicleManager->getCar();
        ss << "CAR INFO\n"
            << "On Ground: " << (car->isOnGround() ? "Yes" : "No") << "\n"
            << "Position: (" << std::fixed << std::setprecision(1)
            << car->getPosition().x << ", " << car->getPosition().y << ")\n"
            << "Orientation: " << std::setprecision(1) << car->getRotation() << " degrees\n"
            << "Press L to transform back to rocket when on ground";
    }

    rocketInfoPanel.setText(ss.str());
}

void UIManager::updatePlanetInfo(VehicleManager* vehicleManager, const std::vector<Planet*>& planets)
{
    std::stringstream ss;
    Planet* closestPlanet = nullptr;
    float closestDistance = std::numeric_limits<float>::max();
    GameObject* activeVehicle = vehicleManager->getActiveVehicle();

    for (const auto& planetPtr : planets) {
        sf::Vector2f direction = planetPtr->getPosition() - activeVehicle->getPosition();
        float dist = std::sqrt(direction.x * direction.x + direction.y * direction.y);

        if (dist < closestDistance) {
            closestDistance = dist;
            closestPlanet = const_cast<Planet*>(planetPtr);
        }
    }

    if (closestPlanet) {
        std::string planetName = (closestPlanet == planets[0]) ? "Blue Planet" : "Green Planet";
        float speed = std::sqrt(closestPlanet->getVelocity().x * closestPlanet->getVelocity().x +
            closestPlanet->getVelocity().y * closestPlanet->getVelocity().y);

        ss << "NEAREST PLANET: " << planetName << "\n"
            << "Distance: " << std::fixed << std::setprecision(0) << closestDistance << " units\n"
            << "Mass: " << closestPlanet->getMass() << " units\n"
            << "Radius: " << closestPlanet->getRadius() << " units\n"
            << "Speed: " << std::setprecision(1) << speed << " units/s\n"
            << "Surface gravity: " << std::setprecision(2)
            << GameConstants::G * closestPlanet->getMass() /
            (closestPlanet->getRadius() * closestPlanet->getRadius()) << " units/s²";
    }

    planetInfoPanel.setText(ss.str());
}

void UIManager::updateOrbitInfo(VehicleManager* vehicleManager, const std::vector<Planet*>& planets)
{
    std::stringstream ss;

    if (vehicleManager->getActiveVehicleType() == VehicleType::ROCKET) {
        Rocket* rocket = vehicleManager->getRocket();
        Planet* primaryBody = nullptr;
        float strongestGravity = 0.0f;

        // Find the primary gravitational body (usually the closest one)
        for (const auto& planetPtr : planets) {
            sf::Vector2f direction = planetPtr->getPosition() - rocket->getPosition();
            float dist = std::sqrt(direction.x * direction.x + direction.y * direction.y);

            float gravityStrength = GameConstants::G * planetPtr->getMass() / (dist * dist);
            if (gravityStrength > strongestGravity) {
                strongestGravity = gravityStrength;
                primaryBody = const_cast<Planet*>(planetPtr);
            }
        }

        if (primaryBody) {
            // Calculate relative position and velocity to primary body
            sf::Vector2f relPos = rocket->getPosition() - primaryBody->getPosition();
            sf::Vector2f relVel = rocket->getVelocity() - primaryBody->getVelocity();

            // Calculate orbit parameters
            float apoapsis = OrbitalMechanics::calculateApoapsis(relPos, relVel, primaryBody->getMass(), GameConstants::G);
            float periapsis = OrbitalMechanics::calculatePeriapsis(relPos, relVel, primaryBody->getMass(), GameConstants::G);

            // Current distance
            float distance = std::sqrt(relPos.x * relPos.x + relPos.y * relPos.y);

            // Check if orbit is valid (not hyperbolic)
            if (periapsis > 0 && apoapsis > 0) {
                ss << "ORBIT INFO\n"
                    << "Primary: " << (primaryBody == planets[0] ? "Blue Planet" : "Green Planet") << "\n"
                    << "Periapsis: " << std::fixed << std::setprecision(0) << periapsis << " units\n"
                    << "Apoapsis: " << std::setprecision(0) << apoapsis << " units\n"
                    << "Current dist: " << std::setprecision(0) << distance << " units";
            }
            else {
                ss << "ORBIT INFO\n"
                    << "Primary: " << (primaryBody == planets[0] ? "Blue Planet" : "Green Planet") << "\n"
                    << "Orbit: Escape trajectory\n"
                    << "Current dist: " << std::fixed << std::setprecision(0) << distance << " units";
            }
        }
    }
    else {
        ss << "ORBIT INFO\n"
            << "Not available in car mode\n"
            << "Transform to rocket for orbital data";
    }

    orbitInfoPanel.setText(ss.str());
}

void UIManager::updateThrustMetrics(VehicleManager* vehicleManager, const std::vector<Planet*>& planets)
{
    if (vehicleManager->getActiveVehicleType() == VehicleType::ROCKET) {
        Rocket* rocket = vehicleManager->getRocket();
        sf::Vector2f rocketPos = rocket->getPosition();

        // Calculate the closest planet for gravity reference
        Planet* closestPlanet = nullptr;
        float closestDistance = std::numeric_limits<float>::max();

        for (const auto& planetPtr : planets) {
            float dist = std::sqrt(std::pow(rocketPos.x - planetPtr->getPosition().x, 2) +
                std::pow(rocketPos.y - planetPtr->getPosition().y, 2));
            if (dist < closestDistance) {
                closestDistance = dist;
                closestPlanet = const_cast<Planet*>(planetPtr);
            }
        }

        if (closestPlanet) {
            // Calculate gravity force and direction
            sf::Vector2f towardsPlanet = closestPlanet->getPosition() - rocketPos;
            float dist = std::sqrt(towardsPlanet.x * towardsPlanet.x + towardsPlanet.y * towardsPlanet.y);
            sf::Vector2f gravityDir;
            if (dist > 0) {
                gravityDir = sf::Vector2f(towardsPlanet.x / dist, towardsPlanet.y / dist);
            }
            else {
                gravityDir = sf::Vector2f(0, 0);
            }

            // Calculate weight (gravity force) at current position
            float weight = GameConstants::G * rocket->getMass() * closestPlanet->getMass() / (dist * dist);

            // Calculate current thrust force based on thrust level
            float maxThrust = 0.0f;
            for (const auto& part : rocket->getParts()) {
                if (auto* engine = dynamic_cast<Engine*>(part.get())) {
                    maxThrust += engine->getThrust();
                }
            }
            float currentThrust = maxThrust * rocket->getThrustLevel();

            // Calculate expected acceleration (thrust/mass - gravity)
            float thrustToWeightRatio = currentThrust / (weight > 0 ? weight : 1.0f);

            // Calculate expected acceleration along the thrust direction
            float radians = rocket->getRotation() * 3.14159f / 180.0f;
            sf::Vector2f thrustDir(std::sin(radians), -std::cos(radians));

            // Project gravity onto thrust direction (negative if opposing thrust)
            float projectedGravity = gravityDir.x * thrustDir.x + gravityDir.y * thrustDir.y;
            float gravityComponent = weight * projectedGravity;

            // Net acceleration along thrust direction
            float netAccel = (currentThrust - gravityComponent) / rocket->getMass();

            // Set the text content
            std::stringstream ss;
            ss << "THRUST METRICS\n"
                << "Thrust Level: " << std::fixed << std::setprecision(2) << rocket->getThrustLevel() * 100.0f << "%\n"
                << "Thrust-to-Weight Ratio: " << std::setprecision(2) << thrustToWeightRatio << "\n"
                << "Expected Acceleration: " << std::setprecision(2) << netAccel << " units/s²\n"
                << "Escape Velocity: " << std::setprecision(0)
                << std::sqrt(2.0f * GameConstants::G * closestPlanet->getMass() / dist) << " units/s";

            thrustMetricsPanel.setText(ss.str());
        }
        else {
            thrustMetricsPanel.setText("THRUST METRICS\nNo planet in range");
        }
    }
    else {
        thrustMetricsPanel.setText("THRUST METRICS\nNot available in car mode");
    }
}

void UIManager::updateMultiplayerInfo(int connectedClients, bool connected, int playerId, int pingMs)
{
    std::stringstream ss;
    ss << "MULTIPLAYER STATUS\n";
    ss << "Mode: " << (isHost ? "Host" : "Client") << "\n";

    if (isHost) {
        ss << "Connected clients: " << connectedClients << "\n";
    }
    else {
        ss << "Connected to server: " << (connected ? "Yes" : "No") << "\n";
        ss << "Local player ID: " << playerId << "\n";
        ss << "Ping: " << pingMs << "ms\n";
    }

    multiplayerPanel.setText(ss.str());
}