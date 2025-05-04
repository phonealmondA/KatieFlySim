#include "Planet.h"
#include "VectorHelper.h"
#include "GameConstants.h"
#include <cmath>

Planet::Planet(sf::Vector2f pos, float radius, float mass, sf::Color color, int ownerId)
    : GameObject(pos, { 0, 0 }, color), b(mass), d(ownerId)
{
    // If a specific radius was provided, use it
    if (radius > 0) {
        this->c = radius;
    }
    else {
        // Otherwise calculate from mass
        updateRadiusFromMass();
    }

    a.setRadius(this->c);
    a.setFillColor(color);
    a.setOrigin({ this->c, this->c });
    a.setPosition(position);
}

void Planet::update(float deltaTime)
{
    position += velocity * deltaTime;
    a.setPosition(position);
}

void Planet::draw(sf::RenderWindow& window)
{
    window.draw(a);
}

float Planet::getMass() const
{
    return b;
}

float Planet::getRadius() const
{
    return c;
}

void Planet::setMass(float newMass)
{
    b = newMass;
    updateRadiusFromMass();
}

void Planet::updateRadiusFromMass()
{
    // Use cube root relationship between mass and radius
    c = GameConstants::BASE_RADIUS_FACTOR *
        std::pow(b / GameConstants::REFERENCE_MASS, 1.0f / 3.0f);

    // Update the visual shape
    a.setRadius(c);
    a.setOrigin({ c, c });
}

void Planet::drawVelocityVector(sf::RenderWindow& window, float scale)
{
    sf::VertexArray a(sf::PrimitiveType::LineStrip);

    sf::Vertex b;
    b.position = position;
    b.color = sf::Color::Yellow;
    a.append(b);

    sf::Vertex c;
    c.position = position + velocity * scale;
    c.color = sf::Color::Green;
    a.append(c);

    window.draw(a);
}

void Planet::setNearbyPlanets(const std::vector<Planet*>& planets) {
    // This method creates a reference to nearby planets for trajectory calculation
    // We don't actually need to store them since drawOrbitPath takes the planets vector directly
    // This is just to maintain API consistency with Rocket class
}
void Planet::drawOrbitPath(sf::RenderWindow& window, const std::vector<Planet*>& planets,
    float timeStep, int steps)
{
    // Create a vertex array for the trajectory line
    sf::VertexArray trajectoryLine(sf::PrimitiveType::LineStrip);

    // Start with current position and velocity
    sf::Vector2f simPos = position;
    sf::Vector2f simVel = velocity;

    // Add the starting point
    sf::Vertex startPoint;
    startPoint.position = simPos;
    startPoint.color = sf::Color(color.r, color.g, color.b, 100); // Semi-transparent version of planet color
    trajectoryLine.append(startPoint);

    // Simulate future positions
    for (int step = 0; step < steps; step++) {
        // Calculate gravitational forces from all planets
        sf::Vector2f totalForce(0, 0);

        for (const auto& otherPlanet : planets) {
            // Skip self
            if (otherPlanet == this) continue;

            sf::Vector2f direction = otherPlanet->getPosition() - simPos;
            float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

            // Skip if too close
            if (distance <= otherPlanet->getRadius() + GameConstants::TRAJECTORY_COLLISION_RADIUS) {
                // Stop the trajectory if we hit another planet
                break;
            }

            // Use same gravitational constant as in GravitySimulator
            const float gravityConstant = GameConstants::G;  // Use the constant from the header

            // FIX: Use mass() instead of trying to use simPos as mass
            float force = gravityConstant * otherPlanet->getMass() * this->getMass() / (distance * distance);

            // FIX: Calculate normalized direction vector for force
            sf::Vector2f normalizedDir = normalize(direction);

            // FIX: Calculate acceleration (force/mass)
            sf::Vector2f acceleration = normalizedDir * force / this->getMass();

            totalForce += acceleration;
        }

        // Update simulated velocity and position
        simVel += totalForce * timeStep;
        simPos += simVel * timeStep;

        // Calculate fade-out effect
        float alphaValue = 255 * (1.0f - static_cast<float>(step) / steps);

        // Use uint8_t instead of sf::Uint8
        sf::Color fadeColor(color.r, color.g, color.b, static_cast<uint8_t>(alphaValue));

        // Add point to trajectory
        sf::Vertex trajectoryPoint;
        trajectoryPoint.position = simPos;
        trajectoryPoint.color = fadeColor;
        trajectoryLine.append(trajectoryPoint);
    }

    // Draw the trajectory
    window.draw(trajectoryLine);
}