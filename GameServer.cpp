// GameServer.cpp
#include "GameServer.h"
#include "GameConstants.h"
#include <iostream> // Add this line to use std::cout

GameServer::GameServer() : sequenceNumber(0), gameTime(0.0f) {
}

GameServer::~GameServer() {
    // Clean up players
    for (auto& pair : players) {
        delete pair.second;
    }
    players.clear();

    // Clean up planets
    for (auto& planet : planets) {
        delete planet;
    }
    planets.clear();
}
void GameServer::initialize() {
    // Create main planet (sun)
    Planet* mainPlanet = new Planet(
        sf::Vector2f(GameConstants::MAIN_PLANET_X, GameConstants::MAIN_PLANET_Y),
        0, GameConstants::MAIN_PLANET_MASS, sf::Color::Yellow);
    mainPlanet->setVelocity(sf::Vector2f(1.f, -1.f));
    planets.push_back(mainPlanet);

    // Create 9 orbiting planets with different orbital distances, sizes, and colors
    const float basePlanetMass = GameConstants::SECONDARY_PLANET_MASS;
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

    // Distance and mass scaling factors
    const float distanceScalings[] = { 0.4f, 0.7f, 1.0f, 1.5f, 2.2f, 3.0f, 4.0f, 5.0f, 6.0f };
    const float massScalings[] = { 0.1f, 0.8f, 1.0f, 0.5f, 11.0f, 9.5f, 4.0f, 3.8f, 0.05f };

    // Create each planet
    for (int i = 0; i < 9; i++) {
        float orbitDistance = GameConstants::PLANET_ORBIT_DISTANCE * distanceScalings[i];
        float angle = (i * 40.0f) * (3.14159f / 180.0f); // Distribute planets around the sun

        // Calculate position based on orbit distance and angle
        float planetX = mainPlanet->getPosition().x + orbitDistance * cos(angle);
        float planetY = mainPlanet->getPosition().y + orbitDistance * sin(angle);

        // Calculate orbital velocity for a circular orbit
        float orbitalVelocity = std::sqrt(GameConstants::G * mainPlanet->getMass() / orbitDistance);

        // Velocity is perpendicular to position vector
        float velocityX = -sin(angle) * orbitalVelocity;
        float velocityY = cos(angle) * orbitalVelocity;

        // Create the planet with scaled mass
        Planet* planet = new Planet(
            sf::Vector2f(planetX, planetY),
            0, basePlanetMass * massScalings[i], planetColors[i]);

        planet->setVelocity(sf::Vector2f(velocityX, velocityY));
        planets.push_back(planet);
    }

    // Setup gravity simulator
    simulator.setSimulatePlanetGravity(true);
    for (auto planet : planets) {
        simulator.addPlanet(planet);
    }

    // Create a default host player (ID 0)
    sf::Vector2f initialPos = mainPlanet->getPosition() +
        sf::Vector2f(0, -(mainPlanet->getRadius() + GameConstants::ROCKET_SIZE));
    addPlayer(0, initialPos, sf::Color::White);
}

