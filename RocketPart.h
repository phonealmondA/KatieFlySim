// RocketPart.h
#pragma once
#include <SFML/Graphics.hpp>

class RocketPart {
protected:
    sf::Vector2f relativePosition; // Position relative to rocket center
    sf::Color color;

public:
    RocketPart(sf::Vector2f relPos, sf::Color col);
    virtual ~RocketPart() = default;

    virtual void draw(sf::RenderWindow& window, sf::Vector2f rocketPos, float rotation,
        float scale = 1.0f, float thrustLevel = 0.0f, bool hasFuel = true) = 0;
};