#pragma once
#include "GameObject.h"
#include <vector>

class Planet : public GameObject {
private:
    sf::CircleShape a; // shape
    float b; // mass
    float c; // radius
    int d; // ownerId - which player created/owns this planet, -1 for none

public:
    Planet(sf::Vector2f pos, float radius, float mass, sf::Color color = sf::Color::Blue, int ownerId = -1);
    void setPosition(const sf::Vector2f& pos) { position = pos; }
    sf::Color getColor() const { return color; }
    void update(float deltaTime) override;
    void draw(sf::RenderWindow& window) override;

    void setNearbyPlanets(const std::vector<Planet*>& planets);

    float getMass() const;
    float getRadius() const;
    int getOwnerId() const { return d; }
    void setOwnerId(int id) { d = id; }

    // Methods for dynamic radius
    void setMass(float newMass);
    void updateRadiusFromMass();

    // Draw velocity vector for the planet
    void drawVelocityVector(sf::RenderWindow& window, float scale = 1.0f);

    // Draw predicted orbit path
    void drawOrbitPath(sf::RenderWindow& window, const std::vector<Planet*>& planets, float timeStep = 0.5f, int steps = 2000);
};