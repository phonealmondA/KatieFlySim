// GameClient.cpp (complete improved file)
#include "GameClient.h"
#include "GameConstants.h"
#include "VectorHelper.h"
#include <iostream> // For std::cout

GameClient::GameClient()
    : localPlayer(nullptr),
    localPlayerId(0),
    stateTimestamp(0.0f),
    latencyCompensation(0.05f) {
}

GameClient::~GameClient() {
    // Clean up players
    for (auto& pair : remotePlayers) {
        delete pair.second;
    }
    remotePlayers.clear();

    delete localPlayer;

    // Clean up planets
    for (auto& planet : planets) {
        delete planet;
    }
    planets.clear();
}

void GameClient::initialize() {
    // Create main planet (placeholder until we get state from server)
    Planet* mainPlanet = new Planet(
        sf::Vector2f(GameConstants::MAIN_PLANET_X, GameConstants::MAIN_PLANET_Y),
        0, GameConstants::MAIN_PLANET_MASS, sf::Color::Blue);
    mainPlanet->setVelocity(sf::Vector2f(0.f, 0.f));
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
    localPlayer = new VehicleManager(initialPos, planets);

    // Setup gravity simulator
    simulator.setSimulatePlanetGravity(true);
    for (auto planet : planets) {
        simulator.addPlanet(planet);
    }
    simulator.addVehicleManager(localPlayer);
}

void GameClient::update(float deltaTime) {
    // Update simulator
    simulator.update(deltaTime);

    // Update planets
    for (auto planet : planets) {
        planet->update(deltaTime);
    }

    // Update local player
    if (localPlayer) {
        localPlayer->update(deltaTime);
    }

    // Update remote players
    for (auto& pair : remotePlayers) {
        // For remote players, use interpolation based on received state
        // and dead reckoning for prediction
        pair.second->update(deltaTime);
    }
}

void GameClient::processGameState(const GameState& state) {
    // Update last state
    lastState = state;
    stateTimestamp = state.timestamp;

    // Process planets
    for (const auto& planetState : state.planets) {
        // Make sure we have enough planets
        while (planetState.planetId >= static_cast<int>(planets.size())) {
            planets.push_back(new Planet(sf::Vector2f(0, 0), 0, 1.0f));
            simulator.addPlanet(planets.back());
        }

        // Update planet state
        Planet* planet = planets[planetState.planetId];
        planet->setPosition(planetState.position);
        planet->setVelocity(planetState.velocity);
        planet->setMass(planetState.mass);
        // Note: setMass will update radius based on the mass-radius relationship
    }

    // Process rockets
    for (const auto& rocketState : state.rockets) {
        if (rocketState.playerId == localPlayerId) {
            // This is our local player, update position for prediction correction
            if (localPlayer) {
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
            // Get or create vehicle manager for this player
            auto it = remotePlayers.find(rocketState.playerId);
            VehicleManager* manager = nullptr;

            if (it == remotePlayers.end()) {
                // Create a new remote player
                manager = new VehicleManager(rocketState.position, planets);
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
                manager = it->second;
            }

            // Update rocket state with interpolation
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
        delete remotePlayers[playerId];
        remotePlayers.erase(playerId);
        remotePlayerStates.erase(playerId);
    }
}

void GameClient::setLatencyCompensation(float value) {
    latencyCompensation = value;
}

PlayerInput GameClient::getLocalPlayerInput(float deltaTime) const {
    PlayerInput input;
    input.playerId = localPlayerId;
    input.deltaTime = deltaTime;

    // Get keyboard state
    input.thrustForward = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W);
    input.thrustBackward = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S);
    input.rotateLeft = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A);
    input.rotateRight = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D);
    input.switchVehicle = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::L);

    // Get thrust level
    if (localPlayer && localPlayer->getActiveVehicleType() == VehicleType::ROCKET) {
        input.thrustLevel = localPlayer->getRocket()->getThrustLevel();
    }

    return input;
}

void GameClient::applyLocalInput(const PlayerInput& input) {
    if (!localPlayer) return;

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
    if (localPlayer->getActiveVehicleType() == VehicleType::ROCKET) {
        localPlayer->getRocket()->setThrustLevel(input.thrustLevel);
    }
}

void GameClient::interpolateRemotePlayers(float currentTime) {
    for (auto& [playerId, stateData] : remotePlayerStates) {
        auto it = remotePlayers.find(playerId);
        if (it == remotePlayers.end()) continue;

        VehicleManager* manager = it->second;
        Rocket* rocket = manager->getRocket();

        // Calculate interpolation factor
        float timeElapsed = currentTime - stateData.timestamp;
        float alpha = std::min(timeElapsed / latencyCompensation, 1.0f);

        // Interpolate position and velocity
        sf::Vector2f interpolatedPos = stateData.startPos + (stateData.targetPos - stateData.startPos) * alpha;
        sf::Vector2f interpolatedVel = stateData.startVel + (stateData.targetVel - stateData.startVel) * alpha;

        rocket->setPosition(interpolatedPos);
        rocket->setVelocity(interpolatedVel);
    }
}