// UIManager.cpp
#include "UIManager.h"
#include "OrbitalMechanics.h"
#include <sstream>
#include <iomanip>
#include <cmath>
#include <string>

UIManager::UIManager(sf::RenderWindow& window, sf::Font& font, sf::View& uiView, bool multiplayer, bool host)
    : window(window),
    font(font),
    uiView(uiView),
    isMultiplayer(multiplayer),
    isHost(host),
    nearestPlanet(nullptr),
    activeVehicleManager(nullptr),
    // Initialize all objects in the initializer list
    rocketInfoPanel(font, 14, sf::Vector2f(10, 10), sf::Vector2f(300, 150)),
    planetInfoPanel(font, 14, sf::Vector2f(10, 170), sf::Vector2f(300, 120)),
    orbitInfoPanel(font, 14, sf::Vector2f(10, 300), sf::Vector2f(300, 100)),
    controlsPanel(font, 14, sf::Vector2f(10, 410), sf::Vector2f(300, 160)),
    thrustMetricsPanel(font, 14, sf::Vector2f(10, 580), sf::Vector2f(300, 80)),
    multiplayerPanel(font, 14, sf::Vector2f(window.getSize().x - 310, 10), sf::Vector2f(300, 100)),
    increaseMassButton(
        sf::Vector2f(320, 120), // Position to the right of planetInfoPanel
        sf::Vector2f(30, 30),   // Small square button
        "+",                    // Plus symbol
        font,
        [this]() {
            if (nearestPlanet && activeVehicleManager) {
                Rocket* rocket = activeVehicleManager->getRocket();
                if (rocket && rocket->getStoredMass() > 0.0f) {
                    // Transfer 1 unit of mass from rocket to planet
                    float massToTransfer = 0.1f;
                    rocket->addStoredMass(-massToTransfer); // Remove from rocket
                    nearestPlanet->setMass(nearestPlanet->getMass() + massToTransfer); // Add to planet
                }
            }
        }
    ),
    decreaseMassButton(
        sf::Vector2f(320, 160), // Position below increase button
        sf::Vector2f(30, 30),   // Small square button
        "-",                    // Minus symbol
        font,
        [this]() {
            if (nearestPlanet && activeVehicleManager) {
                Rocket* rocket = activeVehicleManager->getRocket();
                if (rocket && nearestPlanet->getMass() > 1.0f) { // Don't let planet go below 1 mass
                    // Transfer 1 unit of mass from planet to rocket
                    float massToTransfer = 0.1f;
                    nearestPlanet->setMass(nearestPlanet->getMass() - massToTransfer); // Remove from planet
                    rocket->addStoredMass(massToTransfer); // Add to rocket
                }
            }
        }
    )
{
    // Set default control instructions
    std::string controlText;
    if (isMultiplayer && !isHost) {
        // Client controls
        controlText = "Controls:\n"
            "W - Forward Thrust\n"
            "S - Backward Thrust\n"
            "A - Rotate Left\n"
            "D - Rotate Right\n"
            "L - Transform Vehicle\n"
            "1-9 - Set Thrust Level\n"
            "0 - Zero Thrust";
    }
    else {
        // Host or single player controls
        controlText = "Controls:\n"
            "Arrow Up - Forward Thrust\n"
            "Arrow Down - Backward Thrust\n"
            "Arrow Left - Rotate Left\n"
            "Arrow Right - Rotate Right\n"
            "L - Transform Vehicle\n"
            "1-9 - Set Thrust Level\n"
            "0 - Zero Thrust";
    }
    controlsPanel.setText(controlText);
}

void UIManager::update(VehicleManager* vehicleManager, const std::vector<Planet*>& planets, float deltaTime)
{
    // Store the current vehicle manager
    activeVehicleManager = vehicleManager;

    // Only update if we have a valid vehicle manager
    if (!vehicleManager) return;

    // Find nearest planet for planet info display and mass buttons
    nearestPlanet = findNearestPlanet(vehicleManager, planets);

    // Update UI panels
    updateRocketInfo(vehicleManager);
    updatePlanetInfo(vehicleManager, planets);
    updateOrbitInfo(vehicleManager, planets);
    updateThrustMetrics(vehicleManager, planets);

    // Update button states - check if mouse is hovering over buttons
    sf::Vector2i mousePosition = sf::Mouse::getPosition(window);
    sf::Vector2f mousePosView = window.mapPixelToCoords(mousePosition, uiView);

    increaseMassButton.update(mousePosView);
    decreaseMassButton.update(mousePosView);

    // Check for button clicks
    if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
        increaseMassButton.handleClick();
        decreaseMassButton.handleClick();
    }
}

