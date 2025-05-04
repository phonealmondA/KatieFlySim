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
    sf::VertexArray a(sf::PrimitiveType::LineStrip);

    // Start with current position and velocity
    sf::Vector2f b = position;
    sf::Vector2f c = velocity;

    // Add the starting point
    sf::Vertex d;
    d.position = b;
    d.color = sf::Color(color.r, color.g, color.b, 100); // Semi-transparent version of planet color
    a.append(d);

    // Simulate future positions
    for (int e = 0; e < steps; e++) {
        // Calculate gravitational forces from all planets
        sf::Vector2f f(0, 0);

        for (const auto& g : planets) {
            // Skip self
            if (g == this) continue;

            sf::Vector2f h = g->getPosition() - b;
            float i = std::sqrt(h.x * h.x + h.y * h.y);

            // Skip if too close
            if (i <= g->getRadius() + c + GameConstants::TRAJECTORY_COLLISION_RADIUS) {
                // Stop the trajectory if we hit another planet
                break;
            }

            // Use same gravitational constant as in GravitySimulator
            const float j = GameConstants::G;  // Use the constant from the header
            float k = j * g->getMass() * b / (i * i);

            sf::Vector2f l = normalize(h) * k / b;
            f += l;
        }

        // Update simulated velocity and position
        c += f * timeStep;
        b += c * timeStep;

        // Calculate fade-out effect
        float g = 255 * (1.0f - static_cast<float>(e) / steps);
        // Use uint8_t instead of sf::Uint8
        sf::Color h(color.r, color.g, color.b, static_cast<uint8_t>(g));

        // Add point to trajectory
        sf::Vertex i;
        i.position = b;
        i.color = h;
        a.append(i);
    }

    // Draw the trajectory
    window.draw(a);
}