int GameServer::addPlayer(int playerId, sf::Vector2f initialPos, sf::Color color) {
    // Check if player already exists
    if (players.find(playerId) != players.end()) {
        return playerId; // Player already exists
    }

    try {
        // Create a new vehicle manager for this player
        VehicleManager* manager = new VehicleManager(initialPos, planets);

        // Make sure rocket was initialized properly
        if (manager && manager->getRocket()) {
            manager->getRocket()->setColor(color);

            // Add to simulator
            simulator.addVehicleManager(manager);

            // Store in players map
            players[playerId] = manager;

            std::cout << "Added player with ID: " << playerId << std::endl;
        }
        else {
            std::cerr << "Failed to initialize rocket for player ID: " << playerId << std::endl;
            delete manager; // Clean up if rocket initialization failed
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Exception when adding player ID " << playerId << ": " << e.what() << std::endl;
    }

    return playerId;
}

void GameServer::removePlayer(int playerId) {
    auto it = players.find(playerId);
    if (it != players.end()) {
        simulator.removeVehicleManager(it->second);
        delete it->second;
        players.erase(it);
    }
}

void GameServer::update(float deltaTime) {
    // Update game time
    gameTime += deltaTime;

    // Update simulator
    simulator.update(deltaTime);

    // Update planets
    for (auto planet : planets) {
        planet->update(deltaTime);
    }

    // Update all players
    for (auto& pair : players) {
        if (pair.second) { // Add null check before updating
            pair.second->update(deltaTime);
        }
    }

    // Increment sequence number
    sequenceNumber++;
}

void GameServer::handlePlayerInput(int playerId, const PlayerInput& input) {
    auto it = players.find(playerId);
    if (it == players.end()) {
        // Player not found - could be a new connection, create player
        std::cout << "Unknown player ID: " << playerId << ", creating new player" << std::endl;
        sf::Vector2f spawnPos = planets[0]->getPosition() +
            sf::Vector2f(0, -(planets[0]->getRadius() + GameConstants::ROCKET_SIZE));

        // Add the player with error handling
        VehicleManager* manager = new VehicleManager(spawnPos, planets);
        if (manager && manager->getRocket()) {
            players[playerId] = manager;
            simulator.addVehicleManager(manager);
        }
        else {
            std::cerr << "Failed to create player for ID: " << playerId << std::endl;
            delete manager;
        }
        return;
    }

    // Debug output
    std::cout << "Server applying input to player ID: " << playerId << std::endl;

    // Apply input to the correct player's vehicle manager
    VehicleManager* manager = it->second;
    if (!manager) return; // Add null check

    // Apply input to the vehicle
    if (input.thrustForward) {
        manager->applyThrust(1.0f);
        std::cout << "Applied thrust forward to player " << playerId << std::endl;
    }
    if (input.thrustBackward) {
        manager->applyThrust(-0.5f);
        std::cout << "Applied thrust backward to player " << playerId << std::endl;
    }
    if (input.rotateLeft) {
        manager->rotate(-6.0f * input.deltaTime * 60.0f);
        std::cout << "Applied rotate left to player " << playerId << std::endl;
    }
    if (input.rotateRight) {
        manager->rotate(6.0f * input.deltaTime * 60.0f);
        std::cout << "Applied rotate right to player " << playerId << std::endl;
    }
    if (input.switchVehicle) {
        manager->switchVehicle();
        std::cout << "Applied vehicle switch to player " << playerId << std::endl;
    }

    // Apply thrust level with null checking
    if (manager->getActiveVehicleType() == VehicleType::ROCKET && manager->getRocket()) {
        manager->getRocket()->setThrustLevel(input.thrustLevel);
    }
}


GameState GameServer::getGameState() const {
    GameState state;
    state.sequenceNumber = sequenceNumber;
    state.timestamp = gameTime;

    try {
        // Add all rockets
        for (const auto& pair : players) {
            int playerId = pair.first;
            const VehicleManager* manager = pair.second;

            // Add null checks
            if (!manager) continue;

            // Only add if it's a rocket
            if (manager->getActiveVehicleType() == VehicleType::ROCKET) {
                const Rocket* rocket = manager->getRocket();

                // Add null check for rocket
                if (!rocket) continue;

                RocketState rocketState;
                rocketState.playerId = playerId;
                rocketState.position = rocket->getPosition();
                rocketState.velocity = rocket->getVelocity();
                rocketState.rotation = rocket->getRotation();
                rocketState.angularVelocity = 0.0f; // Not tracked in your current design
                rocketState.thrustLevel = rocket->getThrustLevel();
                rocketState.mass = rocket->getMass();
                rocketState.color = rocket->getColor();

                state.rockets.push_back(rocketState);
            }
            // TODO: Add car state if needed
        }

        // Add all planets
        for (size_t i = 0; i < planets.size(); ++i) {
            const Planet* planet = planets[i];

            // Skip null planets
            if (!planet) continue;

            PlanetState planetState;
            planetState.planetId = static_cast<int>(i);
            planetState.position = planet->getPosition();
            planetState.velocity = planet->getVelocity();
            planetState.mass = planet->getMass();
            planetState.radius = planet->getRadius();
            planetState.color = planet->getColor();

            state.planets.push_back(planetState);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in getGameState: " << e.what() << std::endl;
        // Return a minimal valid state to avoid crashes
    }

    return state;
}
