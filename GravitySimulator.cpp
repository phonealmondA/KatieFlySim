// Update in GravitySimulator.cpp
#include "GravitySimulator.h"
#include "VehicleManager.h"

void GravitySimulator::addPlanet(Planet* planet)
{
    planets.push_back(planet);
}

void GravitySimulator::addRocket(Rocket* rocket)
{
    rockets.push_back(rocket);
}

void GravitySimulator::clearRockets()
{
    rockets.clear();
}


void GravitySimulator::updateVehicleManagerPlanets() {
    if (!vehicleManager) return;

    try {
        vehicleManager->updatePlanets(planets);
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in updateVehicleManagerPlanets: " << e.what() << std::endl;
    }
}



void GravitySimulator::addRocketGravityInteractions(float deltaTime)
{
    // Apply gravity between rockets
    for (size_t i = 0; i < rockets.size(); i++) {
        for (size_t j = i + 1; j < rockets.size(); j++) {
            Rocket* rocket1 = rockets[i];
            Rocket* rocket2 = rockets[j];

            sf::Vector2f direction = rocket2->getPosition() - rocket1->getPosition();
            float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

            // Minimum distance to prevent extreme forces when very close
            const float minDistance = GameConstants::TRAJECTORY_COLLISION_RADIUS;
            if (distance < minDistance) {
                distance = minDistance;
            }

            // Apply inverse square law for gravity
            float forceMagnitude = G * rocket1->getMass() * rocket2->getMass() / (distance * distance);

            sf::Vector2f normalizedDir = normalize(direction);
            sf::Vector2f accel1 = normalizedDir * forceMagnitude / rocket1->getMass();
            sf::Vector2f accel2 = -normalizedDir * forceMagnitude / rocket2->getMass();

            rocket1->setVelocity(rocket1->getVelocity() + accel1 * deltaTime);
            rocket2->setVelocity(rocket2->getVelocity() + accel2 * deltaTime);
        }
    }
}



void GravitySimulator::checkPlanetCollisions() {
    if (planets.size() < 2) return;

    std::vector<Planet*> planetsToKeep;
    std::vector<Planet*> planetsToDelete;
    std::vector<bool> markedForDeletion(planets.size(), false);

    // First pass: identify small planets to remove and do null checks
    for (size_t i = 0; i < planets.size(); i++) {
        Planet* planet = planets[i];
        if (!planet) {
            // Mark null planets for deletion
            markedForDeletion[i] = true;
            continue;
        }

        // Check if the planet's mass is below threshold
        if (planet->getMass() < 10.0f) {
            planetsToDelete.push_back(planet);
            markedForDeletion[i] = true;
            planets[i] = nullptr; // Explicitly set to nullptr to prevent further access
        }
    }

    // Second pass: check for collisions between non-deleted planets
    for (size_t i = 0; i < planets.size(); i++) {
        if (markedForDeletion[i]) continue; // Skip already marked planets

        Planet* planet1 = planets[i];
        if (!planet1) continue; // Extra safety check

        for (size_t j = i + 1; j < planets.size(); j++) {
            if (markedForDeletion[j]) continue; // Skip already marked planets

            Planet* planet2 = planets[j];
            if (!planet2) continue; // Extra safety check

            // Calculate distance safely
            sf::Vector2f direction;
            try {
                direction = planet2->getPosition() - planet1->getPosition();
            }
            catch (const std::exception& e) {
                std::cerr << "Exception calculating planet distance: " << e.what() << std::endl;
                continue; // Skip this pair if an exception occurs
            }

            float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

            // Check for collision
            if (distance <= planet1->getRadius() + planet2->getRadius()) {
                try {
                    // Determine which planet is larger
                    if (planet1->getMass() >= planet2->getMass()) {
                        // Planet 1 absorbs planet 2
                        float newMass = planet1->getMass() + (planet2->getMass());

                        // Conservation of momentum for velocity
                        sf::Vector2f newVelocity = (planet1->getVelocity() * planet1->getMass() +
                            planet2->getVelocity() * planet2->getMass()) / newMass;

                        // Update planet 1
                        planet1->setMass(newMass);
                        planet1->setVelocity(newVelocity);

                        // Mark planet 2 for deletion
                        if (!markedForDeletion[j]) {
                            planetsToDelete.push_back(planet2);
                            markedForDeletion[j] = true;
                            planets[j] = nullptr; // Explicitly set to nullptr
                        }
                    }
                    else {
                        // Planet 2 absorbs planet 1
                        float newMass = planet1->getMass() + planet2->getMass();

                        // Conservation of momentum for velocity
                        sf::Vector2f newVelocity = (planet1->getVelocity() * planet1->getMass() +
                            planet2->getVelocity() * planet2->getMass()) / newMass;

                        // Update planet 2
                        planet2->setMass(newMass);
                        planet2->setVelocity(newVelocity);

                        // Mark planet 1 for deletion
                        if (!markedForDeletion[i]) {
                            planetsToDelete.push_back(planet1);
                            markedForDeletion[i] = true;
                            planets[i] = nullptr; // Explicitly set to nullptr
                        }
                        break; // Exit inner loop since planet 1 is gone
                    }
                }
                catch (const std::exception& e) {
                    std::cerr << "Exception during planet collision: " << e.what() << std::endl;
                    continue; // Skip this pair if an exception occurs
                }
            }
        }
    }

    // Build the filtered list of planets to keep
    for (size_t i = 0; i < planets.size(); i++) {
        if (!markedForDeletion[i] && planets[i]) {
            planetsToKeep.push_back(planets[i]);
        }
    }

    // Delete the marked planets safely
    for (Planet* planet : planetsToDelete) {
        if (planet) { // Extra safety check
            try {
                delete planet;
            }
            catch (const std::exception& e) {
                std::cerr << "Exception deleting planet: " << e.what() << std::endl;
            }
        }
    }

    // Replace the planets vector with the filtered list
    planets = planetsToKeep;

    // Update planets in vehicle manager - use our method
    try {
        updateVehicleManagerPlanets();
    }
    catch (const std::exception& e) {
        std::cerr << "Exception updating vehicle manager planets: " << e.what() << std::endl;
    }
}




