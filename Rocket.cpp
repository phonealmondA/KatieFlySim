// Rocket.cpp
#include "Rocket.h"
#include "VectorHelper.h"
#include "GameConstants.h"
#include <cmath>
#include <iostream>
#include <cstdint>  // For uint8_t
#include <cstdlib>  // For rand()
#include <ctime>    // For std::time

Rocket::Rocket(sf::Vector2f pos, sf::Vector2f vel, sf::Color col, float mass, int ownerId)
    : GameObject(pos, vel, col), c(0), d(0),
    e(0.0f), f(), g(mass), h(0.0f),
    i(GameConstants::BASE_FUEL_CONSUMPTION_RATE),
    k(1.0f), l(1.0f),
    m(false), n(ownerId), o(0.0f)  // Initialize new variables
{
    try {
        // Create rocket body (a simple triangle)
        a.setPointCount(3);
        a.setPoint(0, { 0, -GameConstants::ROCKET_SIZE });
        a.setPoint(1, { -GameConstants::ROCKET_SIZE / 2, GameConstants::ROCKET_SIZE });
        a.setPoint(2, { GameConstants::ROCKET_SIZE / 2, GameConstants::ROCKET_SIZE });
        a.setFillColor(color);
        a.setOrigin({ 0, 0 });
        a.setPosition(position);

        // Add default engine
        addPart(std::make_unique<Engine>(sf::Vector2f(0, GameConstants::ROCKET_SIZE), GameConstants::ENGINE_THRUST_POWER));

        // Initialize stored mass visual
        j.setFillColor(sf::Color(100, 200, 255, 180)); // Light blue, semi-transparent
        j.setOrigin(sf::Vector2f(0.0f, 0.0f));
        updateStoredMassVisual();
    }
    catch (const std::exception& a) {
        std::cerr << "Exception in Rocket constructor: " << a.what() << std::endl;
    }
}

void Rocket::updateStoredMassVisual() {
    // Size based on stored mass (with minimum size)
    float a = std::max(5.0f, std::sqrt(h) * 3.0f);
    j.setRadius(a);
    j.setOrigin(sf::Vector2f(a, a));

    // Position at the front of the rocket, distance adjusted by size
    float b = GameConstants::ROCKET_SIZE + a * 2.0f;
    float c = this->c * 3.14159f / 180.0f;
    // In SFML, 0 degrees is right, 90 degrees is down
    sf::Vector2f d(std::sin(c), -std::cos(c));
    j.setPosition(position + d * b);
}

RocketState Rocket::createState() const {
    RocketState a;
    a.a = n; // playerId = ownerId
    a.b = position; // position
    a.c = velocity; // velocity
    a.d = c; // rotation
    a.e = d; // angularVelocity
    a.f = e; // thrustLevel
    a.g = g; // mass
    a.h = color; // color
    a.i = o; // lastStateTimestamp
    a.j = true; // isAuthoritative

    return a;
}

void Rocket::applyState(const RocketState& state) {
    // Only apply if this state is for our rocket
    if (state.a != n) return;

    // Only apply if newer than our current state
    if (state.i <= o) return;

    position = state.b;
    velocity = state.c;
    c = state.d;
    d = state.e;
    e = state.f;
    g = state.g;
    color = state.h;
    o = state.i;

    // Update visuals
    a.setPosition(position);
    a.setRotation(sf::degrees(c));
    updateStoredMassVisual();
}

void Rocket::addStoredMass(float amount) {
    // Add to stored mass
    h += amount;

    // Make sure stored mass doesn't go negative
    if (h < 0.0f) {
        h = 0.0f;
    }

    // Update total mass (base mass of 1.0 + stored mass)
    g = 1.0f + h;

    // Update visual representation
    updateStoredMassVisual();

    // Update timestamp
    o = static_cast<float>(std::time(nullptr));
}

bool Rocket::upgradeThrust(float massCost) {
    // Check if we have enough stored mass
    if (h < massCost) {
        return false;
    }

    // Spend the mass
    h -= massCost;
    g = 1.0f + h; // Update total mass

    // Increase thrust multiplier by 10%
    k += 0.001f;

    // Update visual representation
    updateStoredMassVisual();

    // Update timestamp
    o = static_cast<float>(std::time(nullptr));

    return true;
}

