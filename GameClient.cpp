// GameClient.cpp
#include "GameClient.h"
#include "GameConstants.h"
#include "VectorHelper.h"
#include <iostream>

GameClient::GameClient()
    : localPlayer(nullptr),
    localPlayerId(0),
    stateTimestamp(0.0f),
    latencyCompensation(0.05f),
    connectionState(ClientConnectionState::DISCONNECTED),
    hasReceivedInitialState(false) {
}

GameClient::~GameClient() {
    // Clean up players
    for (auto& pair : remotePlayers) {
        delete pair.second;
    }
    remotePlayers.clear();

    delete localPlayer;
    localPlayer = nullptr;

    // Clean up planets
    for (auto& planet : planets) {
        delete planet;
    }
    planets.clear();
}

void GameClient::initialize() {
    try {
        // Flag that we're still waiting for initial state
        hasReceivedInitialState = false;
        connectionState = ClientConnectionState::CONNECTING;

        // Create main planet (placeholder until we get state from server)
        Planet* mainPlanet = new Planet(
            sf::Vector2f(GameConstants::MAIN_PLANET_X, GameConstants::MAIN_PLANET_Y),
            0, GameConstants::MAIN_PLANET_MASS, sf::Color::Blue);
        mainPlanet->setVelocity(sf::Vector2f(1.f, -1.f));
        planets.push_back(mainPlanet);

        // Create secondary planet
        Planet* secondaryPlanet = new Planet(
            sf::Vector2f(GameConstants::SECONDARY_PLANET_X, GameConstants::SECONDARY_PLANET_Y),
            0, GameConstants::SECONDARY_PLANET_MASS, sf::Color::Green);
        secondaryPlanet->setVelocity(sf::Vector2f(0.f, GameConstants::SECONDARY_PLANET_ORBITAL_VELOCITY));
        planets.push_back(secondaryPlanet);

        // Setup local player (placeholder until we get assigned ID)
        sf::Vector2f initialPos = planets[0]->getPosition() +
            sf::Vector2f(0, -(planets[0]->getRadius() + GameConstants::ROCKET_SIZE));

        // Always check null pointers
        if (localPlayer) {
            delete localPlayer;
            localPlayer = nullptr;
        }

        // Create the local player with error checking
        try {
            localPlayer = new VehicleManager(initialPos, planets);
            if (!localPlayer || !localPlayer->getRocket()) {
                std::cerr << "Failed to create valid VehicleManager for local player" << std::endl;
                delete localPlayer;
                localPlayer = nullptr;
                throw std::runtime_error("Local player creation failed");
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Exception creating local player: " << e.what() << std::endl;
            if (localPlayer) {
                delete localPlayer;
                localPlayer = nullptr;
            }
            throw; // Re-throw to inform caller
        }

        // Setup gravity simulator
        simulator.setSimulatePlanetGravity(true);
        for (auto planet : planets) {
            if (planet) {
                simulator.addPlanet(planet);
            }
        }

        if (localPlayer) {
            simulator.addVehicleManager(localPlayer);
        }

        std::cout << "GameClient initialized successfully" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in GameClient::initialize: " << e.what() << std::endl;
        // Clean up any resources that might have been partially initialized
        for (auto& planet : planets) {
            delete planet;
        }
        planets.clear();

        delete localPlayer;
        localPlayer = nullptr;

        // Re-throw to inform caller
        throw;
    }
}

void GameClient::update(float deltaTime) {
    try {
        // Skip updates if not fully connected
        if (connectionState != ClientConnectionState::CONNECTED && !hasReceivedInitialState) {
            return;
        }

        // Update simulator with null checking
        simulator.update(deltaTime);

        // Update planets with null checking
        for (auto planet : planets) {
            if (planet) {
                planet->update(deltaTime);
            }
        }

        // Update local player with null checking
        if (localPlayer) {
            localPlayer->update(deltaTime);
        }

        // Update remote players with null checking
        for (auto& pair : remotePlayers) {
            if (pair.second) {
                pair.second->update(deltaTime);
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in GameClient::update: " << e.what() << std::endl;
        // Continue execution - don't re-throw from update
    }
}

void GameClient::processGameState(const GameState& state) {
    try {
        // Don't process empty states
        if (state.planets.empty()) {
            std::cerr << "Received empty game state" << std::endl;
            return;
        }

        // Update last state
        lastState = state;
        stateTimestamp = state.timestamp;

        // Update connection state if this is our first state
        if (!hasReceivedInitialState) {
            connectionState = ClientConnectionState::CONNECTED;
            hasReceivedInitialState = true;
            std::cout << "Received initial game state" << std::endl;
        }

        // Process planets - ensure we have the right number of planets
        for (const auto& planetState : state.planets) {
            if (planetState.planetId < 0) {
                std::cerr << "Invalid planet ID: " << planetState.planetId << std::endl;
                continue;
            }

            // Make sure we have enough planets
            while (planetState.planetId >= static_cast<int>(planets.size())) {
                try {
                    Planet* newPlanet = new Planet(sf::Vector2f(0, 0), 0, 1.0f);
                    planets.push_back(newPlanet);
                    simulator.addPlanet(newPlanet);
                }
                catch (const std::exception& e) {
                    std::cerr << "Exception creating new planet: " << e.what() << std::endl;
                    break;
                }
            }

            // Update planet state
            if (planetState.planetId < static_cast<int>(planets.size())) {
                Planet* planet = planets[planetState.planetId];
                if (planet) {
                    planet->setPosition(planetState.position);
                    planet->setVelocity(planetState.velocity);
                    planet->setMass(planetState.mass);
                }
            }
        }

        // Process rockets
        for (const auto& rocketState : state.rockets) {
            // First ensure we have a valid local player initialized
            if (!localPlayer && !planets.empty()) {
                try {
                    sf::Vector2f pos = planets[0]->getPosition() +
                        sf::Vector2f(0, -(planets[0]->getRadius() + GameConstants::ROCKET_SIZE));
                    localPlayer = new VehicleManager(pos, planets);
                    simulator.addVehicleManager(localPlayer);
                    std::cout << "Created local player (was null)" << std::endl;
                }
                catch (const std::exception& e) {
                    std::cerr << "Exception creating local player: " << e.what() << std::endl;
                    continue;
                }
            }

            if (rocketState.playerId == localPlayerId) {
                // This is our local player
                if (!localPlayer) {
                    // Create local player if it doesn't exist
                    try {
                        localPlayer = new VehicleManager(rocketState.position, planets);
                        simulator.addVehicleManager(localPlayer);
                        std::cout << "Created local player with ID: " << localPlayerId << std::endl;
                    }
                    catch (const std::exception& e) {
                        std::cerr << "Exception creating local player: " << e.what() << std::endl;
                        continue;
                    }
                }

                if (localPlayer && localPlayer->getRocket()) {
                    // Calculate position difference
                    sf::Vector2f posDiff = rocketState.position - localPlayer->getRocket()->getPosition();
                    float distance = std::sqrt(posDiff.x * posDiff.x + posDiff.y * posDiff.y);

                    // Improved client-side prediction with smoothing
                    if (distance > 20.0f) {
                        // Hard correction for big differences
                        localPlayer->getRocket()->setPosition(rocketState.position);
                        localPlayer->getRocket()->setVelocity(rocketState.velocity);
                    }
                    else if (distance > 5.0f) {
                        // Smooth interpolation for small differences
                        sf::Vector2f correctionVector = rocketState.position - localPlayer->getRocket()->getPosition();
                        localPlayer->getRocket()->setPosition(
                            localPlayer->getRocket()->getPosition() + correctionVector * 0.2f);

                        // Also smoothly adjust velocity
                        sf::Vector2f velCorrection = rocketState.velocity - localPlayer->getRocket()->getVelocity();
                        localPlayer->getRocket()->setVelocity(
                            localPlayer->getRocket()->getVelocity() + velCorrection * 0.2f);
                    }

                    // Keep local rotation control for better responsiveness
                    // Only update server rotation if significantly different
                    float rotDiff = std::abs(rocketState.rotation - localPlayer->getRocket()->getRotation());
                    if (rotDiff > 45.0f) {
                        localPlayer->getRocket()->setRotation(rocketState.rotation);
                    }
                }
            }
            else {
                // This is a remote player
                VehicleManager* manager = nullptr;
                auto it = remotePlayers.find(rocketState.playerId);

                if (it == remotePlayers.end()) {
                    // Create a new remote player
                    try {
                        manager = new VehicleManager(rocketState.position, planets);
                        if (manager && manager->getRocket()) {
                            remotePlayers[rocketState.playerId] = manager;
                            simulator.addVehicleManager(manager);

                            // Set color based on player ID
                            manager->getRocket()->setColor(sf::Color(
                                100 + (rocketState.playerId * 50) % 155,
                                100 + (rocketState.playerId * 30) % 155,
                                100 + (rocketState.playerId * 70) % 155
                            ));

                            std::cout << "Added remote player with ID: " << rocketState.playerId << std::endl;
                        }
                        else {
                            std::cerr << "Failed to create valid VehicleManager for remote player" << std::endl;
                            delete manager; // Clean up if rocket initialization failed
                        }
                    }
                    catch (const std::exception& e) {
                        std::cerr << "Exception creating remote player: " << e.what() << std::endl;
                        delete manager; // Clean up on exception
                        continue;
                    }
                }
                else {
                    manager = it->second;
                }

                // Update rocket state with interpolation if manager exists
                if (manager && manager->getRocket()) {
                    Rocket* rocket = manager->getRocket();

                    // Store previous position for interpolation
                    sf::Vector2f prevPos = rocket->getPosition();
                    sf::Vector2f prevVel = rocket->getVelocity();

                    // Update with server values
                    rocket->setPosition(rocketState.position);
                    rocket->setVelocity(rocketState.velocity);
                    rocket->setRotation(rocketState.rotation);
                    rocket->setThrustLevel(rocketState.thrustLevel);

                    // Store for interpolation
                    remotePlayerStates[rocketState.playerId] = {
                        prevPos, prevVel,
                        rocketState.position, rocketState.velocity,
                        rocketState.rotation,
                        state.timestamp
                    };
                }
            }
        }

        // Remove any players that weren't in the update
        std::vector<int> playersToRemove;
        for (const auto& pair : remotePlayers) {
            bool found = false;
            for (const auto& rocketState : state.rockets) {
                if (rocketState.playerId == pair.first) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                playersToRemove.push_back(pair.first);
            }
        }

        for (int playerId : playersToRemove) {
            std::cout << "Remote player " << playerId << " disconnected" << std::endl;
            if (remotePlayers[playerId]) {
                simulator.removeVehicleManager(remotePlayers[playerId]);
                delete remotePlayers[playerId];
            }
            remotePlayers.erase(playerId);
            remotePlayerStates.erase(playerId);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in processGameState: " << e.what() << std::endl;
    }
}

void GameClient::setLatencyCompensation(float value) {
    latencyCompensation = value;
}

//void GameClient::setLocalPlayerId(int id) {
//    localPlayerId = id;
//    connectionState = ClientConnectionState::WAITING_FOR_STATE;
//    std::cout << "Local player ID set to: " << id << std::endl;
//}

PlayerInput GameClient::getLocalPlayerInput(float deltaTime) const {
    PlayerInput input;
    input.playerId = localPlayerId;
    input.deltaTime = deltaTime;

    // Skip input collection if not fully connected
    if (connectionState != ClientConnectionState::CONNECTED || !hasReceivedInitialState || !localPlayer) {
        return input;
    }

    // Get keyboard state
    input.thrustForward = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W);
    input.thrustBackward = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S);
    input.rotateLeft = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A);
    input.rotateRight = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D);

    // Get thrust level
    if (localPlayer && localPlayer->getActiveVehicleType() == VehicleType::ROCKET && localPlayer->getRocket()) {
        input.thrustLevel = localPlayer->getRocket()->getThrustLevel();
    }

    return input;
}

