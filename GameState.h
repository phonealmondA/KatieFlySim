// GameState.h
#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <SFML/Network.hpp>

// Serializable state for a rocket
struct RocketState {
    int playerId;
    sf::Vector2f position;
    sf::Vector2f velocity;
    float rotation;
    float angularVelocity;
    float thrustLevel;
    float mass;
    sf::Color color;

    // Packet operators for serialization
    friend sf::Packet& operator <<(sf::Packet& packet, const RocketState& state);
    friend sf::Packet& operator >>(sf::Packet& packet, RocketState& state);
};

// Serializable state for a planet
struct PlanetState {
    int planetId;
    sf::Vector2f position;
    sf::Vector2f velocity;
    float mass;
    float radius;
    sf::Color color;

    // Packet operators for serialization
    friend sf::Packet& operator <<(sf::Packet& packet, const PlanetState& state);
    friend sf::Packet& operator >>(sf::Packet& packet, PlanetState& state);
};

// Complete game state for synchronization
struct GameState {
    unsigned long sequenceNumber;
    float timestamp;
    std::vector<RocketState> rockets;
    std::vector<PlanetState> planets;

    // Packet operators for serialization
    friend sf::Packet& operator <<(sf::Packet& packet, const GameState& state);
    friend sf::Packet& operator >>(sf::Packet& packet, GameState& state);
};