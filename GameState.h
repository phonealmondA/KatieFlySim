// GameState.h
#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <SFML/Network.hpp>

// Serializable state for a rocket
struct RocketState {
    int a; // playerId
    sf::Vector2f b; // position
    sf::Vector2f c; // velocity
    float d; // rotation
    float e; // angularVelocity
    float f; // thrustLevel
    float g; // mass
    sf::Color h; // color
    float i; // timestamp of this state
    bool j; // isAuthoritative - whether this is the definitive state from server

    // Packet operators for serialization
    friend sf::Packet& operator <<(sf::Packet& packet, const RocketState& state);
    friend sf::Packet& operator >>(sf::Packet& packet, RocketState& state);
};

// Serializable state for a planet
struct PlanetState {
    int a; // planetId
    sf::Vector2f b; // position
    sf::Vector2f c; // velocity
    float d; // mass
    float e; // radius
    sf::Color f; // color
    int g; // ownerId - which player owns/created this planet
    float h; // timestamp of this state

    // Packet operators for serialization
    friend sf::Packet& operator <<(sf::Packet& packet, const PlanetState& state);
    friend sf::Packet& operator >>(sf::Packet& packet, PlanetState& state);
};

// Complete game state for synchronization
struct GameState {
    unsigned long a; // sequenceNumber
    float b; // timestamp
    std::vector<RocketState> c; // rockets
    std::vector<PlanetState> d; // planets
    bool e; // isInitialState - if this is the first state sent to a client

    // Packet operators for serialization
    friend sf::Packet& operator <<(sf::Packet& packet, const GameState& state);
    friend sf::Packet& operator >>(sf::Packet& packet, GameState& state);
};