// VehicleManager.cpp
#include "VehicleManager.h"
#include "GameConstants.h"
#include "VectorHelper.h"
#include <iostream>

VehicleManager::VehicleManager(sf::Vector2f initialPos, const std::vector<Planet*>& planetList)
    : activeVehicle(VehicleType::ROCKET)
{
    try {
        // First initialize rockets and cars
        rocket = std::make_unique<Rocket>(initialPos, sf::Vector2f(0, 0));
        car = std::make_unique<Car>(initialPos, sf::Vector2f(0, 0));

        // Then handle planets - make a safe copy
        if (!planetList.empty()) {
            // Only add non-null planets
            for (auto* planet : planetList) {
                if (planet) {
                    planets.push_back(planet);
                }
            }

            // Only set planets if we have valid ones
            if (!planets.empty() && rocket) {
                rocket->setNearbyPlanets(planets);
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in VehicleManager constructor: " << e.what() << std::endl;

        // Make sure rocket and car are properly initialized
        if (!rocket) rocket = std::make_unique<Rocket>(initialPos, sf::Vector2f(0, 0));
        if (!car) car = std::make_unique<Car>(initialPos, sf::Vector2f(0, 0));

        // Clear any potentially problematic planets
        planets.clear();
    }
}

void VehicleManager::switchVehicle() {
    if (!rocket || !car) {
        std::cerr << "Error: Rocket or Car not initialized in switchVehicle" << std::endl;
        return;
    }

    if (planets.empty()) {
        return; // Can't switch if there are no planets
    }

    if (activeVehicle == VehicleType::ROCKET) {
        // Check if rocket is close to a planet surface
        bool canTransform = false;
        for (const auto& planet : planets) {
            if (!planet) continue; // Skip null planets

            float dist = distance(rocket->getPosition(), planet->getPosition());
            if (dist <= planet->getRadius() + GameConstants::TRANSFORM_DISTANCE) {
                canTransform = true;
                break;
            }
        }

        if (canTransform) {
            // Transfer rocket state to car
            car->initializeFromRocket(rocket.get());
            car->checkGrounding(planets);
            activeVehicle = VehicleType::CAR;
        }
    }
    else {
        // Only allow switching back to rocket if car is on ground
        if (car->isOnGround()) {
            // Transfer car state to rocket
            rocket->setPosition(car->getPosition());
            rocket->setVelocity(sf::Vector2f(0, 0)); // Start with zero velocity
            activeVehicle = VehicleType::ROCKET;
        }
    }
}

void VehicleManager::update(float deltaTime) {
    // Don't proceed if the planets vector is empty
    if (planets.empty()) {
        // Still update objects but don't set planets
        if (activeVehicle == VehicleType::ROCKET) {
            if (rocket) {
                rocket->update(deltaTime);
            }
        }
        else {
            if (car) {
                car->update(deltaTime);
            }
        }
        return;
    }

    // Create a safe copy of valid planets
    std::vector<Planet*> validPlanets;
    try {
        for (auto* planet : planets) {
            if (planet) {
                validPlanets.push_back(planet);
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Exception creating validPlanets: " << e.what() << std::endl;
        validPlanets.clear();
    }

    if (validPlanets.empty()) {
        // Still update objects without planets
        if (activeVehicle == VehicleType::ROCKET) {
            if (rocket) {
                rocket->update(deltaTime);
            }
        }
        else {
            if (car) {
                car->update(deltaTime);
            }
        }
        return;
    }

    // Now update with valid planets
    if (activeVehicle == VehicleType::ROCKET) {
        if (rocket) {
            try {
                rocket->setNearbyPlanets(validPlanets);
                rocket->update(deltaTime);
            }
            catch (const std::exception& e) {
                std::cerr << "Exception in rocket update: " << e.what() << std::endl;
            }
        }
    }
    else {
        if (car) {
            try {
                car->checkGrounding(validPlanets);
                car->update(deltaTime);
            }
            catch (const std::exception& e) {
                std::cerr << "Exception in car update: " << e.what() << std::endl;
            }
        }
    }
}

void VehicleManager::draw(sf::RenderWindow& window) {
    if (!window.isOpen()) return;

    if (activeVehicle == VehicleType::ROCKET) {
        if (rocket) {
            rocket->draw(window);
        }
    }
    else {
        if (car) {
            car->draw(window);
        }
    }
}

void VehicleManager::drawWithConstantSize(sf::RenderWindow& window, float zoomLevel) {
    if (!window.isOpen()) return;

    if (activeVehicle == VehicleType::ROCKET) {
        if (rocket) {
            rocket->drawWithConstantSize(window, zoomLevel);
        }
    }
    else {
        if (car) {
            car->drawWithConstantSize(window, zoomLevel);
        }
    }
}

void VehicleManager::applyThrust(float amount) {
    if (activeVehicle == VehicleType::ROCKET) {
        if (rocket) {
            rocket->applyThrust(amount);
        }
    }
    else {
        if (car) {
            car->accelerate(amount);
        }
    }
}

void VehicleManager::rotate(float amount) {
    if (activeVehicle == VehicleType::ROCKET) {
        if (rocket) {
            rocket->rotate(amount);
        }
    }
    else {
        if (car) {
            car->rotate(amount);
        }
    }
}

void VehicleManager::drawVelocityVector(sf::RenderWindow& window, float scale) {
    if (!window.isOpen()) return;

    if (activeVehicle == VehicleType::ROCKET) {
        if (rocket) {
            rocket->drawVelocityVector(window, scale);
        }
    }
    // Car doesn't have a velocity vector display
}

GameObject* VehicleManager::getActiveVehicle() {
    if (activeVehicle == VehicleType::ROCKET) {
        return rocket ? rocket.get() : nullptr;
    }
    else {
        return car ? car.get() : nullptr;
    }
}

void VehicleManager::updatePlanets(const std::vector<Planet*>& newPlanets) {
    // Update the internal planets vector with the new set of planets
    planets.clear();
    for (auto* planet : newPlanets) {
        if (planet) {
            planets.push_back(planet);
        }
    }

    // Update the planet references in rocket
    if (rocket) {
        rocket->setNearbyPlanets(planets);
    }

    // Update in car if needed
    if (car) {
        car->checkGrounding(planets);
    }
}