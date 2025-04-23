// OrbitalMechanics.cpp
#include "OrbitalMechanics.h"
#include <cmath>

namespace OrbitalMechanics {

    float calculateApoapsis(sf::Vector2f pos, sf::Vector2f vel, float planetMass, float G) {
        // Calculate specific orbital energy
        float speed = std::sqrt(vel.x * vel.x + vel.y * vel.y);
        float distance = std::sqrt(pos.x * pos.x + pos.y * pos.y);
        float energy = 0.5f * speed * speed - G * planetMass / distance;

        // Calculate semi-major axis
        float semiMajor = -G * planetMass / (2 * energy);

        // If energy is positive, orbit is hyperbolic (no apoapsis)
        if (energy >= 0) return -1.0f;

        // Calculate eccentricity vector
        sf::Vector2f eVec;
        float vSquared = speed * speed;
        eVec.x = (vSquared * pos.x - (pos.x * vel.x + pos.y * vel.y) * vel.x) / (G * planetMass) - pos.x / distance;
        eVec.y = (vSquared * pos.y - (pos.x * vel.x + pos.y * vel.y) * vel.y) / (G * planetMass) - pos.y / distance;
        float ecc = std::sqrt(eVec.x * eVec.x + eVec.y * eVec.y);

        // Calculate apoapsis
        return semiMajor * (1 + ecc);
    }

    float calculatePeriapsis(sf::Vector2f pos, sf::Vector2f vel, float planetMass, float G) {
        // Calculate specific orbital energy
        float speed = std::sqrt(vel.x * vel.x + vel.y * vel.y);
        float distance = std::sqrt(pos.x * pos.x + pos.y * pos.y);
        float energy = 0.5f * speed * speed - G * planetMass / distance;

        // Calculate semi-major axis
        float semiMajor = -G * planetMass / (2 * energy);

        // If energy is positive, orbit is hyperbolic
        if (energy >= 0) return -1.0f;

        // Calculate eccentricity vector
        sf::Vector2f eVec;
        float vSquared = speed * speed;
        eVec.x = (vSquared * pos.x - (pos.x * vel.x + pos.y * vel.y) * vel.x) / (G * planetMass) - pos.x / distance;
        eVec.y = (vSquared * pos.y - (pos.x * vel.x + pos.y * vel.y) * vel.y) / (G * planetMass) - pos.y / distance;
        float ecc = std::sqrt(eVec.x * eVec.x + eVec.y * eVec.y);

        // Calculate periapsis
        return semiMajor * (1 - ecc);
    }

    float calculateOrbitalPeriod(float semimajorAxis, float planetMass, float G) {
        // Kepler's Third Law: T^2 ? a^3
        // T = 2??(a^3/?) where ? = GM
        if (semimajorAxis <= 0) return -1.0f; // Not a closed orbit

        return 2.0f * 3.14159f * std::sqrt(std::pow(semimajorAxis, 3) / (G * planetMass));
    }

    float calculateSemimajorAxis(float energy, float planetMass, float G) {
        // a = -?/(2E) where ? = GM
        if (energy >= 0) return -1.0f; // Not a closed orbit

        return -G * planetMass / (2.0f * energy);
    }

    sf::Vector2f calculateEccentricityVector(sf::Vector2f pos, sf::Vector2f vel, float planetMass, float G) {
        float speed = std::sqrt(vel.x * vel.x + vel.y * vel.y);
        float distance = std::sqrt(pos.x * pos.x + pos.y * pos.y);
        float vSquared = speed * speed;

        sf::Vector2f eVec;
        eVec.x = (vSquared * pos.x - (pos.x * vel.x + pos.y * vel.y) * vel.x) / (G * planetMass) - pos.x / distance;
        eVec.y = (vSquared * pos.y - (pos.x * vel.x + pos.y * vel.y) * vel.y) / (G * planetMass) - pos.y / distance;

        return eVec;
    }

    float calculateEccentricity(sf::Vector2f pos, sf::Vector2f vel, float planetMass, float G) {
        sf::Vector2f eVec = calculateEccentricityVector(pos, vel, planetMass, G);
        return std::sqrt(eVec.x * eVec.x + eVec.y * eVec.y);
    }

    float calculateOrbitalEnergy(sf::Vector2f pos, sf::Vector2f vel, float planetMass, float G) {
        float speed = std::sqrt(vel.x * vel.x + vel.y * vel.y);
        float distance = std::sqrt(pos.x * pos.x + pos.y * pos.y);

        // E = v²/2 - ?/r
        return 0.5f * speed * speed - G * planetMass / distance;
    }

} // namespace OrbitalMechanics