// GameManager.cpp
#include "GameManager.h"
#include "TextPanel.h"
#include "OrbitalMechanics.h"
#include "GameConstants.h"
#include "UIManager.h"  // Add this include
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <limits>

// In GameManager.cpp, update the constructor
GameManager::GameManager(sf::RenderWindow& window, UIManager* ui)
    : window(window),
    gameView(sf::Vector2f(640.f, 360.f), sf::Vector2f(1280.f, 720.f)),
    uiView(sf::Vector2f(640.f, 360.f), sf::Vector2f(1280.f, 720.f)),
    zoomLevel(1.0f),
    targetZoom(1.0f),
    uiManager(ui)
{
    // Initialize clock
    clock.restart();
}

GameManager::~GameManager()
{
    cleanup();
}

void GameManager::initialize()
{
    // Setup game views
    zoomLevel = 1.0f;
    targetZoom = 1.0f;

    // Create planets in single player mode
    Planet* planet = new Planet(sf::Vector2f(GameConstants::MAIN_PLANET_X, GameConstants::MAIN_PLANET_Y),
        0, GameConstants::MAIN_PLANET_MASS, sf::Color::Blue);
    planet->setVelocity(sf::Vector2f(0.f, 0.f));

    Planet* planet2 = new Planet(sf::Vector2f(GameConstants::SECONDARY_PLANET_X, GameConstants::SECONDARY_PLANET_Y),
        0, GameConstants::SECONDARY_PLANET_MASS, sf::Color::Green);
    planet2->setVelocity(sf::Vector2f(0.f, GameConstants::SECONDARY_PLANET_ORBITAL_VELOCITY));

    planets.push_back(planet);
    planets.push_back(planet2);

    // Calculate position for rocket - start at the top of the first planet
    sf::Vector2f planetPos = planet->getPosition();
    float planetRadius = planet->getRadius();
    sf::Vector2f direction(0, -1);
    sf::Vector2f rocketPos = planetPos + direction * (planetRadius + GameConstants::ROCKET_SIZE);

    // Create vehicle manager
    activeVehicleManager = new VehicleManager(rocketPos, planets);

    // Set up gravity simulator
    gravitySimulator.setSimulatePlanetGravity(true);
    for (auto planet : planets) {
        gravitySimulator.addPlanet(planet);
    }
    gravitySimulator.addVehicleManager(activeVehicleManager);
}


void GameManager::update(float deltaTime)
{
    // Update simulation - this may remove planets through collision detection
    gravitySimulator.update(deltaTime);

    // Refresh the planets list from the simulator to avoid accessing deleted planets
    planets = gravitySimulator.getPlanets();

    // Now update the remaining valid planets
    for (auto* planet : planets) {
        if (planet) { // Null check still good to have
            planet->update(deltaTime);
        }
    }

    // Update active vehicle
    activeVehicleManager->update(deltaTime);

    // Handle camera/zoom
    updateCamera(deltaTime);
}




void GameManager::updateCamera(float deltaTime)
{
    // Calculate distance from vehicle to planets for automatic zoom
    sf::Vector2f vehiclePos = activeVehicleManager->getActiveVehicle()->getPosition();
    sf::Vector2f vehicleToPlanet1 = planets[0]->getPosition() - vehiclePos;
    sf::Vector2f vehicleToPlanet2 = planets[1]->getPosition() - vehiclePos;
    float distance1 = std::sqrt(vehicleToPlanet1.x * vehicleToPlanet1.x + vehicleToPlanet1.y * vehicleToPlanet1.y);
    float distance2 = std::sqrt(vehicleToPlanet2.x * vehicleToPlanet2.x + vehicleToPlanet2.y * vehicleToPlanet2.y);

    const float minZoom = 1.0f;
    const float maxZoom = 1000.0f;
    const float zoomSpeed = 1.0f;
    // Use closest planet for zoom calculation if not manually zooming
    if (!sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Z) &&
        !sf::Keyboard::isKeyPressed(sf::Keyboard::Key::X) &&
        !sf::Keyboard::isKeyPressed(sf::Keyboard::Key::C)) {
        float closest = std::min(distance1, distance2);
        targetZoom = minZoom + (closest - (planets[0]->getRadius() + GameConstants::ROCKET_SIZE)) / 100.0f;
        targetZoom = std::max(minZoom, std::min(targetZoom, maxZoom));

        // Update view center to follow vehicle
        gameView.setCenter(vehiclePos);
    }

    // Smoothly interpolate current zoom to target zoom
    zoomLevel += (targetZoom - zoomLevel) * deltaTime * zoomSpeed;

    // Set view size based on zoom level
    gameView.setSize(sf::Vector2f(1280.f * zoomLevel, 720.f * zoomLevel));
}