bool Rocket::upgradeEfficiency(float massCost) {
    // Check if we have enough stored mass
    if (h < massCost) {
        return false;
    }

    // Spend the mass
    h -= massCost;
    g = 1.0f + h; // Update total mass

    // Increase efficiency multiplier by 10%
    l += 0.1f;

    // Update fuel consumption rate based on efficiency
    i = GameConstants::BASE_FUEL_CONSUMPTION_RATE / l;

    // Update visual representation
    updateStoredMassVisual();

    // Update timestamp
    o = static_cast<float>(std::time(nullptr));

    return true;
}

Planet* Rocket::dropStoredMass() {
    if (h < 0.1f) {
        return nullptr; // Not enough mass to drop
    }

    // Calculate position in front of the rocket
    float a = c * 3.14159f / 180.0f;
    sf::Vector2f b(-std::sin(a), std::cos(a));
    sf::Vector2f c = position + b * (GameConstants::ROCKET_SIZE * 2.0f);

    // Create a new planet with the stored mass
    uint8_t d = 100 + (rand() % 156); // 100-255
    uint8_t e = 100 + (rand() % 156); // 100-255
    uint8_t f = 100 + (rand() % 156); // 100-255
    sf::Color g(d, e, f);

    // Create a new planet with the stored mass and random color and owned by this rocket's owner
    Planet* i = new Planet(c, 0, h, g, n);

    // Give it the rocket's velocity plus a small offset to prevent immediate collisions
    i->setVelocity(velocity + b * 10.0f);

    // Reset stored mass
    h = 0.0f;
    this->g = 1.0f; // Base mass
    updateStoredMassVisual();

    // Update timestamp
    o = static_cast<float>(std::time(nullptr));

    return i;
}

void Rocket::consumeFuel(float deltaTime) {
    // Only consume fuel when thrust is actually being applied AND thrust level is positive
    if (m && e > 0.0f && h > 0.0f) {
        // Calculate mass to consume based on thrust level squared (for more dramatic effect)
        // Higher thrust levels will consume disproportionately more fuel
        // The efficiency multiplier reduces the consumption rate
        float a = e * e; // Square the thrust level for more dramatic scaling
        float b = (i * a * deltaTime) / l;

        // Don't consume more than we have
        b = std::min(b, h);

        // Update stored mass and total mass
        h -= b;
        g = 1.0f + h; // Base mass (1.0) + stored mass

        // Update visual
        updateStoredMassVisual();
    }

    // Reset the thrusting flag after consumption is calculated
    m = false;
}

bool Rocket::hasFuel() const {
    return h > 0.0f;
}

void Rocket::setNearbyPlanets(const std::vector<Planet*>& planets) {
    // Check if input is empty first
    if (planets.empty()) {
        f.clear();
        return;
    }
    try {
        // Clear existing planets first
        f.clear();
        // Only add non-null planets
        for (auto* a : planets) {
            if (a) {
                f.push_back(a);
            }
        }
    }
    catch (const std::exception& a) {
        std::cerr << "Exception in setNearbyPlanets: " << a.what() << std::endl;
        // Ensure we have a valid vector even if an exception occurs
        f.clear();
    }
}

void Rocket::addPart(std::unique_ptr<RocketPart> part)
{
    if (part) {
        b.push_back(std::move(part));
    }
}

void Rocket::applyThrust(float amount)
{
    // Set the thrusting flag based on the amount
    m = (amount != 0.0f);

    // Only apply thrust if we have fuel or if braking (negative thrust)
    if (hasFuel() || amount < 0) {
        // Calculate thrust direction based on rocket rotation
        float a = c * 3.14159f / 180.0f;

        // In SFML, 0 degrees points up, 90 degrees points right
        // So we need to use -sin for x and -cos for y to get the direction
        sf::Vector2f b(std::sin(a), -std::cos(a));

        // Apply force with thrust multiplier
        velocity += b * amount * e * k / g;
    }

    // Update timestamp
    o = static_cast<float>(std::time(nullptr));
}

void Rocket::rotate(float amount)
{
    d += amount;

    // Update timestamp
    o = static_cast<float>(std::time(nullptr));
}

void Rocket::setThrustLevel(float level)
{
    // Clamp level between 0.0 and 1.0
    e = std::max(0.0f, std::min(1.0f, level));

    // Update timestamp
    o = static_cast<float>(std::time(nullptr));
}

