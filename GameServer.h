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
    GravitySimulator simulator;
    std::vector<Planet*> planets;
    std::map<int, VehicleManager*> players;
    unsigned long sequenceNumber;
    float gameTime;

public:
    GameServer();
    ~GameServer();

    void initialize();
    void update(float deltaTime);
    void handlePlayerInput(int playerId, const PlayerInput& input);
    GameState getGameState() const;

    int addPlayer(int playerId, sf::Vector2f initialPos, sf::Color color = sf::Color::White);
    void removePlayer(int playerId);

    const std::vector<Planet*>& getPlanets() const { return planets; }
    VehicleManager* getPlayer(int playerId) {
        auto it = players.find(playerId);
        return (it != players.end()) ? it->second : nullptr;
    }
    const std::map<int, VehicleManager*>& getPlayers() const {
        return players;
    }
};