void GameManager::render()
{
    // Apply the game view for world rendering
    window.setView(gameView);

    // Clear window
    window.clear(sf::Color::Black);

    // Find the closest planet to the rocket for orbit path
    Planet* closestPlanet = nullptr;
    float closestDistance = std::numeric_limits<float>::max();
    sf::Vector2f rocketPos = activeVehicleManager->getActiveVehicle()->getPosition();

    for (auto& planetPtr : planets) {
        sf::Vector2f direction = planetPtr->getPosition() - rocketPos;
        float dist = std::sqrt(direction.x * direction.x + direction.y * direction.y);

        if (dist < closestDistance) {
            closestDistance = dist;
            closestPlanet = planetPtr;
        }
    }

    // Draw orbit path only for the closest planet
    if (closestPlanet) {
        closestPlanet->drawOrbitPath(window, planets);
    }

    // Draw trajectory only if in rocket mode
    if (activeVehicleManager->getActiveVehicleType() == VehicleType::ROCKET) {
        activeVehicleManager->getRocket()->drawTrajectory(window, planets,
            GameConstants::TRAJECTORY_TIME_STEP, GameConstants::TRAJECTORY_STEPS, false);
    }

    // Draw planets
    for (auto planet : planets) {
        planet->draw(window);
        planet->drawVelocityVector(window, 5.0f);
    }

    // Draw active vehicle
    activeVehicleManager->drawWithConstantSize(window, zoomLevel);

    // Draw velocity vector if in rocket mode
    if (activeVehicleManager->getActiveVehicleType() == VehicleType::ROCKET) {
        activeVehicleManager->drawVelocityVector(window, 2.0f);
        activeVehicleManager->getRocket()->drawGravityForceVectors(window, planets, GameConstants::GRAVITY_VECTOR_SCALE);
    }
}