bool Rocket::checkCollision(const Planet& planet)
{
    float a = distance(position, planet.getPosition());
    // Simple collision check based on distance
    return a < planet.getRadius() + GameConstants::ROCKET_SIZE;
}

bool Rocket::isColliding(const Planet& planet)
{
    return checkCollision(planet);
}

// update function is critical for the distributed simulation
void Rocket::update(float deltaTime)
{
    try {
        // Consume fuel when thrusting
        consumeFuel(deltaTime);

        bool a = false;

        // Check if we're resting on any planet
        if (!f.empty()) {
            for (const auto& p : f) {
                if (!p) continue; // Skip null planets

                sf::Vector2f b = position - p->getPosition();
                float dist = std::sqrt(b.x * b.x + b.y * b.y);

                // If we're at or below the surface of the planet
                if (dist <= (p->getRadius() + GameConstants::ROCKET_SIZE)) {
                    // Calculate normal force direction (away from planet center)
                    sf::Vector2f d = normalize(b);

                    // Project velocity onto normal to see if we're moving into the planet
                    float e = velocity.x * d.x + velocity.y * d.y;

                    if (e < 0) {
                        // Remove velocity component toward the planet
                        velocity -= d * e;

                        // Apply a small friction to velocity parallel to surface
                        sf::Vector2f f(-d.y, d.x);
                        float g = velocity.x * f.x + velocity.y * f.y;
                        velocity = f * g * 0.98f;

                        // Position correction to stay exactly on surface
                        position = p->getPosition() + d * (p->getRadius() + GameConstants::ROCKET_SIZE);

                        a = true;
                    }
                }
            }
        }

        // Only apply normal updates if not resting on a planet
        if (!a) {
            position += velocity * deltaTime;
        }

        // Update rotation based on angular velocity
        c += d * deltaTime;

        // Apply some damping to angular velocity
        d *= 0.98f;

        // Update body position and rotation
        this->a.setPosition(position);
        this->a.setRotation(sf::degrees(c));

        // Update the stored mass visual
        updateStoredMassVisual();

        // Update timestamp
        o = static_cast<float>(std::time(nullptr));
    }
    catch (const std::exception& a) {
        std::cerr << "Exception in Rocket::update: " << a.what() << std::endl;
    }
}

// MISSING IMPLEMENTATIONS ADDED BELOW:

void Rocket::draw(sf::RenderWindow& window) {
    // Draw the rocket body
    window.draw(a);

    // Draw all rocket parts
    for (const auto& part : b) {
        if (part) {
            part->draw(window, position, c, 1.0f, e, h > 0.0f);
        }
    }

    // Draw stored mass visual if we have stored mass
    if (h > 0.0f) {
        window.draw(j);
    }
}

void Rocket::drawWithConstantSize(sf::RenderWindow& window, float zoomLevel) {
    // Create a scaled copy of the rocket shape
    sf::ConvexShape scaledBody = a;

    // Scale the points
    for (size_t i = 0; i < scaledBody.getPointCount(); i++) {
        sf::Vector2f point = a.getPoint(i);
        scaledBody.setPoint(i, point * zoomLevel);
    }

    // Set position and rotation
    scaledBody.setPosition(position);
    scaledBody.setRotation(sf::degrees(c));

    // Draw the scaled body
    window.draw(scaledBody);

    // Draw all parts with the zoom scale
    for (const auto& part : b) {
        if (part) {
            part->draw(window, position, c, zoomLevel, e, h > 0.0f);
        }
    }

    // Scale and draw stored mass visual if we have stored mass
    if (h > 0.0f) {
        sf::CircleShape scaledMass = j;
        scaledMass.setRadius(j.getRadius() * zoomLevel);
        scaledMass.setOrigin(j.getOrigin() * zoomLevel);
        scaledMass.setPosition(j.getPosition());
        window.draw(scaledMass);
    }
}

void Rocket::drawVelocityVector(sf::RenderWindow& window, float scale) {
    // Create a vertex array for the velocity vector line
    sf::VertexArray velocityLine(sf::PrimitiveType::LineStrip);

    // Start point at rocket position
    sf::Vertex start;
    start.position = position;
    start.color = sf::Color::Yellow;
    velocityLine.append(start);

    // End point at position + scaled velocity
    sf::Vertex end;
    end.position = position + velocity * scale;
    end.color = sf::Color::Green;
    velocityLine.append(end);

    // Draw the velocity vector
    window.draw(velocityLine);
}

