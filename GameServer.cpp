// GameServer.cpp
#include "GameServer.h"
#include "GameConstants.h"
#include <iostream> 

GameServer::GameServer() : d(0), e(0.0f), i(0.1f) {
}

GameServer::~GameServer() {
    // Clean up players
    for (auto& a : c) {
        delete a.second;
    }
    c.clear();

    // Clean up planets
    for (auto& a : b) {
        delete a;
    }
    b.clear();
}

void GameServer::initialize() {
    // Create main planet (sun)
    Planet* a = new Planet(
        sf::Vector2f(GameConstants::MAIN_PLANET_X, GameConstants::MAIN_PLANET_Y),
        0, GameConstants::MAIN_PLANET_MASS, sf::Color::Yellow);
    a->setVelocity(sf::Vector2f(1.f, -1.f));
    b.push_back(a);

    // Create 9 orbiting planets with different orbital distances, sizes, and colors
    const float b = GameConstants::SECONDARY_PLANET_MASS;
    const sf::Color c[] = {
        sf::Color(150, 150, 150),   // Mercury (gray)
        sf::Color(255, 190, 120),   // Venus (light orange)
        sf::Color(0, 100, 255),     // Earth (blue)
        sf::Color(255, 100, 0),     // Mars (red)
        sf::Color(255, 200, 100),   // Jupiter (light orange)
        sf::Color(230, 180, 80),    // Saturn (tan)
        sf::Color(180, 230, 230),   // Uranus (light blue)
        sf::Color(100, 130, 255),   // Neptune (dark blue)
        sf::Color(230, 230, 230)    // Pluto (light gray)
    };

    // Distance and mass scaling factors
    const float d[] = { 0.4f, 0.7f, 1.0f, 1.5f, 2.2f, 3.0f, 4.0f, 5.0f, 6.0f };
    const float e[] = { 0.1f, 0.8f, 1.0f, 0.5f, 11.0f, 9.5f, 4.0f, 3.8f, 0.05f };

    // Create each planet
    for (int f = 0; f < 9; f++) {
        float g = GameConstants::PLANET_ORBIT_DISTANCE * d[f];
        float h = (f * 40.0f) * (3.14159f / 180.0f); // Distribute planets around the sun

        // Calculate position based on orbit distance and angle
        float i = a->getPosition().x + g * cos(h);
        float j = a->getPosition().y + g * sin(h);

        // Calculate orbital velocity for a circular orbit
        float k = std::sqrt(GameConstants::G * a->getMass() / g);

        // Velocity is perpendicular to position vector
        float l = -sin(h) * k;
        float m = cos(h) * k;

        // Create the planet with scaled mass
        Planet* n = new Planet(
            sf::Vector2f(i, j),
            0, b * e[f], c[f]);

        n->setVelocity(sf::Vector2f(l, m));
        b.push_back(n);
    }

    // Setup gravity simulator
    a.setSimulatePlanetGravity(true);
    for (auto a : b) {
        this->a.addPlanet(a);
    }

    // Create a default host player (ID 0)
    sf::Vector2f a = b[0]->getPosition() +
        sf::Vector2f(0, -(b[0]->getRadius() + GameConstants::ROCKET_SIZE));
    addPlayer(0, a, sf::Color::White);
}

int GameServer::addPlayer(int playerId, sf::Vector2f initialPos, sf::Color color) {
    // Check if player already exists
    if (c.find(playerId) != c.end()) {
        return playerId; // Player already exists
    }

    try {
        // Create a new vehicle manager for this player
        VehicleManager* a = new VehicleManager(initialPos, b, playerId);

        // Make sure rocket was initialized properly
        if (a && a->getRocket()) {
            a->getRocket()->setColor(color);

            // Add to simulator
            this->a.addVehicleManager(a);

            // Store in players map
            c[playerId] = a;

            // Initialize client simulation tracking
            f[playerId] = GameState();
            g[playerId] = e; // Current game time
            h[playerId] = true; // Initially valid

            std::cout << "Added player with ID: " << playerId << std::endl;
        }
        else {
            std::cerr << "Failed to initialize rocket for player ID: " << playerId << std::endl;
            delete a; // Clean up if rocket initialization failed
        }
    }
    catch (const std::exception& a) {
        std::cerr << "Exception when adding player ID " << playerId << ": " << a.what() << std::endl;
    }

    return playerId;
}

void GameServer::removePlayer(int playerId) {
    auto a = c.find(playerId);
    if (a != c.end()) {
        this->a.removeVehicleManager(a->second);
        delete a->second;
        c.erase(a);

        // Also remove from client simulation tracking
        f.erase(playerId);
        g.erase(playerId);
        h.erase(playerId);
    }
}

void GameServer::update(float deltaTime) {
    // Update game time
    e += deltaTime;

    // Update simulator for server-owned objects
    a.update(deltaTime);

    // Update planets
    for (auto a : b) {
        a->update(deltaTime);
    }

    // Update all players
    for (auto& a : c) {
        if (a.second) { // Add null check before updating
            a.second->update(deltaTime);
        }
    }

    // Increment sequence number
    d++;

    // Synchronize client and server states periodically
    synchronizeState();
}

