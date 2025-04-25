// Rocket.cpp
#include "Rocket.h"
#include "VectorHelper.h"
#include "GameConstants.h"
#include <cmath>
#include <iostream>

Rocket::Rocket(sf::Vector2f pos, sf::Vector2f vel, sf::Color col, float m)
    : GameObject(pos, vel, col), rotation(0), angularVelocity(0),
    thrustLevel(0.0f), mass(m), storedMass(0.0f),
    fuelConsumptionRate(GameConstants::BASE_FUEL_CONSUMPTION_RATE)
{
    try {
        // Create rocket body (a simple triangle)
        body.setPointCount(3);
        body.setPoint(0, { 0, -GameConstants::ROCKET_SIZE });
        body.setPoint(1, { -GameConstants::ROCKET_SIZE / 2, GameConstants::ROCKET_SIZE });
        body.setPoint(2, { GameConstants::ROCKET_SIZE / 2, GameConstants::ROCKET_SIZE });
        body.setFillColor(color);
        body.setOrigin({ 0, 0 });
        body.setPosition(position);

        // Add default engine
        addPart(std::make_unique<Engine>(sf::Vector2f(0, GameConstants::ROCKET_SIZE), GameConstants::ENGINE_THRUST_POWER));
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in Rocket constructor: " << e.what() << std::endl;
    }
}

void Rocket::addStoredMass(float amount) {
    // Add to stored mass
    storedMass += amount;

    // Make sure stored mass doesn't go negative
    if (storedMass < 0.0f) {
        storedMass = 0.0f;
    }

    // Update total mass (base mass of 1.0 + stored mass)
    mass = 1.0f + storedMass;
}

void Rocket::consumeFuel(float deltaTime) {
    // Only consume fuel if thrusting AND thrust is actually being applied
    // This requires passing in an "isThrusting" parameter or tracking it in the Rocket class
    static bool isThrusting = false;

    // Get current thrust input state
    bool currentThrusting = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up);

    // Only consume fuel when actively applying thrust with keys
    if (thrustLevel > 0.0f && currentThrusting) {
        // Calculate mass to consume based on thrust level and time
        float consumedMass = fuelConsumptionRate * thrustLevel * deltaTime;

        // Don't consume more than we have
        consumedMass = std::min(consumedMass, storedMass);

        // Update stored mass and total mass
        storedMass -= consumedMass;
        mass = 1.0f + storedMass; // Base mass (1.0) + stored mass
    }

    // Store current thrust state for next frame
    isThrusting = currentThrusting;
}

bool Rocket::hasFuel() const {
    return storedMass > 0.0f;
}

