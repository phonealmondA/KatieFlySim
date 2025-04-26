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
    selectedPlanet(nullptr),
    activeVehicleManager(nullptr),
    // Initialize all panels
    rocketInfoPanel(font, 14, sf::Vector2f(10, 10), sf::Vector2f(300, 150)),
    planetInfoPanel(font, 14, sf::Vector2f(10, 170), sf::Vector2f(300, 120)),
    orbitInfoPanel(font, 14, sf::Vector2f(10, 300), sf::Vector2f(300, 100)),
    controlsPanel(font, 14, sf::Vector2f(10, 410), sf::Vector2f(300, 160)),
    thrustMetricsPanel(font, 14, sf::Vector2f(10, 580), sf::Vector2f(300, 80)),
    multiplayerPanel(font, 14, sf::Vector2f(window.getSize().x - 310, 10), sf::Vector2f(300, 100)),
    // Initialize mass transfer buttons
    increaseMassButton(
        sf::Vector2f(320, 20),
        sf::Vector2f(80, 30),
        "fuel --",
        font,
        [this]() {
            if (selectedPlanet && activeVehicleManager) {
                Rocket* rocket = activeVehicleManager->getRocket();
                if (rocket && rocket->getStoredMass() > 0.0f) {
                    // Transfer 0.01 units of mass from rocket to planet
                    float massToTransfer = 0.05f;
                    rocket->addStoredMass(-massToTransfer); // Remove from rocket
                    selectedPlanet->setMass(selectedPlanet->getMass() + massToTransfer); // Add to planet
                }
            }
        }
    ),
    decreaseMassButton(
        sf::Vector2f(320, 60),
        sf::Vector2f(80, 30),
        "fuel ++",
        font,
        [this]() {
            if (selectedPlanet && activeVehicleManager) {
                Rocket* rocket = activeVehicleManager->getRocket();
                if (rocket && selectedPlanet->getMass() > 1.0f) { // Don't let planet go below 1 mass
                    // Transfer 0.01 units of mass from planet to rocket
                    float massToTransfer = 0.05f;
                    selectedPlanet->setMass(selectedPlanet->getMass() - massToTransfer); // Remove from planet
                    rocket->addStoredMass(massToTransfer); // Add to rocket
                }
            }
        }
    ),
    // Initialize engine upgrade buttons
    increaseThrustButton(
        sf::Vector2f(320, 100),  // Position below the other buttons
        sf::Vector2f(80, 30),
        "Thrust +",
        font,
        [this]() {
            if (activeVehicleManager &&
                activeVehicleManager->getActiveVehicleType() == VehicleType::ROCKET) {
                Rocket* rocket = activeVehicleManager->getRocket();
                if (rocket) {
                    rocket->upgradeThrust(upgradeCost);
                }
            }
        }
    ),
    increaseEfficiencyButton(
        sf::Vector2f(320, 140),  // Position below thrust button
        sf::Vector2f(80, 30),
        "Fuel Eff +",
        font,
        [this]() {
            if (activeVehicleManager &&
                activeVehicleManager->getActiveVehicleType() == VehicleType::ROCKET) {
                Rocket* rocket = activeVehicleManager->getRocket();
                if (rocket) {
                    rocket->upgradeEfficiency(upgradeCost);
                }
            }
        }
    )
{
    // Constructor body - can be empty
}

