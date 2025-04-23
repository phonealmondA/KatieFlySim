// OrbitalMechanics.h
#pragma once
#include <SFML/System/Vector2.hpp>

namespace OrbitalMechanics {
    // Calculate apoapsis (furthest point in orbit) from relative position and velocity
    float calculateApoapsis(sf::Vector2f pos, sf::Vector2f vel, float planetMass, float G);

    // Calculate periapsis (closest point in orbit) from relative position and velocity
    float calculatePeriapsis(sf::Vector2f pos, sf::Vector2f vel, float planetMass, float G);

    // Calculate orbital period in seconds
    float calculateOrbitalPeriod(float semimajorAxis, float planetMass, float G);

    // Calculate semi-major axis from specific orbital energy
    float calculateSemimajorAxis(float energy, float planetMass, float G);

    // Calculate eccentricity vector
    sf::Vector2f calculateEccentricityVector(sf::Vector2f pos, sf::Vector2f vel, float planetMass, float G);

    // Calculate eccentricity (scalar)
    float calculateEccentricity(sf::Vector2f pos, sf::Vector2f vel, float planetMass, float G);

    // Calculate specific orbital energy
    float calculateOrbitalEnergy(sf::Vector2f pos, sf::Vector2f vel, float planetMass, float G);
}