void Rocket::setNearbyPlanets(const std::vector<Planet*>& planets) {
    // Check if input is empty first
    if (planets.empty()) {
        nearbyPlanets.clear();
        return;
    }
    try {
        // Clear existing planets first
        nearbyPlanets.clear();
        // Only add non-null planets
        for (auto* planet : planets) {  // Changed from 'const auto*' to 'auto*'
            if (planet) {
                nearbyPlanets.push_back(planet);
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in setNearbyPlanets: " << e.what() << std::endl;
        // Ensure we have a valid vector even if an exception occurs
        nearbyPlanets.clear();
    }
}

void Rocket::addPart(std::unique_ptr<RocketPart> part)
{
    if (part) {
        parts.push_back(std::move(part));
    }
}

void Rocket::applyThrust(float amount)
{
    // Only apply thrust if we have fuel or if braking (negative thrust)
    if (hasFuel() || amount < 0) {
        // Calculate thrust direction based on rocket rotation
        float radians = rotation * 3.14159f / 180.0f;

        // In SFML, 0 degrees points up, 90 degrees points right
        // So we need to use -sin for x and -cos for y to get the direction
        sf::Vector2f thrustDir(std::sin(radians), -std::cos(radians));

        // Apply force and convert to acceleration by dividing by mass (F=ma -> a=F/m)
        velocity += thrustDir * amount * thrustLevel / mass;
    }
}

void Rocket::rotate(float amount)
{
    angularVelocity += amount;
}

void Rocket::setThrustLevel(float level)
{
    // Clamp level between 0.0 and 1.0
    thrustLevel = std::max(0.0f, std::min(1.0f, level));
}

bool Rocket::checkCollision(const Planet& planet)
{
    float dist = distance(position, planet.getPosition());
    // Simple collision check based on distance
    return dist < planet.getRadius() + GameConstants::ROCKET_SIZE;
}

bool Rocket::isColliding(const Planet& planet)
{
    return checkCollision(planet);
}

Rocket* Rocket::mergeWith(Rocket* other)
{
    if (!other) return nullptr;

    try {
        // Create a new rocket with combined properties
        sf::Vector2f mergedPosition = (position + other->getPosition()) / 2.0f;

        // Conservation of momentum: (m1v1 + m2v2) / (m1 + m2)
        sf::Vector2f mergedVelocity = (velocity * mass + other->getVelocity() * other->getMass())
            / (mass + other->getMass());

        // Use the color of the more massive rocket
        sf::Color mergedColor = (mass > other->getMass()) ? color : other->color;

        // Create a new rocket with combined mass
        float mergedMass = mass + other->getMass();
        Rocket* mergedRocket = new Rocket(mergedPosition, mergedVelocity, mergedColor, mergedMass);

        // Combine thrust capabilities by adding an engine with combined thrust power
        float combinedThrust = 0.0f;

        // This is simplified - in a real implementation, you'd need to loop through
        // all engines from both rockets and sum their thrust values
        for (const auto& part : parts) {
            if (part && dynamic_cast<Engine*>(part.get())) {
                combinedThrust += dynamic_cast<Engine*>(part.get())->getThrust();
            }
        }

        for (const auto& part : other->parts) {
            if (part && dynamic_cast<Engine*>(part.get())) {
                combinedThrust += dynamic_cast<Engine*>(part.get())->getThrust();
            }
        }

        // Add a more powerful engine to the merged rocket
        mergedRocket->addPart(std::make_unique<Engine>(sf::Vector2f(0, GameConstants::ROCKET_SIZE), combinedThrust));

        return mergedRocket;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in Rocket::mergeWith: " << e.what() << std::endl;
        return nullptr;
    }
}

void Rocket::update(float deltaTime)
{
    try {
        // Consume fuel when thrusting
        consumeFuel(deltaTime);

        bool resting = false;

        // Check if we're resting on any planet
        if (!nearbyPlanets.empty()) {
            for (const auto& planet : nearbyPlanets) {
                if (!planet) continue; // Skip null planets

                sf::Vector2f direction = position - planet->getPosition();
                float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

                // If we're at or below the surface of the planet
                if (distance <= (planet->getRadius() + GameConstants::ROCKET_SIZE)) {
                    // Calculate normal force direction (away from planet center)
                    sf::Vector2f normal = normalize(direction);

                    // Project velocity onto normal to see if we're moving into the planet
                    float velDotNormal = velocity.x * normal.x + velocity.y * normal.y;

                    if (velDotNormal < 0) {
                        // Remove velocity component toward the planet
                        velocity -= normal * velDotNormal;

                        // Apply a small friction to velocity parallel to surface
                        sf::Vector2f tangent(-normal.y, normal.x);
                        float velDotTangent = velocity.x * tangent.x + velocity.y * tangent.y;
                        velocity = tangent * velDotTangent * 0.98f;

                        // Position correction to stay exactly on surface
                        position = planet->getPosition() + normal * (planet->getRadius() + GameConstants::ROCKET_SIZE);

                        resting = true;
                    }
                }
            }
        }

        // Only apply normal updates if not resting on a planet
        if (!resting) {
            position += velocity * deltaTime;
        }

        // Update rotation based on angular velocity
        rotation += angularVelocity * deltaTime;

        // Apply some damping to angular velocity
        angularVelocity *= 0.98f;

        // Update body position and rotation
        body.setPosition(position);
        body.setRotation(sf::degrees(rotation));
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in Rocket::update: " << e.what() << std::endl;
    }
}

void Rocket::draw(sf::RenderWindow& window)
{
    try {
        if (!window.isOpen()) return;

        // Draw rocket body
        window.draw(body);

        // Draw all rocket parts
        for (const auto& part : parts) {
            if (part) {
                part->draw(window, position, rotation, 1.0f, thrustLevel, hasFuel());
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in Rocket::draw: " << e.what() << std::endl;
    }
}

void Rocket::drawWithConstantSize(sf::RenderWindow& window, float zoomLevel)
{
    try {
        if (!window.isOpen()) return;

        // Store original position and scale
        sf::ConvexShape scaledBody = body;

        // Scale the body based on zoom level to maintain visual size
        float scaleMultiplier = zoomLevel;

        // Apply the scaling to the body shape
        // We're adjusting the points directly to keep the rocket centered properly
        for (size_t i = 0; i < scaledBody.getPointCount(); i++) {
            sf::Vector2f point = body.getPoint(i);
            scaledBody.setPoint(i, point * scaleMultiplier);
        }

        // Draw the scaled body
        window.draw(scaledBody);

        // Draw rocket parts with appropriate scaling
        for (const auto& part : parts) {
            if (part) {
                part->draw(window, position, rotation, scaleMultiplier, thrustLevel, hasFuel());
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in Rocket::drawWithConstantSize: " << e.what() << std::endl;
    }
}

void Rocket::drawVelocityVector(sf::RenderWindow& window, float scale)
{
    try {
        if (!window.isOpen()) return;

        sf::VertexArray line(sf::PrimitiveType::LineStrip);

        sf::Vertex startVertex;
        startVertex.position = position;
        startVertex.color = sf::Color::Yellow;
        line.append(startVertex);

        sf::Vertex endVertex;
        endVertex.position = position + velocity * scale;
        endVertex.color = sf::Color::Red;
        line.append(endVertex);

        window.draw(line);
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in Rocket::drawVelocityVector: " << e.what() << std::endl;
    }
}

void Rocket::drawGravityForceVectors(sf::RenderWindow& window, const std::vector<Planet*>& planets, float scale)
{
    try {
        if (!window.isOpen()) return;
        if (planets.empty()) return;

        // Gravitational constant - same as in GravitySimulator
        const float G = GameConstants::G;

        // Draw gravity force vector for each planet
        for (const auto& planet : planets) {
            if (!planet) continue; // Skip null planets

            // Calculate direction and distance
            sf::Vector2f direction = planet->getPosition() - position;
            float dist = std::sqrt(direction.x * direction.x + direction.y * direction.y);

            // Skip if we're inside the planet or too close
            if (dist <= planet->getRadius() + 15.0f) {
                continue;
            }

            // Calculate gravitational force
            float forceMagnitude = G * planet->getMass() * mass / (dist * dist);
            sf::Vector2f forceVector = normalize(direction) * forceMagnitude;

            // Scale the force for visualization
            forceVector *= scale / mass;

            // Create a vertex array for the force line
            sf::VertexArray forceLine(sf::PrimitiveType::LineStrip);

            // Start point (at rocket position)
            sf::Vertex startVertex;
            startVertex.position = position;
            startVertex.color = sf::Color::Magenta; // Pink color
            forceLine.append(startVertex);

            // End point (force direction and magnitude)
            sf::Vertex endVertex;
            endVertex.position = position + forceVector;
            endVertex.color = sf::Color(255, 20, 147); // Deep pink
            forceLine.append(endVertex);

            // Draw the force vector
            window.draw(forceLine);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in Rocket::drawGravityForceVectors: " << e.what() << std::endl;
    }
}

void Rocket::drawTrajectory(sf::RenderWindow& window, const std::vector<Planet*>& planets,
    float timeStep, int steps, bool detectSelfIntersection) {
    try {
        if (!window.isOpen()) return;
        if (planets.empty()) return;

        // Filter out null planets
        std::vector<Planet*> validPlanets;
        for (auto* planet : planets) {
            if (planet) validPlanets.push_back(planet);
        }

        if (validPlanets.empty()) return;

        // Create a vertex array for the trajectory line
        sf::VertexArray trajectory(sf::PrimitiveType::LineStrip);

        // Start with current position and velocity
        sf::Vector2f simPosition = position;
        sf::Vector2f simVelocity = velocity;

        // Add the starting point
        sf::Vertex startPoint;
        startPoint.position = simPosition;
        startPoint.color = sf::Color::Blue; // Blue at the beginning
        trajectory.append(startPoint);

        // Store previous positions
        std::vector<sf::Vector2f> previousPositions;
        previousPositions.push_back(simPosition);

        // Use the same gravitational constant as defined in GameConstants
        const float G = GameConstants::G;
        const float selfIntersectionThreshold = GameConstants::TRAJECTORY_COLLISION_RADIUS;

        // Create simulated planet positions and velocities
        std::vector<sf::Vector2f> simPlanetPositions;
        std::vector<sf::Vector2f> simPlanetVelocities;

        // Initialize simulated planet data
        for (const auto& planet : validPlanets) {
            simPlanetPositions.push_back(planet->getPosition());
            simPlanetVelocities.push_back(planet->getVelocity());
        }

        // Simulate future positions using more accurate physics
        for (int i = 0; i < steps; i++) {
            // Update simulated planet positions
            for (size_t j = 0; j < validPlanets.size(); j++) {
                simPlanetPositions[j] += simPlanetVelocities[j] * timeStep;
            }

            // Calculate gravitational interactions between planets
            for (size_t j = 0; j < validPlanets.size(); j++) {
                // Skip the first planet (index 0) if it's pinned in place
                if (j == 0) continue;

                sf::Vector2f totalPlanetAcceleration(0, 0);

                // Calculate gravity from other planets
                for (size_t k = 0; k < validPlanets.size(); k++) {
                    if (j == k) continue; // Skip self

                    sf::Vector2f direction = simPlanetPositions[k] - simPlanetPositions[j];
                    float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

                    if (distance > validPlanets[k]->getRadius() + validPlanets[j]->getRadius()) {
                        float forceMagnitude = G * validPlanets[k]->getMass() * validPlanets[j]->getMass() / (distance * distance);
                        sf::Vector2f acceleration = normalize(direction) * forceMagnitude / validPlanets[j]->getMass();
                        totalPlanetAcceleration += acceleration;
                    }
                }

                // Update planet velocity
                simPlanetVelocities[j] += totalPlanetAcceleration * timeStep;
            }

            // Calculate gravitational forces from all planets
            sf::Vector2f totalAcceleration(0, 0);
            bool collisionDetected = false;

            // Calculate gravitational interactions between planets and rocket
            for (size_t j = 0; j < validPlanets.size(); j++) {
                const auto& planet = validPlanets[j];
                sf::Vector2f planetPos = simPlanetPositions[j];

                sf::Vector2f direction = planetPos - simPosition;
                float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

                // Check for planet collision using consistent collision radius
                if (distance <= planet->getRadius() + GameConstants::TRAJECTORY_COLLISION_RADIUS) {
                    collisionDetected = true;
                    break;
                }

                // Calculate gravitational force with proper inverse square law
                // F = G * (m1 * m2) / r^2
                float forceMagnitude = G * planet->getMass() * mass / (distance * distance);

                // Convert force to acceleration (a = F/m)
                sf::Vector2f acceleration = normalize(direction) * forceMagnitude / mass;
                totalAcceleration += acceleration;
            }

            // Stop if we hit a planet
            if (collisionDetected) {
                break;
            }

            // Use velocity Verlet integration for better numerical stability
            sf::Vector2f halfStepVelocity = simVelocity + totalAcceleration * (timeStep * 0.5f);
            simPosition += halfStepVelocity * timeStep;

            // Recalculate acceleration at new position for higher accuracy
            sf::Vector2f newAcceleration(0, 0);

            for (size_t j = 0; j < validPlanets.size(); j++) {
                const auto& planet = validPlanets[j];
                sf::Vector2f planetPos = simPlanetPositions[j];

                sf::Vector2f direction = planetPos - simPosition;
                float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

                if (distance <= planet->getRadius() + GameConstants::TRAJECTORY_COLLISION_RADIUS) {
                    collisionDetected = true;
                    break;
                }

                float forceMagnitude = G * planet->getMass() * mass / (distance * distance);
                sf::Vector2f acceleration = normalize(direction) * forceMagnitude / mass;
                newAcceleration += acceleration;
            }

            if (collisionDetected) {
                break;
            }

            // Complete the velocity update with the new acceleration
            simVelocity = halfStepVelocity + newAcceleration * (timeStep * 0.5f);

            // Self-intersection check if enabled
            if (detectSelfIntersection) {
                for (size_t j = 0; j < previousPositions.size() - 10; j++) {
                    float distToPoint = distance(simPosition, previousPositions[j]);
                    if (distToPoint < selfIntersectionThreshold) {
                        collisionDetected = true;
                        break;
                    }
                }

                if (collisionDetected) {
                    break;
                }
            }

            // Store this position for future self-intersection checks
            previousPositions.push_back(simPosition);

            // Calculate color gradient from blue to pink
            float ratio = static_cast<float>(i) / steps;
            sf::Color pointColor(
                51 + 204 * ratio,  // R: 51 (blue) to 255 (pink)
                51 + 0 * ratio,    // G: 51 (blue) to 51 (pink)
                255 - 155 * ratio  // B: 255 (blue) to 100 (pink)
            );

            // Add point to trajectory
            sf::Vertex point;
            point.position = simPosition;
            point.color = pointColor;
            trajectory.append(point);
        }

        // Draw the trajectory
        window.draw(trajectory);
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in Rocket::drawTrajectory: " << e.what() << std::endl;
    }
}