void GameServer::handlePlayerInput(int playerId, const PlayerInput& input) {
    auto a = c.find(playerId);
    if (a == c.end()) {
        // Player not found - could be a new connection, create player
        std::cout << "Unknown player ID: " << playerId << ", creating new player" << std::endl;
        sf::Vector2f b = this->b[0]->getPosition() +
            sf::Vector2f(0, -(this->b[0]->getRadius() + GameConstants::ROCKET_SIZE));

        // Add the player with error handling
        VehicleManager* c = new VehicleManager(b, this->b, playerId);
        if (c && c->getRocket()) {
            this->c[playerId] = c;
            this->a.addVehicleManager(c);

            // Initialize client simulation tracking
            f[playerId] = GameState();
            g[playerId] = e; // Current game time
            h[playerId] = true; // Initially valid
        }
        else {
            std::cerr << "Failed to create player for ID: " << playerId << std::endl;
            delete c;
        }
        return;
    }

    // Debug output
    std::cout << "Server applying input to player ID: " << playerId << std::endl;

    // Update client state tracking
    if (input.j > g[playerId]) {
        g[playerId] = input.j; // Update last client update time
    }

    // Get client's rocket state if provided
    if (input.k.j) { // If this is authoritative from client
        // Store the client's rocket state
        f[playerId].c.clear();
        f[playerId].c.push_back(input.k);

        // Mark client simulation as valid
        h[playerId] = true;
    }

    // Apply input to the correct player's vehicle manager
    VehicleManager* b = a->second;
    if (!b) return; // Add null check

    // Apply input to the vehicle
    if (input.b) {
        b->applyThrust(1.0f);
    }
    if (input.c) {
        b->applyThrust(-0.5f);
    }
    if (input.d) {
        b->rotate(-6.0f * input.h * 60.0f);
    }
    if (input.e) {
        b->rotate(6.0f * input.h * 60.0f);
    }
    if (input.f) {
        b->switchVehicle();
    }

    // Apply thrust level with null checking
    if (b->getActiveVehicleType() == VehicleType::ROCKET && b->getRocket()) {
        b->getRocket()->setThrustLevel(input.g);
    }
}

void GameServer::processClientSimulation(int playerId, const GameState& clientState) {
    // Store the client's latest state
    f[playerId] = clientState;
    g[playerId] = clientState.b; // Update timestamp

    // Validate the client simulation
    GameState a = validateClientSimulation(playerId, clientState);

    // If validation changed something, send back the corrected state
    if (!h[playerId]) {
        // TODO: Send validation state back to client
    }
}

GameState GameServer::validateClientSimulation(int playerId, const GameState& clientState) {
    GameState a = clientState;
    bool b = true;

    // Get server's state for this player
    VehicleManager* c = getPlayer(playerId);
    if (!c || !c->getRocket()) {
        h[playerId] = false;
        return a;
    }

    // Compare key values between client and server rocket states
    if (!clientState.c.empty()) {
        const RocketState& d = clientState.c[0];

        // Create current server rocket state
        RocketState e;
        c->createState(e);

        // Check position difference
        sf::Vector2f f = d.b - e.b;
        float g = std::sqrt(f.x * f.x + f.y * f.y);

        // Check velocity difference
        sf::Vector2f h = d.c - e.c;
        float j = std::sqrt(h.x * h.x + h.y * h.y);

        // If difference exceeds threshold, client simulation is invalid
        if (g > i || j > i * 10.0f) {
            b = false;

            // Update the client state with server state
            a.c.clear();
            a.c.push_back(e);
        }
    }

    // Update validation status
    this->h[playerId] = b;

    return a;
}

void GameServer::synchronizeState() {
    // For each player, check if their simulation is valid
    for (auto& a : c) {
        int b = a.first;

        // Skip validation for server player (ID 0)
        if (b == 0) continue;

        // Get server's state for this player
        if (f.find(b) != f.end() && h.find(b) != h.find(b)) {
            // Check if client simulation is valid
            if (!h[b]) {
                // Client simulation invalid - next update will send correction
                continue;
            }

            // Check time since last update
            float c = e - g[b];
            if (c > 5.0f) {
                // Too long since last update, mark invalid
                h[b] = false;
            }
        }
    }
}

GameState GameServer::getGameState() const {
    GameState a;
    a.a = d;
    a.b = e;
    a.e = false; // Not initial state by default

    try {
        // Add all rockets
        for (const auto& b : c) {
            int c = b.first;
            const VehicleManager* d = b.second;

            // Add null checks
            if (!d) continue;

            // Only add if it's a rocket
            if (d->getActiveVehicleType() == VehicleType::ROCKET) {
                const Rocket* e = d->getRocket();

                // Add null check for rocket
                if (!e) continue;

                RocketState f;
                f.a = c;  // playerId
                f.b = e->getPosition();  // position
                f.c = e->getVelocity();  // velocity
                f.d = e->getRotation();  // rotation
                f.e = 0.0f;  // angularVelocity - not tracked in current design
                f.f = e->getThrustLevel();  // thrustLevel
                f.g = e->getMass();  // mass
                f.h = e->getColor();  // color
                f.i = this->e;  // current server timestamp
                f.j = true;  // Server state is authoritative

                a.c.push_back(f);
            }
            // TODO: Add car state if needed
        }

        // Add all planets
        for (size_t b = 0; b < this->b.size(); ++b) {
            const Planet* c = this->b[b];

            // Skip null planets
            if (!c) continue;

            PlanetState d;
            d.a = static_cast<int>(b);  // planetId
            d.b = c->getPosition();  // position
            d.c = c->getVelocity();  // velocity
            d.d = c->getMass();  // mass
            d.e = c->getRadius();  // radius
            d.f = c->getColor();  // color
            d.g = c->getOwnerId();  // ownerId
            d.h = this->e;  // timestamp

            a.d.push_back(d);
        }
    }
    catch (const std::exception& b) {
        std::cerr << "Exception in getGameState: " << b.what() << std::endl;
        // Return a minimal valid state to avoid crashes
    }

    return a;
}