void UIManager::render()
{
    // Save current view
    sf::View currentView = window.getView();

    // Apply UI view for interface rendering
    window.setView(uiView);

    // Draw UI panels
    rocketInfoPanel.draw(window);
    planetInfoPanel.draw(window);
    orbitInfoPanel.draw(window);
    controlsPanel.draw(window);
    thrustMetricsPanel.draw(window);

    // Draw multiplayer panel if in multiplayer mode
    if (isMultiplayer) {
        multiplayerPanel.draw(window);
    }

    // Draw planet mass control buttons
    if (nearestPlanet) {
        increaseMassButton.draw(window);
        decreaseMassButton.draw(window);

        // Draw button labels
        sf::Text plusText(font, "");
        plusText.setString("+");
        plusText.setCharacterSize(20);
        plusText.setFillColor(sf::Color::White);
        plusText.setPosition(increaseMassButton.getPosition() + sf::Vector2f(10, 3));
        window.draw(plusText);

        sf::Text minusText(font, "");
        minusText.setString("-");
        minusText.setCharacterSize(20);
        minusText.setFillColor(sf::Color::White);
        minusText.setPosition(decreaseMassButton.getPosition() + sf::Vector2f(10, 3));
        window.draw(minusText);
    }

    // Restore previous view
    window.setView(currentView);
}

void UIManager::updateRocketInfo(VehicleManager* vehicleManager)
{
    if (!vehicleManager) return;

    std::stringstream ss;

    if (vehicleManager->getActiveVehicleType() == VehicleType::ROCKET) {
        Rocket* rocket = vehicleManager->getRocket();
        if (!rocket) return;

        ss << "Rocket Info:\n";
        ss << "Position: (" << std::fixed << std::setprecision(1)
            << rocket->getPosition().x << ", " << rocket->getPosition().y << ")\n";
        ss << "Velocity: (" << std::fixed << std::setprecision(1)
            << rocket->getVelocity().x << ", " << rocket->getVelocity().y << ")\n";
        ss << "Speed: " << std::fixed << std::setprecision(1)
            << std::sqrt(rocket->getVelocity().x * rocket->getVelocity().x +
                rocket->getVelocity().y * rocket->getVelocity().y) << "\n";
        ss << "Mass: " << rocket->getMass() << std::endl;
        ss << "Stored Mass: " << rocket->getStoredMass() << "\n";
        ss << "Thrust Level: " << std::fixed << std::setprecision(1)
            << rocket->getThrustLevel() * 100.0f << "%";
    }
    else {
        Car* car = vehicleManager->getCar();
        if (!car) return;

        ss << "Car Info:\n";
        ss << "Position: (" << std::fixed << std::setprecision(1)
            << car->getPosition().x << ", " << car->getPosition().y << ")\n";
        ss << "Direction: " << (car->getIsFacingRight() ? "Right" : "Left") << "\n";
        ss << "On Ground: " << (car->isOnGround() ? "Yes" : "No");
    }

    rocketInfoPanel.setText(ss.str());
}

Planet* UIManager::findNearestPlanet(VehicleManager* vehicleManager, const std::vector<Planet*>& planets)
{
    if (!vehicleManager || planets.empty()) return nullptr;

    GameObject* vehicle = vehicleManager->getActiveVehicle();
    if (!vehicle) return nullptr;

    Planet* closest = nullptr;
    float closestDistance = std::numeric_limits<float>::max();

    for (Planet* planet : planets) {
        if (!planet) continue;

        float dist = std::sqrt(
            std::pow(vehicle->getPosition().x - planet->getPosition().x, 2) +
            std::pow(vehicle->getPosition().y - planet->getPosition().y, 2)
        );

        if (dist < closestDistance) {
            closestDistance = dist;
            closest = planet;
        }
    }

    return closest;
}

void UIManager::updatePlanetInfo(VehicleManager* vehicleManager, const std::vector<Planet*>& planets)
{
    if (!vehicleManager || planets.empty()) return;
    // Find nearest planet - already found in update()
    Planet* closestPlanet = nearestPlanet;
    if (!closestPlanet) return;
    // Calculate distance to nearest planet
    GameObject* vehicle = vehicleManager->getActiveVehicle();
    if (!vehicle) return;
    float dist = std::sqrt(
        std::pow(vehicle->getPosition().x - closestPlanet->getPosition().x, 2) +
        std::pow(vehicle->getPosition().y - closestPlanet->getPosition().y, 2)
    );

    // Calculate planet speed
    float planetSpeed = std::sqrt(
        std::pow(closestPlanet->getVelocity().x, 2) +
        std::pow(closestPlanet->getVelocity().y, 2)
    );

    std::stringstream ss;
    ss << "Nearest Planet Info:\n";
    ss << "Distance: " << std::fixed << std::setprecision(1) << dist << "\n";
    ss << "Mass: " << std::fixed << std::setprecision(1) << closestPlanet->getMass() << "\n";
    ss << "Radius: " << std::fixed << std::setprecision(1) << closestPlanet->getRadius() << "\n";
    ss << "Speed: " << std::fixed << std::setprecision(1) << planetSpeed << "\n";
    ss << "Surface Gravity: " << std::fixed << std::setprecision(2)
        << (GameConstants::G * closestPlanet->getMass() /
            (closestPlanet->getRadius() * closestPlanet->getRadius())) << "\n";
    // Add the mass adjustment hint
    ss << "Click +/- to transfer mass";
    planetInfoPanel.setText(ss.str());
}

