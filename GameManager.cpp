// GameManager.cpp
#include "GameManager.h"
#include "TextPanel.h"
#include "OrbitalMechanics.h"
#include "GameConstants.h"
#include "UIManager.h"  // Add this include
#include <ctime>
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
    // Seed the random number generator
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    // Create main planet (sun)
    Planet* mainPlanet = new Planet(sf::Vector2f(GameConstants::MAIN_PLANET_X, GameConstants::MAIN_PLANET_Y),
        0, GameConstants::MAIN_PLANET_MASS, sf::Color::Yellow);
    mainPlanet->setVelocity(sf::Vector2f(1.f, -1.f));
    planets.push_back(mainPlanet);
    // Determine random number of planets (1-9)
    int planetCount = 1 + std::rand() % 9;
    // Planet colors
    const sf::Color planetColors[] = {
        sf::Color(150, 150, 150),   // Mercury (gray)
        sf::Color(255, 190, 120),   // Venus (light orange)
        sf::Color(0, 100, 255),     // Earth (blue)
        sf::Color(255, 100, 0),     // Mars (red)
        sf::Color(255, 200, 100),   // Jupiter (light orange)
        sf::Color(230, 180, 80),    // Saturn (tan)
        sf::Color(180, 230, 230),   // Uranus (light blue)
        sf::Color(100, 130, 255),   // Neptune (dark blue)
        sf::Color(230, 230, 230)    // Pluto (light gray)
    };
    // Distance scaling factors - each planet's orbit will be spaced to avoid collisions
    const float distanceScalings[] = { 0.4f, 0.7f, 1.0f, 1.5f, 2.2f, 3.0f, 4.0f, 5.0f, 6.0f };
    // Mass scaling factors - generally increasing with distance
    const float massScalings[] = { 0.1f, 0.2f, 0.3f, 0.5f, 0.8f, 1.2f, 1.8f, 2.5f, 3.5f };
    // Create each planet
    for (int i = 0; i < planetCount; i++) {
        float orbitDistance = GameConstants::PLANET_ORBIT_DISTANCE * distanceScalings[i];
        // Randomly position planet around the orbit
        float angle = (std::rand() % 360) * (3.14159f / 180.0f);
        // Calculate position based on orbit distance and angle
        float planetX = mainPlanet->getPosition().x + orbitDistance * cos(angle);
        float planetY = mainPlanet->getPosition().y + orbitDistance * sin(angle);
        // Calculate orbital velocity for a stable circular orbit
        float orbitalVelocity = std::sqrt(GameConstants::G * mainPlanet->getMass() / orbitDistance);
        // Velocity is perpendicular to position vector
        float velocityX = -sin(angle) * orbitalVelocity;
        float velocityY = cos(angle) * orbitalVelocity;
        // Create the planet with scaled mass
        float planetMass = GameConstants::SECONDARY_PLANET_MASS * massScalings[i];
        // Add some randomness to mass (�30%)
        float massRandomFactor = 0.7f + (std::rand() % 60) / 100.0f;
        planetMass *= massRandomFactor;
        Planet* planet = new Planet(
            sf::Vector2f(planetX, planetY),
            0, planetMass, planetColors[i]);
        planet->setVelocity(sf::Vector2f(velocityX, velocityY));
        planets.push_back(planet);
    }

    // Find the second largest planet
    Planet* secondLargestPlanet = nullptr;
    Planet* largestPlanet = mainPlanet; // Main planet is typically the largest
    float secondLargestMass = 0.0f;

    // Loop through all planets (except the main one) to find second largest
    for (size_t i = 1; i < planets.size(); i++) {
        float mass = planets[i]->getMass();
        if (mass > secondLargestMass) {
            secondLargestPlanet = planets[i];
            secondLargestMass = mass;
        }
    }

    // If no second planet was found or only one planet exists, use the main planet
    if (!secondLargestPlanet) {
        secondLargestPlanet = mainPlanet;
    }

    // Calculate position for rocket - start at the top of the second largest planet
    sf::Vector2f planetPos = secondLargestPlanet->getPosition();
    float planetRadius = secondLargestPlanet->getRadius();
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
    // Get vehicle position for camera centering
    sf::Vector2f vehiclePos = activeVehicleManager->getActiveVehicle()->getPosition();

    // Always update view center to follow vehicle
    gameView.setCenter(vehiclePos);

    const float minZoom = 1.0f;
    const float maxZoom = 1000.0f;
    const float zoomSpeed = 0.10f;

    // Smoothly interpolate current zoom to target zoom
    zoomLevel += (targetZoom - zoomLevel) * deltaTime * zoomSpeed;

    // Clamp zoom level to valid range
    zoomLevel = std::max(minZoom, std::min(zoomLevel, maxZoom));

    // Set view size based on zoom level
    gameView.setSize(sf::Vector2f(1280.f * zoomLevel, 720.f * zoomLevel));
}

void GameManager::render()
{
    // Apply the game view for world rendering
    window.setView(gameView);

    // Clear window
    window.clear(sf::Color::Black);

    // Get the selected planet from the UI manager
    Planet* selectedPlanet = nullptr;
    if (uiManager) {
        selectedPlanet = uiManager->getSelectedPlanet();
    }

    // Draw orbit path only for the selected planet
    if (selectedPlanet) {
        selectedPlanet->drawOrbitPath(window, planets);
    }

    // Replace with this null-checked version:
    if (activeVehicleManager &&
        activeVehicleManager->getActiveVehicleType() == VehicleType::ROCKET &&
        activeVehicleManager->getRocket()) {
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
                //else if (keyEvent->code == sf::Keyboard::Key::L && !lKeyPressed) {
                //    lKeyPressed = true;
                ///    activeVehicleManager->switchVehicle();
                //}
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
                if (keyEvent->code == sf::Keyboard::Key::Tab) {
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
        // Gradually increase zoom to see more of the system (zoom out)
        targetZoom = std::min(100.0f, targetZoom * 1.05f);
        // Focus on active vehicle
        gameView.setCenter(activeVehicleManager->getActiveVehicle()->getPosition());
    }
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::X)) {
        // Gradually decrease zoom to see less of the system (zoom in)
        targetZoom = std::max(1.0f, targetZoom / 1.05f);
        // Focus on active vehicle
        gameView.setCenter(activeVehicleManager->getActiveVehicle()->getPosition());
    }
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::C)) {
    
       targetZoom = 1.0f;
       gameView.setCenter(activeVehicleManager->getActiveVehicle()->getPosition());
    }
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