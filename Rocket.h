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
    sf::ConvexShape a; // body
    std::vector<std::unique_ptr<RocketPart>> b; // parts
    float c; // rotation
    float d; // angularVelocity
    float e; // thrustLevel - Current thrust level (0.0 to 1.0)
    std::vector<Planet*> f; // nearbyPlanets
    float g; // mass - Added mass property for physics calculations
    float h; // storedMass - Mass taken from planets that can be transferred back
    float i; // fuelConsumptionRate - Mass consumed per second at full thrust
    sf::CircleShape j; // storedMassVisual - Visual representation of stored mass
    float k; // thrustMultiplier - Multiplier for engine thrust (starts at 1.0)
    float l; // efficiencyMultiplier - Multiplier for fuel efficiency (starts at 1.0)
    bool m; // isThrusting - Flag to track when thrust is actually being applied
    int n; // ownerId - which player owns/controls this rocket
    float o; // lastStateTimestamp - when the rocket state was last updated

    void updateStoredMassVisual();
    bool checkCollision(const Planet& planet);

public:
    Rocket(sf::Vector2f pos, sf::Vector2f vel, sf::Color col = sf::Color::White, float m = 1.0f, int ownerId = -1);

    void addPart(std::unique_ptr<RocketPart> part);
    void applyThrust(float amount);
    void rotate(float amount);
    void setThrustLevel(float level); // Set thrust level between 0.0 and 1.0
    bool isColliding(const Planet& planet);

    // Modified to include null checks
    void setNearbyPlanets(const std::vector<Planet*>& planets);
    const std::vector<Planet*>& getNearbyPlanets() const { return f; }

    void setPosition(sf::Vector2f pos) { position = pos; }
    Rocket* mergeWith(Rocket* other);

    // Ownership methods
    int getOwnerId() const { return n; }
    void setOwnerId(int id) { n = id; }

    // State timing methods
    float getLastStateTimestamp() const { return o; }
    void setLastStateTimestamp(float timestamp) { o = timestamp; }

    // Mass related methods
    float getMass() const { return g; }
    float getStoredMass() const { return h; }
    void addStoredMass(float amount);
    Planet* dropStoredMass(); // New method to drop stored mass as a planet

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
    float getThrustLevel() const { return e; }
    const std::vector<std::unique_ptr<RocketPart>>& getParts() const { return b; }
    float getRotation() const { return c; }
    void setRotation(float rot) { c = rot; }

    // Engine upgrade methods
    bool upgradeThrust(float massCost);
    bool upgradeEfficiency(float massCost);

    // Getters for upgrade values
    float getThrustMultiplier() const { return k; }
    float getEfficiencyMultiplier() const { return l; }

    void setColor(sf::Color col) { color = col; }
    sf::Color getColor() const { return color; }

    // Serialization helper - create RocketState from this rocket
    RocketState createState() const;
    // Apply state to this rocket
    void applyState(const RocketState& state);
};