// Rocket.h
#pragma once
#include "GameObject.h"
#include "GameConstants.h"
#include "RocketPart.h"
#include "Engine.h"
#include "Planet.h"
#include <vector>
#include <memory>
#include <iostream>

class Rocket : public GameObject {
private:
    sf::ConvexShape body;
    std::vector<std::unique_ptr<RocketPart>> parts;
    float rotation;
    float angularVelocity;
    float thrustLevel; // Current thrust level (0.0 to 1.0)
    std::vector<Planet*> nearbyPlanets;
    float mass; // Added mass property for physics calculations
    float storedMass; // Mass taken from planets that can be transferred back
    float fuelConsumptionRate; // Mass consumed per second at full thrust
    sf::CircleShape storedMassVisual; // Visual representation of stored mass


    float thrustMultiplier;     // Multiplier for engine thrust (starts at 1.0)
    float efficiencyMultiplier; // Multiplier for fuel efficiency (starts at 1.0)

    void updateStoredMassVisual();
    bool checkCollision(const Planet& planet);

public:
    Rocket(sf::Vector2f pos, sf::Vector2f vel, sf::Color col = sf::Color::White, float m = 1.0f);

    void addPart(std::unique_ptr<RocketPart> part);
    void applyThrust(float amount);
    void rotate(float amount);
    void setThrustLevel(float level); // Set thrust level between 0.0 and 1.0
    bool isColliding(const Planet& planet);

    // Modified to include null checks
    void setNearbyPlanets(const std::vector<Planet*>& planets);
    const std::vector<Planet*>& getNearbyPlanets() const { return nearbyPlanets; }

    void setPosition(sf::Vector2f pos) { position = pos; }
    Rocket* mergeWith(Rocket* other);

    // Add getter for mass
    float getMass() const { return mass; }

    // Add storedMass methods
    float getStoredMass() const { return storedMass; }
    void addStoredMass(float amount);

    // New method to drop stored mass as a planet
    Planet* dropStoredMass();

    // Fuel consumption methods
    void consumeFuel(float deltaTime);
    bool hasFuel() const;

    void update(float deltaTime) override;
    void draw(sf::RenderWindow& window) override;
    void drawWithConstantSize(sf::RenderWindow& window, float zoomLevel);

    // Draw velocity vector line
    void drawVelocityVector(sf::RenderWindow& window, float scale = GameConstants::VELOCITY_VECTOR_SCALE);

    // New method to draw gravity force vectors
    void drawGravityForceVectors(sf::RenderWindow& window, const std::vector<Planet*>& planets, float scale = 1.0f);

    void drawTrajectory(sf::RenderWindow& window, const std::vector<Planet*>& planets,
        float timeStep = 0.5f, int steps = 200, bool detectSelfIntersection = false);
    float getThrustLevel() const { return thrustLevel; }
    const std::vector<std::unique_ptr<RocketPart>>& getParts() const { return parts; }
    float getRotation() const { return rotation; }
    void setRotation(float rot) { rotation = rot; }



    // Engine upgrade methods
    bool upgradeThrust(float massCost);
    bool upgradeEfficiency(float massCost);

    // Getters for upgrade values
    float getThrustMultiplier() const { return thrustMultiplier; }
    float getEfficiencyMultiplier() const { return efficiencyMultiplier; }

    void setColor(sf::Color col) { color = col; }
    sf::Color getColor() const { return color; }
};