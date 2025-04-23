// Car.cpp
#include "Car.h"
#include "Rocket.h"
#include "VectorHelper.h"
#include "GameConstants.h"
#include <cmath>
#include <float.h>

Car::Car(sf::Vector2f pos, sf::Vector2f vel, sf::Color col)
    : GameObject(pos, vel, col), rotation(0), speed(0), maxSpeed(200.0f),
    currentPlanet(nullptr), isGrounded(false), isFacingRight(true)
{
    // Create car body (small box)
    body.setSize({ GameConstants::CAR_BODY_WIDTH, GameConstants::CAR_BODY_HEIGHT });
    body.setFillColor(color);
    body.setOrigin({ GameConstants::CAR_BODY_WIDTH / 2, GameConstants::CAR_BODY_HEIGHT / 2 });
    body.setPosition(position);
    body.setScale(sf::Vector2f(1.0f, 1.0f));  // Changed to Vector2f

    // Create wheels
    for (int i = 0; i < 2; i++) {
        wheels[i].setRadius(GameConstants::CAR_WHEEL_RADIUS);
        wheels[i].setFillColor(sf::Color::Black);
        wheels[i].setOrigin({ GameConstants::CAR_WHEEL_RADIUS, GameConstants::CAR_WHEEL_RADIUS });
    }

    // Create direction arrow
    directionArrow.setPointCount(3);
    directionArrow.setPoint(0, { 10.0f, 0.0f });
    directionArrow.setPoint(1, { 0.0f, -5.0f });
    directionArrow.setPoint(2, { 0.0f, 5.0f });
    directionArrow.setFillColor(sf::Color::Red);
    directionArrow.setOrigin({ 0.0f, 0.0f });
}

void Car::accelerate(float amount) {
    if (isGrounded) {
        // Apply acceleration in the direction the car is facing
        if ((amount > 0 && isFacingRight) || (amount < 0 && !isFacingRight)) {
            speed += std::abs(amount) * 10.0f;
        }
        else {
            speed -= std::abs(amount) * 10.0f;
        }

        // Clamp speed
        speed = std::min(speed, maxSpeed);
        speed = std::max(speed, -maxSpeed / 2.0f); // Slower in reverse
    }
}

void Car::rotate(float amount) {
    if (isGrounded) {
        // Instead of changing rotation value, flip the car direction
        if ((amount < 0 && isFacingRight) || (amount > 0 && !isFacingRight)) {
            isFacingRight = !isFacingRight;
        }
    }
}

void Car::checkGrounding(const std::vector<Planet*>& planets) {
    isGrounded = false;
    float closestDistance = FLT_MAX;
    currentPlanet = nullptr;

    for (const auto& planet : planets) {
        sf::Vector2f direction = position - planet->getPosition();
        float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

        // Check if we're at or very close to the surface
        if (distance <= (planet->getRadius() + GameConstants::ROCKET_SIZE) && distance < closestDistance) {
            closestDistance = distance;
            currentPlanet = const_cast<Planet*>(planet);
            isGrounded = true;
        }
    }
}

