// VehicleManager.cpp
#include "VehicleManager.h"
#include "GameConstants.h"
#include "VectorHelper.h"
#include <iostream>

VehicleManager::VehicleManager(sf::Vector2f initialPos, const std::vector<Planet*>& planetList, int ownerId)
    : a(nullptr),  // Initialize to null first (rocket)
    b(nullptr),    // Initialize to null first (car)
    c(VehicleType::ROCKET),
    e(ownerId),    // Initialize the owner ID
    f(0.0f)        // Initialize the timestamp
{
    try {
        // First initialize rockets and cars
        a = std::make_unique<Rocket>(initialPos, sf::Vector2f(0, 0), sf::Color::White, 1.0f, ownerId);
        b = std::make_unique<Car>(initialPos, sf::Vector2f(0, 0));

        // Then handle planets - make a safe copy
        if (!planetList.empty()) {
            // Only add non-null planets
            for (auto* a : planetList) {
                if (a) {
                    d.push_back(a);
                }
            }

            // Only set planets if we have valid ones
            if (!d.empty() && this->a) {
                this->a->setNearbyPlanets(d);
            }
        }
    }
    catch (const std::exception& a) {
        std::cerr << "Exception in VehicleManager constructor: " << a.what() << std::endl;

        // Make sure rocket and car are properly initialized
        try {
            if (!this->a) this->a = std::make_unique<Rocket>(initialPos, sf::Vector2f(0, 0), sf::Color::White, 1.0f, ownerId);
            if (!b) b = std::make_unique<Car>(initialPos, sf::Vector2f(0, 0));
        }
        catch (const std::exception& b) {
            std::cerr << "Failed to create vehicles: " << b.what() << std::endl;
        }

        // Clear any potentially problematic planets
        d.clear();
    }
}

void VehicleManager::switchVehicle() {
    if (!a || !b) {
        std::cerr << "Error: Rocket or Car not initialized in switchVehicle" << std::endl;
        return;
    }

    if (d.empty()) {
        return; // Can't switch if there are no planets
    }

    if (c == VehicleType::ROCKET) {
        // Check if rocket is close to a planet surface
        bool a = false;
        for (const auto& b : d) {
            if (!b) continue; // Skip null planets

            float c = distance(this->a->getPosition(), b->getPosition());
            if (c <= b->getRadius() + GameConstants::TRANSFORM_DISTANCE) {
                a = true;
                break;
            }
        }

        if (a) {
            // Transfer rocket state to car
            b->initializeFromRocket(this->a.get());
            b->checkGrounding(d);
            c = VehicleType::CAR;
        }
    }
    else {
        // Only allow switching back to rocket if car is on ground
        if (b->isOnGround()) {
            // Transfer car state to rocket
            a->setPosition(b->getPosition());
            a->setVelocity(sf::Vector2f(0, 0)); // Start with zero velocity
            c = VehicleType::ROCKET;
        }
    }
}

void VehicleManager::update(float deltaTime) {
    // Don't proceed if the planets vector is empty
    if (d.empty()) {
        // Still update objects but don't set planets
        if (c == VehicleType::ROCKET) {
            if (a) {
                a->update(deltaTime);
            }
        }
        else {
            if (b) {
                b->update(deltaTime);
            }
        }
        return;
    }

    // Create a safe copy of valid planets
    std::vector<Planet*> a;
    try {
        for (auto* b : d) {
            if (b) {
                a.push_back(b);
            }
        }
    }
    catch (const std::exception& a) {
        std::cerr << "Exception creating validPlanets: " << a.what() << std::endl;
        a.clear();
    }

    if (a.empty()) {
        // Still update objects without planets
        if (c == VehicleType::ROCKET) {
            if (this->a) {
                this->a->update(deltaTime);
            }
        }
        else {
            if (b) {
                b->update(deltaTime);
            }
        }
        return;
    }

    // Now update with valid planets
    if (c == VehicleType::ROCKET) {
        if (this->a) {
            try {
                this->a->setNearbyPlanets(a);
                this->a->update(deltaTime);
                // Update the timestamp after a successful update
                f = this->a->getLastStateTimestamp();
            }
            catch (const std::exception& a) {
                std::cerr << "Exception in rocket update: " << a.what() << std::endl;
            }
        }
    }
    else {
        if (b) {
            try {
                b->checkGrounding(a);
                b->update(deltaTime);
            }
            catch (const std::exception& a) {
                std::cerr << "Exception in car update: " << a.what() << std::endl;
            }
        }
    }
}

void VehicleManager::draw(sf::RenderWindow& window) {
    if (!window.isOpen()) return;

    if (c == VehicleType::ROCKET) {
        if (a) {
            try {
                a->draw(window);
            }
            catch (const std::exception& a) {
                std::cerr << "Exception drawing rocket: " << a.what() << std::endl;
            }
        }
    }
    else {
        if (b) {
            try {
                b->draw(window);
            }
            catch (const std::exception& a) {
                std::cerr << "Exception drawing car: " << a.what() << std::endl;
            }
        }
    }
}