void GravitySimulator::update(float deltaTime)
{
    // Apply gravity between planets if enabled
    if (simulatePlanetGravity) {
        for (size_t i = 0; i < planets.size(); i++) {
            for (size_t j = i + 1; j < planets.size(); j++) {
                Planet* planet1 = planets[i];
                Planet* planet2 = planets[j];

                // Skip the first planet (index 0) - it's pinned in place
                if (i == 0) {
                    // Only apply gravity from planet1 to planet2
                    sf::Vector2f direction = planet1->getPosition() - planet2->getPosition();
                    float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

                    if (distance > planet1->getRadius() + planet2->getRadius()) {
                        float forceMagnitude = G * planet1->getMass() * planet2->getMass() / (distance * distance);
                        sf::Vector2f normalizedDir = normalize(direction);
                        sf::Vector2f accel2 = normalizedDir * forceMagnitude / planet2->getMass();
                        planet2->setVelocity(planet2->getVelocity() + accel2 * deltaTime);
                    }
                }
                else {
                    // Regular gravity calculation between other planets
                    sf::Vector2f direction = planet2->getPosition() - planet1->getPosition();
                    float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

                    if (distance > planet1->getRadius() + planet2->getRadius()) {
                        float forceMagnitude = G * planet1->getMass() * planet2->getMass() / (distance * distance);
                        sf::Vector2f normalizedDir = normalize(direction);
                        sf::Vector2f accel1 = normalizedDir * forceMagnitude / planet1->getMass();
                        sf::Vector2f accel2 = -normalizedDir * forceMagnitude / planet2->getMass();
                        planet1->setVelocity(planet1->getVelocity() + accel1 * deltaTime);
                        planet2->setVelocity(planet2->getVelocity() + accel2 * deltaTime);
                    }
                }
            }
        }
    }

    // Apply gravity to the vehicle manager's active vehicle
    if (vehicleManager) {
        if (vehicleManager->getActiveVehicleType() == VehicleType::ROCKET) {
            Rocket* rocket = vehicleManager->getRocket();
            if (rocket) {
                for (auto planet : planets) {
                    sf::Vector2f direction = planet->getPosition() - rocket->getPosition();
                    float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

                    // Avoid division by zero and very small distances
                    if (distance > planet->getRadius() + GameConstants::TRAJECTORY_COLLISION_RADIUS) {
                        float forceMagnitude = G * planet->getMass() * rocket->getMass() / (distance * distance);
                        sf::Vector2f acceleration = normalize(direction) * forceMagnitude / rocket->getMass();
                        sf::Vector2f velocityChange = acceleration * deltaTime;
                        rocket->setVelocity(rocket->getVelocity() + velocityChange);
                    }
                }
            }
        }
        // Car gravity is handled internally in Car::update
    }
    else {
        // Legacy code for handling individual rockets
        for (auto rocket : rockets) {
            for (auto planet : planets) {
                sf::Vector2f direction = planet->getPosition() - rocket->getPosition();
                float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

                // Avoid division by zero and very small distances
                if (distance > planet->getRadius() + GameConstants::TRAJECTORY_COLLISION_RADIUS) {
                    float forceMagnitude = G * planet->getMass() * rocket->getMass() / (distance * distance);
                    sf::Vector2f acceleration = normalize(direction) * forceMagnitude / rocket->getMass();
                    sf::Vector2f velocityChange = acceleration * deltaTime;
                    rocket->setVelocity(rocket->getVelocity() + velocityChange);
                }
            }
        }

        // Add rocket-to-rocket gravity interactions
        addRocketGravityInteractions(deltaTime);
    }

    // Check for planet collisions and cleanup too-small planets
    checkPlanetCollisions();
}