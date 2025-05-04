// GravitySimulator.h
#pragma once
#include "Planet.h"
#include "Rocket.h"
#include "VectorHelper.h"
#include "GameConstants.h"  // Include the constants
#include <vector>

// Forward declaration
class VehicleManager;

class GravitySimulator {
private:
    std::vector<Planet*> a; // planets
    std::vector<Rocket*> b; // rockets
    VehicleManager* c; // vehicleManager
    const float d; // G - Use the constant from the header
    bool e; // simulatePlanetGravity
    int f; // ownerId - for limiting simulation to owned objects

public:
    GravitySimulator(int ownerId = -1);

    void addPlanet(Planet* planet);
    void addRocket(Rocket* rocket);
    void addVehicleManager(VehicleManager* manager) { c = manager; }
    void update(float deltaTime);
    void clearRockets();
    void addRocketGravityInteractions(float deltaTime);
    void checkPlanetCollisions();
    const std::vector<Planet*>& getPlanets() const { return a; }
    void setSimulatePlanetGravity(bool enable) { e = enable; }
    int getOwnerId() const { return f; }

    // Only simulate physics for planets/rockets owned by this simulator's owner
    void setOwnerId(int id) { f = id; }
    bool shouldSimulateObject(int objectOwnerId) const;

    // Only declare the method, don't implement it here
    void updateVehicleManagerPlanets();

    void removeVehicleManager(VehicleManager* manager) { if (c == manager) { c = nullptr; } }
};