void Car::update(float deltaTime) {
    if (isGrounded && currentPlanet) {
        // Get direction to planet center (gravity normal)
        sf::Vector2f toPlanet = currentPlanet->getPosition() - position;
        float distToPlanet = std::sqrt(toPlanet.x * toPlanet.x + toPlanet.y * toPlanet.y);
        sf::Vector2f normal = toPlanet / distToPlanet;

        // Calculate tangent direction (perpendicular to normal)
        // Always use the same formula for tangent regardless of direction
        sf::Vector2f tangent(-normal.y, normal.x);

        // Calculate angle for car orientation (align with surface)
        rotation = std::atan2(-normal.x, normal.y) * 180.0f / 3.14159f;

        // Adjust visual orientation based on direction
        if (!isFacingRight) {
            // Only flip the visual representation, not the movement direction
            body.setScale(sf::Vector2f(-1.0f, 1.0f));  // Changed to Vector2f
            directionArrow.setScale(sf::Vector2f(-1.0f, 1.0f));  // Changed to Vector2f
        }
        else {
            body.setScale(sf::Vector2f(1.0f, 1.0f));  // Changed to Vector2f
            directionArrow.setScale(sf::Vector2f(1.0f, 1.0f));  // Changed to Vector2f
        }

        // Move along surface based on facing direction
        float speedWithDirection = isFacingRight ? speed : -speed;
        position += tangent * speedWithDirection * deltaTime;

        // Always place car at exact distance from planet surface
        position = currentPlanet->getPosition() + normal * (currentPlanet->getRadius() + GameConstants::TRAJECTORY_COLLISION_RADIUS);

        // Apply friction
        speed *= 0.98f;
    }
    else {
        // If in air, apply simple physics (fall with gravity)
        position += velocity * deltaTime;
    }

    // Update visual components
    body.setPosition(position);
    body.setRotation(sf::degrees(rotation));

    // Update wheels position based on body orientation
    float wheelOffset = GameConstants::CAR_BODY_WIDTH / 2 - GameConstants::CAR_WHEEL_RADIUS;
    float radians = rotation * 3.14159f / 180.0f;
    float cos_val = std::cos(radians);
    float sin_val = std::sin(radians);

    wheels[0].setPosition(position + sf::Vector2f(-wheelOffset * cos_val, -wheelOffset * sin_val));
    wheels[1].setPosition(position + sf::Vector2f(wheelOffset * cos_val, wheelOffset * sin_val));

    // Update arrow position
    float arrowOffset = GameConstants::CAR_BODY_WIDTH / 2 + 5.0f;
    directionArrow.setPosition(position + sf::Vector2f(arrowOffset * cos_val, arrowOffset * sin_val));
    directionArrow.setRotation(sf::degrees(rotation));
}

void Car::draw(sf::RenderWindow& window) {
    window.draw(body);
    window.draw(wheels[0]);
    window.draw(wheels[1]);
    window.draw(directionArrow);
}

void Car::drawWithConstantSize(sf::RenderWindow& window, float zoomLevel) {
    // Scale all components based on zoom level
    sf::RectangleShape scaledBody = body;
    scaledBody.setSize(body.getSize() * zoomLevel);
    scaledBody.setOrigin(body.getOrigin() * zoomLevel);
    scaledBody.setPosition(position);
    scaledBody.setRotation(sf::degrees(rotation));
    scaledBody.setScale(body.getScale());

    sf::CircleShape scaledWheels[2];
    for (int i = 0; i < 2; i++) {
        scaledWheels[i] = wheels[i];
        scaledWheels[i].setRadius(wheels[i].getRadius() * zoomLevel);
        scaledWheels[i].setOrigin(wheels[i].getOrigin() * zoomLevel);
        scaledWheels[i].setPosition(wheels[i].getPosition());
    }

    sf::ConvexShape scaledArrow = directionArrow;
    for (size_t i = 0; i < scaledArrow.getPointCount(); i++) {
        scaledArrow.setPoint(i, directionArrow.getPoint(i) * zoomLevel);
    }
    scaledArrow.setPosition(directionArrow.getPosition());
    scaledArrow.setRotation(directionArrow.getRotation());
    scaledArrow.setScale(directionArrow.getScale());

    window.draw(scaledBody);
    window.draw(scaledWheels[0]);
    window.draw(scaledWheels[1]);
    window.draw(scaledArrow);
}

void Car::initializeFromRocket(const Rocket* rocket) {
    position = rocket->getPosition();
    velocity = rocket->getVelocity() * GameConstants::TRANSFORM_VELOCITY_FACTOR;
    speed = 0.0f; // Start with zero speed

    // Initialize direction (facing right by default)
    isFacingRight = true;

    // Check if we're near a planet to align with its surface
    checkGrounding(rocket->getNearbyPlanets());

    if (currentPlanet) {
        sf::Vector2f toPlanet = currentPlanet->getPosition() - position;
        float distToPlanet = std::sqrt(toPlanet.x * toPlanet.x + toPlanet.y * toPlanet.y);
        sf::Vector2f normal = toPlanet / distToPlanet;

        // Calculate angle for proper orientation on the surface
        rotation = std::atan2(-normal.x, normal.y) * 180.0f / 3.14159f;

        // Ensure consistent distance from planet surface
        position = currentPlanet->getPosition() + normal * (currentPlanet->getRadius() + GameConstants::TRAJECTORY_COLLISION_RADIUS);
    }
}