void UIManager::updateOrbitInfo(VehicleManager* vehicleManager, const std::vector<Planet*>& planets)
{
    if (!vehicleManager || planets.empty() ||
        vehicleManager->getActiveVehicleType() != VehicleType::ROCKET) return;

    Rocket* rocket = vehicleManager->getRocket();
    if (!rocket) return;

    // Use the already found nearest planet
    Planet* closestPlanet = nearestPlanet;
    if (!closestPlanet) return;

    // Get relative position and velocity
    sf::Vector2f relPos = rocket->getPosition() - closestPlanet->getPosition();
    sf::Vector2f relVel = rocket->getVelocity() - closestPlanet->getVelocity();

    // Calculate orbital parameters
    float periapsis = OrbitalMechanics::calculatePeriapsis(relPos, relVel, closestPlanet->getMass(), GameConstants::G);
    float apoapsis = OrbitalMechanics::calculateApoapsis(relPos, relVel, closestPlanet->getMass(), GameConstants::G);
    float period = OrbitalMechanics::calculateOrbitalPeriod(
        (periapsis + apoapsis) / 2.0f, closestPlanet->getMass(), GameConstants::G);
    float eccentricity = OrbitalMechanics::calculateEccentricity(relPos, relVel, closestPlanet->getMass(), GameConstants::G);

    std::stringstream ss;
    ss << "Orbit Info (nearest planet):\n";

    // If we're in a valid orbit
    if (periapsis > closestPlanet->getRadius() && !std::isnan(periapsis) && !std::isnan(apoapsis) &&
        apoapsis > periapsis && eccentricity < 1.0f) {
        ss << "Periapsis: " << std::fixed << std::setprecision(1) << periapsis << "\n";
        ss << "Apoapsis: " << std::fixed << std::setprecision(1) << apoapsis << "\n";
        ss << "Period: " << std::fixed << std::setprecision(1) << period << "s\n";
        ss << "Eccentricity: " << std::fixed << std::setprecision(3) << eccentricity;
    }
    else if (eccentricity >= 1.0f) {
        ss << "Hyperbolic trajectory\n";
        ss << "Periapsis: " << std::fixed << std::setprecision(1) << periapsis << "\n";
        ss << "Eccentricity: " << std::fixed << std::setprecision(3) << eccentricity;
    }
    else {
        ss << "Not in stable orbit\n";
        ss << "Impact predicted!";
    }

    orbitInfoPanel.setText(ss.str());
}

void UIManager::updateThrustMetrics(VehicleManager* vehicleManager, const std::vector<Planet*>& planets)
{
    if (!vehicleManager || vehicleManager->getActiveVehicleType() != VehicleType::ROCKET) return;

    Rocket* rocket = vehicleManager->getRocket();
    if (!rocket) return;

    // Get max thrust for the rocket
    float maxThrust = 0.0f;
    for (const auto& part : rocket->getParts()) {
        if (const Engine* engine = dynamic_cast<const Engine*>(part.get())) {
            maxThrust += engine->getThrust();
        }
    }

    // Get current thrust level moddddd
    float currentThrust = 100 * rocket->getThrustLevel();

    // Calculate TWR (Thrust to Weight Ratio)
    float totalMass = rocket->getMass();
    float weight = 0.0f;

    // Find planet generating most gravity
    if (!planets.empty() && nearestPlanet) {
        // Use nearest planet for weight calculation
        sf::Vector2f relPos = rocket->getPosition() - nearestPlanet->getPosition();
        float distance = std::sqrt(relPos.x * relPos.x + relPos.y * relPos.y);
        weight = GameConstants::G * nearestPlanet->getMass() * totalMass /
            (distance * distance);
    }

    float twr = (weight > 0) ? currentThrust / weight : 0.0f;

    std::stringstream ss;
    ss << "Thrust Metrics:\n";
    ss << "Current Thrust: " << std::fixed << std::setprecision(1) << currentThrust << "\n";
    ss << "TWR: " << std::fixed << std::setprecision(2) << twr;

    thrustMetricsPanel.setText(ss.str());
}

void UIManager::updateMultiplayerInfo(int connectedClients, bool connected, int playerId, int pingMs)
{
    if (!isMultiplayer) return;

    std::stringstream ss;
    ss << "Network Info:\n";

    if (isHost) {
        ss << "Role: Server\n";
        ss << "Connected clients: " << connectedClients << "\n";
    }
    else {
        ss << "Role: Client\n";
        ss << "Player ID: " << playerId << "\n";
        ss << "Ping: " << pingMs << "ms\n";
    }

    ss << "Status: " << (connected ? "Connected" : "Disconnected");

    multiplayerPanel.setText(ss.str());
}