void GameManager::handleEvents()
{
    static bool lKeyPressed = false;
    static bool tabKeyPressed = false;  // Track Tab key state

    if (std::optional<sf::Event> event = window.pollEvent())
    {
        if (event->is<sf::Event::Closed>())
            window.close();

        if (event->is<sf::Event::Resized>())
        {
            const auto* resizeEvent = event->getIf<sf::Event::Resized>();
            if (resizeEvent)
            {
                float aspectRatio = static_cast<float>(resizeEvent->size.x) / static_cast<float>(resizeEvent->size.y);
                gameView.setSize(sf::Vector2f(
                    resizeEvent->size.y * aspectRatio * zoomLevel,
                    resizeEvent->size.y * zoomLevel
                ));

                // Update UI view
                uiView.setSize(sf::Vector2f(
                    static_cast<float>(resizeEvent->size.x),
                    static_cast<float>(resizeEvent->size.y)
                ));
                uiView.setCenter(sf::Vector2f(
                    static_cast<float>(resizeEvent->size.x) / 2.0f,
                    static_cast<float>(resizeEvent->size.y) / 2.0f
                ));

                window.setView(gameView);
            }
        }

        // Modify in main.cpp - add to the event handling section where key presses are processed
// (inside gameManager.handleEvents() or similar function)

        if (event->is<sf::Event::KeyPressed>()) {
            const auto* keyEvent = event->getIf<sf::Event::KeyPressed>();
            if (keyEvent) {
                if (keyEvent->code == sf::Keyboard::Key::Escape)
                    window.close();
                else if (keyEvent->code == sf::Keyboard::Key::P) {
                    // Toggle planet gravity simulation
                    static bool planetGravity = true;
                    planetGravity = !planetGravity;
                    gravitySimulator.setSimulatePlanetGravity(planetGravity);
                }
                else if (keyEvent->code == sf::Keyboard::Key::L && !lKeyPressed) {
                    lKeyPressed = true;
                    activeVehicleManager->switchVehicle();
                }
                // Add key handler for cycling through planets with Tab
                else if (keyEvent->code == sf::Keyboard::Key::Tab && !tabKeyPressed && uiManager) {
                    tabKeyPressed = true;

                    // Find current selected planet in the planets vector
                    Planet* currentPlanet = nullptr;
                    int currentIndex = -1;
                    for (size_t i = 0; i < planets.size(); i++) {
                        // Need to implement a getter for selectedPlanet in UIManager
                        if (planets[i] == uiManager->getSelectedPlanet()) {
                            currentPlanet = planets[i];
                            currentIndex = static_cast<int>(i);
                            break;
                        }
                    }

                    // Move to the next planet in the list
                    if (currentIndex >= 0 && planets.size() > 1) {
                        int nextIndex = (currentIndex + 1) % planets.size();
                        uiManager->setSelectedPlanet(planets[nextIndex]);
                        std::cout << "Selected planet " << nextIndex << std::endl;
                    }
                    else if (!planets.empty()) {
                        // No planet selected or not found, select the first one
                        uiManager->setSelectedPlanet(planets[0]);
                        std::cout << "Selected planet 0" << std::endl;
                    }
                }
                // Add new key handler for dropping stored mass
                else if (keyEvent->code == sf::Keyboard::Key::Hyphen) {
                    if (activeVehicleManager &&
                        activeVehicleManager->getActiveVehicleType() == VehicleType::ROCKET &&
                        activeVehicleManager->getRocket()) {

                        Planet* newPlanet = activeVehicleManager->getRocket()->dropStoredMass();
                        if (newPlanet) {
                            // Add the new planet to the simulation
                            planets.push_back(newPlanet);
                            gravitySimulator.addPlanet(newPlanet);

                            // Debug output
                            std::cout << "Dropped mass and created new planet at position ("
                                << newPlanet->getPosition().x << ", "
                                << newPlanet->getPosition().y << ")" << std::endl;
                            std::cout << "Total planets in vector: " << planets.size() << std::endl;

                            // Check if uiManager is valid
                            if (uiManager) {
                                std::cout << "UI Manager found, setting selected planet" << std::endl;
                                uiManager->setSelectedPlanet(newPlanet);

                                // Verify selection
                                if (uiManager->getSelectedPlanet() == newPlanet) {
                                    std::cout << "Successfully selected new planet" << std::endl;
                                }
                                else {
                                    std::cout << "Failed to select new planet" << std::endl;
                                }
                            }
                            else {
                                std::cout << "UI Manager is null, cannot select planet" << std::endl;
                            }
                        }
                        else {
                            std::cout << "Failed to create new planet (not enough mass?)" << std::endl;
                        }
                    }
                }
            }
        }
        if (event->is<sf::Event::KeyReleased>())
        {
            const auto* keyEvent = event->getIf<sf::Event::KeyReleased>();
            if (keyEvent) {
                if (keyEvent->code == sf::Keyboard::Key::L) {
                    lKeyPressed = false;
                }
                else if (keyEvent->code == sf::Keyboard::Key::Tab) {
                    tabKeyPressed = false;
                }
            }
        }
    }

    // Handle thrust level setting
    float setThrustLevel = 1.0f;
    float deltaTime = std::min(clock.restart().asSeconds(), 0.1f);
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num1))
        activeVehicleManager->getRocket()->setThrustLevel(0.1f);
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num2))
        activeVehicleManager->getRocket()->setThrustLevel(0.2f);
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num3))
        activeVehicleManager->getRocket()->setThrustLevel(0.3f);
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num4))
        activeVehicleManager->getRocket()->setThrustLevel(0.4f);
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num5))
        activeVehicleManager->getRocket()->setThrustLevel(0.5f);
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num6))
        activeVehicleManager->getRocket()->setThrustLevel(0.6f);
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num7))
        activeVehicleManager->getRocket()->setThrustLevel(0.7f);
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num8))
        activeVehicleManager->getRocket()->setThrustLevel(0.8f);
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num9))
        activeVehicleManager->getRocket()->setThrustLevel(0.9f);
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num0))
        activeVehicleManager->getRocket()->setThrustLevel(0.0f);
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Equal))
        activeVehicleManager->getRocket()->setThrustLevel(1.0f);

    // Apply thrust and rotation
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
        activeVehicleManager->applyThrust(1.0f);
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
        activeVehicleManager->applyThrust(-0.5f);
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
        activeVehicleManager->rotate(-4.0f * deltaTime * 60.0f);
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
        activeVehicleManager->rotate(4.0f * deltaTime * 60.0f);

    // Camera control keys
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Z)) {
        // Gradually increase zoom to see more of the system
        targetZoom = std::min(1000.0f, targetZoom * 1.05f);
        // Focus on active vehicle
        gameView.setCenter(activeVehicleManager->getActiveVehicle()->getPosition());
    }
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::X)) {
        // Auto-zoom based on distance to closest planet
        float dist1 = std::sqrt(
            std::pow(activeVehicleManager->getActiveVehicle()->getPosition().x - planets[0]->getPosition().x, 2) +
            std::pow(activeVehicleManager->getActiveVehicle()->getPosition().y - planets[0]->getPosition().y, 2)
        );
        float dist2 = std::sqrt(
            std::pow(activeVehicleManager->getActiveVehicle()->getPosition().x - planets[1]->getPosition().x, 2) +
            std::pow(activeVehicleManager->getActiveVehicle()->getPosition().y - planets[1]->getPosition().y, 2)
        );
        targetZoom = 1.0f + (std::min(dist1, dist2) - (planets[0]->getRadius() + GameConstants::ROCKET_SIZE)) / 100.0f;
        gameView.setCenter(activeVehicleManager->getActiveVehicle()->getPosition());
    }
    //else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::C)) {
        // Follow planet 2
     //   targetZoom = 10.0f;
     //   gameView.setCenter(planets[1]->getPosition());
    //}
}

void GameManager::cleanup()
{
    delete activeVehicleManager;

    for (auto planet : planets) {
        delete planet;
    }
    planets.clear();
}

VehicleManager* GameManager::getActiveVehicleManager()
{
    return activeVehicleManager;
}

const std::vector<Planet*>& GameManager::getPlanets() const
{
    return planets;
}

sf::View& GameManager::getGameView()
{
    return gameView;
}

sf::View& GameManager::getUIView()
{
    return uiView;
}