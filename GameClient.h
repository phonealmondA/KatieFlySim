// GameClient.h (improved file)
#pragma once
#include "GravitySimulator.h"
#include "VehicleManager.h"
#include "GameState.h"
#include "PlayerInput.h"
#include <vector>
#include <map>

// Struct to store interpolation data for remote players
struct RemotePlayerState {
    sf::Vector2f startPos;
    sf::Vector2f startVel;
    sf::Vector2f targetPos;
    sf::Vector2f targetVel;
    float rotation;
    float timestamp;
};

class GameClient {
private:
    GravitySimulator simulator;
    std::vector<Planet*> planets;
    std::map<int, VehicleManager*> remotePlayers;
    VehicleManager* localPlayer;
    int localPlayerId;

    // Last received state for interpolation
    GameState lastState;
    float stateTimestamp;

    // Interpolation data for remote players
    std::map<int, RemotePlayerState> remotePlayerStates;
    float latencyCompensation; // Time window for interpolation

public:
    GameClient();
    ~GameClient();

    void initialize();
    void update(float deltaTime);
    void processGameState(const GameState& state);
    PlayerInput getLocalPlayerInput(float deltaTime) const;

    // Apply input locally for responsive control
    void applyLocalInput(const PlayerInput& input);

    // Interpolate remote players between received states
    void interpolateRemotePlayers(float currentTime);

    // Set latency compensation window
    void setLatencyCompensation(float value);

    void setLocalPlayerId(int id) { localPlayerId = id; }
    int getLocalPlayerId() const { return localPlayerId; }

    VehicleManager* getLocalPlayer() { return localPlayer; }
    const std::vector<Planet*>& getPlanets() const { return planets; }
    const std::map<int, VehicleManager*>& getRemotePlayers() const { return remotePlayers; }
};