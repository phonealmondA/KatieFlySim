// GameState.cpp
#include "GameState.h"

// Implement serialization for sf::Vector2f
sf::Packet& operator<<(sf::Packet& packet, const sf::Vector2f& vector) {
    return packet << vector.x << vector.y;
}

sf::Packet& operator>>(sf::Packet& packet, sf::Vector2f& vector) {
    return packet >> vector.x >> vector.y;
}

// Implement serialization for sf::Color
sf::Packet& operator<<(sf::Packet& packet, const sf::Color& color) {
    return packet << color.r << color.g << color.b << color.a;
}

sf::Packet& operator>>(sf::Packet& packet, sf::Color& color) {
    return packet >> color.r >> color.g >> color.b >> color.a;
}

// Implement RocketState serialization
sf::Packet& operator<<(sf::Packet& packet, const RocketState& state) {
    return packet << state.a << state.b << state.c
        << state.d << state.e << state.f
        << state.g << state.h << state.i << state.j;
}

sf::Packet& operator>>(sf::Packet& packet, RocketState& state) {
    return packet >> state.a >> state.b >> state.c
        >> state.d >> state.e >> state.f
        >> state.g >> state.h >> state.i >> state.j;
}

// Implement PlanetState serialization
sf::Packet& operator<<(sf::Packet& packet, const PlanetState& state) {
    return packet << state.a << state.b << state.c
        << state.d << state.e << state.f << state.g << state.h;
}

sf::Packet& operator>>(sf::Packet& packet, PlanetState& state) {
    return packet >> state.a >> state.b >> state.c
        >> state.d >> state.e >> state.f >> state.g >> state.h;
}

// Implement GameState serialization
sf::Packet& operator<<(sf::Packet& packet, const GameState& state) {
    packet << static_cast<uint32_t>(state.a) << state.b << state.e;

    // Serialize rockets
    packet << static_cast<uint32_t>(state.c.size());
    for (const auto& a : state.c) {
        packet << a;
    }

    // Serialize planets
    packet << static_cast<uint32_t>(state.d.size());
    for (const auto& a : state.d) {
        packet << a;
    }

    return packet;
}

sf::Packet& operator>>(sf::Packet& packet, GameState& state) {
    uint32_t a;
    packet >> a >> state.b >> state.e;
    state.a = a;

    // Deserialize rockets
    uint32_t b;
    packet >> b;
    state.c.resize(b);
    for (uint32_t a = 0; a < b; ++a) {
        packet >> state.c[a];
    }

    // Deserialize planets
    uint32_t c;
    packet >> c;
    state.d.resize(c);
    for (uint32_t a = 0; a < c; ++a) {
        packet >> state.d[a];
    }

    return packet;
}