void UIManager::update(VehicleManager* vehicleManager, const std::vector<Planet*>& planets, float deltaTime)
{
    // Store the current vehicle manager
    activeVehicleManager = vehicleManager;

    // Only update if we have a valid vehicle manager
    if (!vehicleManager) return;

    // Find nearest planet for planet info display and mass buttons
    nearestPlanet = findNearestPlanet(vehicleManager, planets);

    // If no planet is manually selected, use the nearest
    if (!selectedPlanet) {
        selectedPlanet = nearestPlanet;
    }
    else {
        // Check if the selected planet still exists in the planets vector
        bool found = false;
        for (auto* planet : planets) {
            if (planet == selectedPlanet) {
                found = true;
                break;
            }
        }
        if (!found) {
            // The selected planet was removed, fall back to nearest
            selectedPlanet = nearestPlanet;
        }
    }

    // Update UI panels
    updateRocketInfo(vehicleManager);
    updatePlanetInfo(vehicleManager, planets);
    updateOrbitInfo(vehicleManager, planets);
    updateThrustMetrics(vehicleManager, planets);
    updateControlsInfo();
    // Update button states - check if mouse is hovering over buttons
    sf::Vector2i mousePosition = sf::Mouse::getPosition(window);
    sf::Vector2f mousePosView = window.mapPixelToCoords(mousePosition, uiView);

    increaseMassButton.update(mousePosView);
    decreaseMassButton.update(mousePosView);

    // Also update the new engine upgrade buttons
    increaseThrustButton.update(mousePosView);
    increaseEfficiencyButton.update(mousePosView);

    // Check for button clicks
    if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
        increaseMassButton.handleClick();
        decreaseMassButton.handleClick();

        // Also handle clicks for the new buttons
        increaseThrustButton.handleClick();
        increaseEfficiencyButton.handleClick();
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
    if (selectedPlanet) {
        increaseMassButton.draw(window);
        decreaseMassButton.draw(window);

        // Draw button labels
        sf::Text plusText(font, "");
        plusText.setString("fuel --");
        plusText.setCharacterSize(20);
        plusText.setFillColor(sf::Color::White);
        plusText.setPosition(increaseMassButton.getPosition() + sf::Vector2f(10, 3));
        window.draw(plusText);

        sf::Text minusText(font, "");
        minusText.setString("fuel ++");
        minusText.setCharacterSize(20);
        minusText.setFillColor(sf::Color::White);
        minusText.setPosition(decreaseMassButton.getPosition() + sf::Vector2f(10, 3));
        window.draw(minusText);
    }

    // Draw engine upgrade buttons when in rocket mode
    if (activeVehicleManager && activeVehicleManager->getActiveVehicleType() == VehicleType::ROCKET) {
        increaseThrustButton.draw(window);
        increaseEfficiencyButton.draw(window);

        // Draw button labels
        sf::Text thrustText(font, "");
        thrustText.setString("Thrust +");
        thrustText.setCharacterSize(16);
        thrustText.setFillColor(sf::Color::White);
        thrustText.setPosition(increaseThrustButton.getPosition() + sf::Vector2f(10, 7));
        window.draw(thrustText);

        sf::Text effText(font, "");
        effText.setString("Fuel Eff +");
        effText.setCharacterSize(16);
        effText.setFillColor(sf::Color::White);
        effText.setPosition(increaseEfficiencyButton.getPosition() + sf::Vector2f(5, 7));
        window.draw(effText);

        // Show upgrade cost
        sf::Text costText(font, "");
        std::stringstream ss;
        ss << "Cost: " << std::fixed << std::setprecision(2) << upgradeCost;
        costText.setString(ss.str());
        costText.setCharacterSize(12);
        costText.setFillColor(sf::Color::Yellow);
        costText.setPosition(increaseThrustButton.getPosition() + sf::Vector2f(85, 10));
        window.draw(costText);

        sf::Text costText2 = costText;
        costText2.setPosition(increaseEfficiencyButton.getPosition() + sf::Vector2f(85, 10));
        window.draw(costText2);
    }

    // Restore previous view to draw selected planet highlight in world coordinates
    window.setView(currentView);

    // Highlight the selected planet with a circle outline
    if (selectedPlanet) {
        // Draw a highlight circle around the selected planet
        sf::CircleShape highlight;
        highlight.setRadius(selectedPlanet->getRadius() + 5.0f);  // Slightly larger than planet
        highlight.setOrigin(sf::Vector2f(highlight.getRadius(), highlight.getRadius()));
        highlight.setPosition(selectedPlanet->getPosition());
        highlight.setFillColor(sf::Color::Transparent);
        highlight.setOutlineColor(sf::Color::Yellow);
        highlight.setOutlineThickness(2.0f);
        window.draw(highlight);
    }

    // Restore UI view
    window.setView(uiView);
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
            ss << "Fuel: " << std::fixed << std::setprecision(1)
                << rocket->getStoredMass() << " units\n";
            ss << "Thrust Level: " << std::fixed << std::setprecision(1)
                << rocket->getThrustLevel() * 100.0f << "%\n";

            // Add upgrade info
            ss << "Thrust Mult: " << std::fixed << std::setprecision(1)
                << rocket->getThrustMultiplier() << "x\n";
            ss << "Efficiency: " << std::fixed << std::setprecision(1)
                << rocket->getEfficiencyMultiplier() << "x";

            // Add a fuel warning if low
            if (rocket->getStoredMass() < 0.2f) {
                ss << "\nFUEL LOW!";
            }
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
Planet* UIManager::findNearestPlanet(VehicleManager* vehicleManager, const std::vector<Planet*>& planets) {
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

        // Find the actual closest planet, regardless of size
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

    // Use selectedPlanet instead of nearestPlanet
    Planet* targetPlanet = selectedPlanet;
    if (!targetPlanet) return;

    // Calculate distance to selected planet
    GameObject* vehicle = vehicleManager->getActiveVehicle();
    if (!vehicle) return;
    float dist = std::sqrt(
        std::pow(vehicle->getPosition().x - targetPlanet->getPosition().x, 2) +
        std::pow(vehicle->getPosition().y - targetPlanet->getPosition().y, 2)
    );

    // Calculate planet speed
    float planetSpeed = std::sqrt(
        std::pow(targetPlanet->getVelocity().x, 2) +
        std::pow(targetPlanet->getVelocity().y, 2)
    );

    std::stringstream ss;
    ss << "Selected Planet Info:\n";  // Changed text to indicate selection
    ss << "Distance: " << std::fixed << std::setprecision(1) << dist << "\n";
    ss << "Mass: " << std::fixed << std::setprecision(1) << targetPlanet->getMass() << "\n";
    ss << "Radius: " << std::fixed << std::setprecision(1) << targetPlanet->getRadius() << "\n";
    ss << "Speed: " << std::fixed << std::setprecision(1) << planetSpeed << "\n";
    ss << "Surface Gravity: " << std::fixed << std::setprecision(2)
        << (GameConstants::G * targetPlanet->getMass() /
            (targetPlanet->getRadius() * targetPlanet->getRadius())) << "\n";
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

    // Use the selected planet
    Planet* targetPlanet = selectedPlanet;
    if (!targetPlanet) return;

    // Get relative position and velocity
    sf::Vector2f relPos = rocket->getPosition() - targetPlanet->getPosition();
    sf::Vector2f relVel = rocket->getVelocity() - targetPlanet->getVelocity();

    // Calculate orbital parameters
    float periapsis = OrbitalMechanics::calculatePeriapsis(relPos, relVel, targetPlanet->getMass(), GameConstants::G);
    float apoapsis = OrbitalMechanics::calculateApoapsis(relPos, relVel, targetPlanet->getMass(), GameConstants::G);
    float period = OrbitalMechanics::calculateOrbitalPeriod(
        (periapsis + apoapsis) / 2.0f, targetPlanet->getMass(), GameConstants::G);
    float eccentricity = OrbitalMechanics::calculateEccentricity(relPos, relVel, targetPlanet->getMass(), GameConstants::G);

    std::stringstream ss;
    ss << "Orbit Info (selected planet):\n";  // Updated text

    // If we're in a valid orbit
    if (periapsis > targetPlanet->getRadius() && !std::isnan(periapsis) && !std::isnan(apoapsis) &&
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

void UIManager::updateControlsInfo()
{
    std::stringstream ss;
    ss << "Controls:\n";
    ss << "Arrow Keys: Move rocket\n";
    ss << "0-9: Set thrust level (0-90%)\n";
    ss << "=: Set thrust to 100%\n";
    ss << "L: Transform to/from car\n";
    ss << "Tab: Cycle selected planet\n";
    ss << "-: Drop stored mass as planet\n";
    ss << "Z/X: Zoom out/auto-zoom";

    controlsPanel.setText(ss.str());
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

    // Get current thrust level
    float currentThrust = maxThrust * rocket->getThrustLevel();

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

    // Calculate fuel consumption rate
    float consumptionRate = GameConstants::BASE_FUEL_CONSUMPTION_RATE * rocket->getThrustLevel();
    float burnTimeRemaining = rocket->getStoredMass() / consumptionRate;
    if (rocket->getThrustLevel() < 0.001f) {
        burnTimeRemaining = 0.0f; // Avoid division by near-zero
    }

    std::stringstream ss;
    ss << "Thrust Metrics:\n";
    ss << "Thrust: " << std::fixed << std::setprecision(1) << currentThrust << "\n";
    ss << "TWR: " << std::fixed << std::setprecision(2) << twr << "\n";

    // Only show burn time if we have fuel and are thrusting
    if (rocket->hasFuel() && rocket->getThrustLevel() > 0.001f) {
        ss << "Burn time: " << std::fixed << std::setprecision(1)
            << burnTimeRemaining << "s";
    }

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