void GameClient::applyLocalInput(const PlayerInput& input) {
    // Skip if not fully connected or no local player
    if (!hasReceivedInitialState || connectionState != ClientConnectionState::CONNECTED || !localPlayer) {
        return;
    }

    try {
        // Apply input to local player immediately for responsive feel
        if (input.thrustForward) {
            localPlayer->applyThrust(1.0f);
        }
        if (input.thrustBackward) {
            localPlayer->applyThrust(-0.5f);
        }
        if (input.rotateLeft) {
            localPlayer->rotate(-6.0f * input.deltaTime * 60.0f);
        }
        if (input.rotateRight) {
            localPlayer->rotate(6.0f * input.deltaTime * 60.0f);
        }
        if (input.switchVehicle) {
            localPlayer->switchVehicle();
        }

        // Apply thrust level
        if (localPlayer->getActiveVehicleType() == VehicleType::ROCKET && localPlayer->getRocket()) {
            localPlayer->getRocket()->setThrustLevel(input.thrustLevel);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in applyLocalInput: " << e.what() << std::endl;
    }
}

void GameClient::interpolateRemotePlayers(float currentTime) {
    // Skip if not fully connected
    if (!hasReceivedInitialState || connectionState != ClientConnectionState::CONNECTED) {
        return;
    }

    try {
        for (auto it = remotePlayerStates.begin(); it != remotePlayerStates.end(); ) {
            int playerId = it->first;
            RemotePlayerState& stateData = it->second;

            auto playerIt = remotePlayers.find(playerId);
            if (playerIt == remotePlayers.end() || !playerIt->second || !playerIt->second->getRocket()) {
                // Remove stale state if player no longer exists
                it = remotePlayerStates.erase(it);
                continue;
            }

            VehicleManager* manager = playerIt->second;
            Rocket* rocket = manager->getRocket();
            if (!rocket) {
                ++it;
                continue;
            }

            // Calculate interpolation factor
            float timeElapsed = currentTime - stateData.timestamp;
            float alpha = std::min(timeElapsed / latencyCompensation, 1.0f);

            // Interpolate position and velocity
            sf::Vector2f interpolatedPos = stateData.startPos + (stateData.targetPos - stateData.startPos) * alpha;
            sf::Vector2f interpolatedVel = stateData.startVel + (stateData.targetVel - stateData.startVel) * alpha;

            try {
                rocket->setPosition(interpolatedPos);
                rocket->setVelocity(interpolatedVel);
            }
            catch (const std::exception& e) {
                std::cerr << "Exception in interpolateRemotePlayers: " << e.what() << std::endl;
            }

            ++it;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in interpolateRemotePlayers: " << e.what() << std::endl;
    }
}