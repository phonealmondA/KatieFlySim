// Engine.cpp
#include "Engine.h"
#include "GameConstants.h"
#include <cmath>

Engine::Engine(sf::Vector2f relPos, float thrustPower, sf::Color col)
    : RocketPart(relPos, col), thrust(thrustPower)
{
    // Create engine shape (a simple triangle)
    // Create engine shape (a simple triangle)
    shape.setPointCount(3);
    shape.setPoint(0, { 0, -GameConstants::ROCKET_SIZE * 2 / 3 });
    shape.setPoint(1, { -GameConstants::ROCKET_SIZE / 3, GameConstants::ROCKET_SIZE * 2 / 3 });
    shape.setPoint(2, { GameConstants::ROCKET_SIZE / 3, GameConstants::ROCKET_SIZE * 2 / 3 });
    shape.setFillColor(color);
    shape.setOrigin({ 0, 0 });
}

void Engine::draw(sf::RenderWindow& window, sf::Vector2f rocketPos, float rotation, float scale, float thrustLevel, bool hasFuel)
{
    // Scale the shape based on the zoom level
    sf::ConvexShape scaledShape = shape;
    for (size_t i = 0; i < scaledShape.getPointCount(); i++) {
        sf::Vector2f point = shape.getPoint(i);
        scaledShape.setPoint(i, point * scale);
    }

    // Calculate actual position based on rocket position and rotation
    float radians = rotation * 3.14159f / 180.0f;
    float cosA = std::cos(radians);
    float sinA = std::sin(radians);

    // Rotate the relative position
    sf::Vector2f rotatedRelPos(
        relativePosition.x * cosA - relativePosition.y * sinA,
        relativePosition.x * sinA + relativePosition.y * cosA
    );

    scaledShape.setPosition(rocketPos + rotatedRelPos * scale);
    scaledShape.setRotation(sf::degrees(rotation));

    // Set the engine color based on thrust level and fuel status
    if (!hasFuel || thrustLevel < 0.001f) {
        // Engine off color (darker)
        scaledShape.setFillColor(sf::Color(100, 40, 0)); // Darker orange/red
    }
    else {
        // Engine on color - intensity based on thrust level
        int r = std::min(255, static_cast<int>(color.r + thrustLevel * 150));
        int g = std::min(255, static_cast<int>(color.g + thrustLevel * 20));
        int b = std::min(255, static_cast<int>(color.b));
        scaledShape.setFillColor(sf::Color(r, g, b));

        // Add flame effect when thrusting
        if (thrustLevel > 0.1f) {
            // Create a flame triangle that extends below the engine
            sf::ConvexShape flame;
            flame.setPointCount(3);

            // Flame size depends on thrust level
            float flameSize = GameConstants::ROCKET_SIZE * 1.5f * thrustLevel * scale;

            flame.setPoint(0, scaledShape.getPosition() + rotatedRelPos * scale);

            // Calculate flame base points (left and right sides of engine)
            sf::Vector2f flameBaseLeft = scaledShape.getPosition() +
                sf::Vector2f(-GameConstants::ROCKET_SIZE / 3 * cosA - GameConstants::ROCKET_SIZE * 2 / 3 * sinA,
                    -GameConstants::ROCKET_SIZE / 3 * sinA + GameConstants::ROCKET_SIZE * 2 / 3 * cosA) * scale;

            sf::Vector2f flameBaseRight = scaledShape.getPosition() +
                sf::Vector2f(GameConstants::ROCKET_SIZE / 3 * cosA - GameConstants::ROCKET_SIZE * 2 / 3 * sinA,
                    GameConstants::ROCKET_SIZE / 3 * sinA + GameConstants::ROCKET_SIZE * 2 / 3 * cosA) * scale;

            // Set flame triangle points
            flame.setPoint(1, flameBaseLeft);
            flame.setPoint(2, flameBaseRight);

            // Set flame color (bright orange-yellow)
            flame.setFillColor(sf::Color(255, 200 + thrustLevel * 55, 0, 200));

            // Draw the flame first (behind engine)
            window.draw(flame);
        }
    }

    // Draw the engine
    window.draw(scaledShape);
}

float Engine::getThrust() const
{
    return thrust;
}