void VehicleManager::drawWithConstantSize(sf::RenderWindow& window, float zoomLevel)
{
    // Multiple safety checks
    if (!window.isOpen()) return;

    // Check that rocket and car pointers are valid before proceeding
    if (c == VehicleType::ROCKET) {
        if (!a) {
            std::cerr << "Warning: Attempted to draw null rocket in drawWithConstantSize" << std::endl;
            return; // Early return if rocket is null
        }

        try {
            a->drawWithConstantSize(window, zoomLevel);
        }
        catch (const std::exception& a) {
            std::cerr << "Exception drawing rocket: " << a.what() << std::endl;
        }
    }
    else if (c == VehicleType::CAR) {
        if (!b) {
            std::cerr << "Warning: Attempted to draw null car in drawWithConstantSize" << std::endl;
            return; // Early return if car is null
        }

        try {
            b->drawWithConstantSize(window, zoomLevel);
        }
        catch (const std::exception& a) {
            std::cerr << "Exception drawing car: " << a.what() << std::endl;
        }
    }
}

void VehicleManager::applyThrust(float amount) {
    if (c == VehicleType::ROCKET) {
        if (a) {
            try {
                a->applyThrust(amount);
            }
            catch (const std::exception& a) {
                std::cerr << "Exception applying thrust: " << a.what() << std::endl;
            }
        }
    }
    else {
        if (b) {
            try {
                b->accelerate(amount);
            }
            catch (const std::exception& a) {
                std::cerr << "Exception accelerating car: " << a.what() << std::endl;
            }
        }
    }
}

void VehicleManager::rotate(float amount) {
    if (c == VehicleType::ROCKET) {
        if (a) {
            try {
                a->rotate(amount);
            }
            catch (const std::exception& a) {
                std::cerr << "Exception rotating rocket: " << a.what() << std::endl;
            }
        }
    }
    else {
        if (b) {
            try {
                b->rotate(amount);
            }
            catch (const std::exception& a) {
                std::cerr << "Exception rotating car: " << a.what() << std::endl;
            }
        }
    }
}

void VehicleManager::drawVelocityVector(sf::RenderWindow& window, float scale) {
    if (!window.isOpen()) return;

    if (c == VehicleType::ROCKET) {
        if (a) {
            try {
                a->drawVelocityVector(window, scale);
            }
            catch (const std::exception& a) {
                std::cerr << "Exception drawing velocity vector: " << a.what() << std::endl;
            }
        }
    }
    // Car doesn't have a velocity vector display
}

GameObject* VehicleManager::getActiveVehicle() {
    if (c == VehicleType::ROCKET) {
        return a ? a.get() : nullptr;
    }
    else {
        return b ? b.get() : nullptr;
    }
}

void VehicleManager::updatePlanets(const std::vector<Planet*>& newPlanets) {
    try {
        // Update the internal planets vector with the new set of planets
        d.clear();
        for (auto* a : newPlanets) {
            if (a) {
                d.push_back(a);
            }
        }

        // Update the planet references in rocket
        if (a) {
            a->setNearbyPlanets(d);
        }

        // Update in car if needed
        if (b) {
            b->checkGrounding(d);
        }
    }
    catch (const std::exception& a) {
        std::cerr << "Exception in updatePlanets: " << a.what() << std::endl;
    }
}

void VehicleManager::createState(RocketState& state) const {
    if (a && c == VehicleType::ROCKET) {
        // Set player ID based on owner
        state.a = e;

        // Set position, velocity, rotation
        state.b = a->getPosition();
        state.c = a->getVelocity();
        state.d = a->getRotation();

        // Set other rocket properties
        state.e = 0.0f; // Angular velocity not tracked directly
        state.f = a->getThrustLevel();
        state.g = a->getMass();
        state.h = a->getColor();

        // Set timestamp
        state.i = f;

        // Flag this as authoritative for this client
        state.j = true;
    }
    else {
        // Create an empty state if no rocket exists
        state.a = e;
        state.b = sf::Vector2f(0, 0);
        state.c = sf::Vector2f(0, 0);
        state.d = 0.0f;
        state.e = 0.0f;
        state.f = 0.0f;
        state.g = 1.0f;
        state.h = sf::Color::White;
        state.i = f;
        state.j = false;
    }
}

void VehicleManager::applyState(const RocketState& state) {
    // Only apply states for our owner
    if (state.a != e) return;

    // Only apply if we're in rocket mode
    if (c != VehicleType::ROCKET || !a) return;

    // Only apply if the state is newer than our current state
    if (state.i <= f) return;

    // Apply the rocket state
    a->setPosition(state.b);
    a->setVelocity(state.c);
    a->setRotation(state.d);
    a->setThrustLevel(state.f);

    // Update our timestamp
    f = state.i;
    a->setLastStateTimestamp(state.i);
}