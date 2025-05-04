// GameServer.h
#pragma once
#include "GravitySimulator.h"
#include "VehicleManager.h"
#include "GameState.h"
#include "PlayerInput.h"
#include <vector>
#include <map>

class GameServer {
private:
    GravitySimulator a; // simulator
    std::vector<Planet*> b; // planets
    std::map<int, VehicleManager*> c; // players
    unsigned long d; // sequenceNumber
    float e; // gameTime

    // New members for distributed simulation
    std::map<int, GameState> f; // clientSimulations - stores each client's simulation state
    std::map<int, float> g; // lastClientUpdateTime - when each client last sent their simulation
    std::map<int, bool> h; // clientSimulationValid - whether each client's simulation is valid
    float i; // validationThreshold - how much difference is allowed before correcting client

public:
    GameServer();
    ~GameServer();

    void initialize();
    void update(float deltaTime);
    void handlePlayerInput(int playerId, const PlayerInput& input);
    GameState getGameState() const;

    // New methods for distributed simulation
    void processClientSimulation(int playerId, const GameState& clientState);
    GameState validateClientSimulation(int playerId, const GameState& clientState);
    void synchronizeState();
    void setValidationThreshold(float threshold) { i = threshold; }
    float getValidationThreshold() const { return i; }

    int addPlayer(int playerId, sf::Vector2f initialPos, sf::Color color = sf::Color::White);
    void removePlayer(int playerId);

    const std::vector<Planet*>& getPlanets() const { return b; }
    VehicleManager* getPlayer(int playerId) {
        auto it = c.find(playerId);
        return (it != c.end()) ? it->second : nullptr;
    }
    const std::map<int, VehicleManager*>& getPlayers() const {
        return c;
    }
};