void Rocket::drawGravityForceVectors(sf::RenderWindow& window, const std::vector<Planet*>& planets, float scale) {
    // For each planet, draw a line representing gravity force
    for (const auto& planet : planets) {
        if (!planet) continue; // Skip null planets

        // Calculate direction to planet
        sf::Vector2f direction = planet->getPosition() - position;
        float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

        // Skip if too close or too far
        if (distance <= planet->getRadius() || distance > 2000.0f) {
            continue;
        }

        // Calculate force magnitude using gravity formula
        float forceMagnitude = GameConstants::G * planet->getMass() * g / (distance * distance);

        // Normalize the direction vector
        sf::Vector2f normalizedDir = normalize(direction);

        // Scale the force for visualization
        sf::Vector2f forceVector = normalizedDir * forceMagnitude * scale;

        // Create a vertex array for the force line
        sf::VertexArray forceLine(sf::PrimitiveType::LineStrip);

        // Start point at rocket position
        sf::Vertex start;
        start.position = position;
        start.color = sf::Color::Blue;
        forceLine.append(start);

        // End point at position + scaled force
        sf::Vertex end;
        end.position = position + forceVector;
        end.color = sf::Color::Red;
        forceLine.append(end);

        // Draw the force vector
        window.draw(forceLine);
    }
}

void Rocket::drawTrajectory(sf::RenderWindow& window, const std::vector<Planet*>& planets, float timeStep, int steps, bool detectSelfIntersection) {
    // Create a vertex array for the trajectory
    sf::VertexArray trajectoryLine(sf::PrimitiveType::LineStrip);

    // Start with current position and velocity
    sf::Vector2f simPos = position;
    sf::Vector2f simVel = velocity;
    float simMass = g;

    // Add the starting point
    sf::Vertex startPoint;
    startPoint.position = simPos;
    startPoint.color = sf::Color(color.r, color.g, color.b, 100); // Semi-transparent
    trajectoryLine.append(startPoint);

    // Track if we've hit something to stop the trajectory
    bool hitObject = false;

    // Simulate future positions
    for (int step = 0; step < steps && !hitObject; step++) {
        // Calculate gravitational forces from all planets
        sf::Vector2f totalForce(0, 0);

        for (const auto& planet : planets) {
            if (!planet) continue; // Skip null planets

            // Skip if the planet is ourselves (when projecting a planet's orbit)
            //if (planet == this) continue;

            sf::Vector2f direction = planet->getPosition() - simPos;
            float dist = std::sqrt(direction.x * direction.x + direction.y * direction.y);

            // Check for collision with planet
            if (dist <= planet->getRadius() + GameConstants::TRAJECTORY_COLLISION_RADIUS) {
                hitObject = true;
                break;
            }

            // Calculate gravitational force
            float forceMag = GameConstants::G * planet->getMass() * simMass / (dist * dist);
            sf::Vector2f forceDir = normalize(direction);
            totalForce += forceDir * forceMag;
        }

        if (hitObject) break;

        // Update simulated velocity and position
        simVel += totalForce / simMass * timeStep;
        simPos += simVel * timeStep;

        // Calculate fade-out effect
        float alpha = 255 * (1.0f - static_cast<float>(step) / steps);

        // Add point to trajectory
        sf::Vertex point;
        point.position = simPos;
        point.color = sf::Color(color.r, color.g, color.b, static_cast<uint8_t>(alpha));
        trajectoryLine.append(point);

        // Check for self-intersection if requested
        if (detectSelfIntersection && step > 10) {
            // Simple implementation: just check against starting point
            float distToStart = std::sqrt(
                (simPos.x - position.x) * (simPos.x - position.x) +
                (simPos.y - position.y) * (simPos.y - position.y)
            );

            if (distToStart < GameConstants::ROCKET_SIZE) {
                // Add a special marker at intersection point
                sf::Vertex marker;
                marker.position = simPos;
                marker.color = sf::Color::Yellow;
                trajectoryLine.append(marker);
                break;
            }
        }
    }

    // Draw the trajectory
    window.draw(trajectoryLine);
}