// PlayerInput.h
#pragma once
#include <SFML/Network.hpp>

struct PlayerInput {
    int a; // playerId
    bool b; // thrustForward
    bool c; // thrustBackward
    bool d; // rotateLeft
    bool e; // rotateRight
    bool f; // switchVehicle
    float g; // thrustLevel
    float h; // deltaTime
    float i; // clientTimestamp - when the client generated this input
    float j; // lastServerStateTimestamp - the timestamp of the last state the client had when generating this input

    // Additional member for the distributed simulation
    RocketState k; // clientRocketState - the client's current rocket state 

    // Default constructor
    PlayerInput() : a(0), b(false), c(false),
        d(false), e(false), f(false),
        g(0.0f), h(0.0f), i(0.0f), j(0.0f) {
    }

    // Packet operators for serialization
    friend sf::Packet& operator <<(sf::Packet& packet, const PlayerInput& input);
    friend sf::Packet& operator >>(sf::Packet& packet, PlayerInput& input);
};

// Option 1: Keep the implementations in the header as inline
inline sf::Packet& operator <<(sf::Packet& packet, const PlayerInput& input) {
    return packet << input.a
        << input.b << input.c
        << input.d << input.e
        << input.f << input.g
        << input.h << input.i
        << input.j << input.k;
}

inline sf::Packet& operator >>(sf::Packet& packet, PlayerInput& input) {
    return packet >> input.a
        >> input.b >> input.c
        >> input.d >> input.e
        >> input.f >> input.g
        >> input.h >> input.i
        >> input.j >> input.k;
}