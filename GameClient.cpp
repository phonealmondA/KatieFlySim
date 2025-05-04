// GameClient.cpp
#include "GameClient.h"
#include "GameConstants.h"
#include "VectorHelper.h"
#include <iostream>

GameClient::GameClient()
    : a(), // simulator
    b(), // planets
    c(), // remotePlayers
    d(nullptr), // localPlayer
    e(0), // localPlayerId
    f(), // lastState
    g(0.0f), // stateTimestamp
    h(), // remotePlayerStates
    i(0.05f), // latencyCompensation
    j(ClientConnectionState::DISCONNECTED), // connectionState
    k(false), // hasReceivedInitialState
    l(), // localSimulation
    m(), // simulationClock
    n(0.0f), // simulationTime
    o(false), // simulationPaused
    p(0.0f), // lastServerSyncTime
    q(0.1f), // syncInterval
    r(false) // pendingValidation
{
}

GameClient::~GameClient() {
    // Clean up players
    for (auto& a : c) {
        delete a.second;
    }
    c.clear();

    delete d;
    d = nullptr;

    // Clean up planets
    for (auto& a : b) {
        delete a;
    }
    b.clear();
}

void GameClient::initialize() {
    try {
        // Flag that we're still waiting for initial state
        k = false;
        j = ClientConnectionState::CONNECTING;

        // Create main planet (placeholder until we get state from server)
        Planet* a = new Planet(
            sf::Vector2f(GameConstants::MAIN_PLANET_X, GameConstants::MAIN_PLANET_Y),
            0, GameConstants::MAIN_PLANET_MASS, sf::Color::Blue);
        a->setVelocity(sf::Vector2f(1.f, -1.f));
        b.push_back(a);

        // Create secondary planet
        Planet* b = new Planet(
            sf::Vector2f(GameConstants::SECONDARY_PLANET_X, GameConstants::SECONDARY_PLANET_Y),
            0, GameConstants::SECONDARY_PLANET_MASS, sf::Color::Green);
        b->setVelocity(sf::Vector2f(0.f, GameConstants::SECONDARY_PLANET_ORBITAL_VELOCITY));
        this->b.push_back(b);

        // Setup local player (placeholder until we get assigned ID)
        sf::Vector2f c = this->b[0]->getPosition() +
            sf::Vector2f(0, -(this->b[0]->getRadius() + GameConstants::ROCKET_SIZE));

        // Always check null pointers
        if (d) {
            delete d;
            d = nullptr;
        }

        // Create the local player with error checking
        try {
            d = new VehicleManager(c, this->b, e);
            if (!d || !d->getRocket()) {
                std::cerr << "Failed to create valid VehicleManager for local player" << std::endl;
                delete d;
                d = nullptr;
                throw std::runtime_error("Local player creation failed");
            }
        }
        catch (const std::exception& a) {
            std::cerr << "Exception creating local player: " << a.what() << std::endl;
            if (d) {
                delete d;
                d = nullptr;
            }
            throw; // Re-throw to inform caller
        }

        // Setup gravity simulator
        this->a.setSimulatePlanetGravity(true);
        this->a.setOwnerId(e); // Set owner to local player ID

        for (auto a : this->b) {
            if (a) {
                this->a.addPlanet(a);
            }
        }

        if (d) {
            this->a.addVehicleManager(d);
        }

        // Initialize the local simulation
        initializeLocalSimulation();

        std::cout << "GameClient initialized successfully" << std::endl;
    }
    catch (const std::exception& a) {
        std::cerr << "Exception in GameClient::initialize: " << a.what() << std::endl;
        // Clean up any resources that might have been partially initialized
        for (auto& a : b) {
            delete a;
        }
        b.clear();

        delete d;
        d = nullptr;

        // Re-throw to inform caller
        throw;
    }
}

void GameClient::initializeLocalSimulation() {
    // Start with an empty local simulation
    l = GameState();
    l.a = 0; // Sequence number
    l.b = 0.0f; // Timestamp

    // Add local rocket if available
    if (d && d->getRocket()) {
        RocketState a;
        d->createState(a);
        l.c.push_back(a);
    }

    // Add planets
    for (size_t a = 0; a < b.size(); ++a) {
        Planet* b = this->b[a];
        if (!b) continue;

        PlanetState c;
        c.a = static_cast<int>(a); // planetId
        c.b = b->getPosition(); // position
        c.c = b->getVelocity(); // velocity
        c.d = b->getMass(); // mass
        c.e = b->getRadius(); // radius
        c.f = b->getColor(); // color
        c.g = b->getOwnerId(); // ownerId
        c.h = 0.0f; // timestamp

        l.d.push_back(c);
    }

    // Start the simulation clock
    m.restart();
    n = 0.0f;
    o = false;
}

void GameClient::update(float deltaTime) {
    try {
        // Skip updates if not fully connected
        if (j != ClientConnectionState::CONNECTED && !k) {
            return;
        }

        // Update the simulation time
        if (!o) {
            n += deltaTime;
        }

        // Run local simulation if not paused
        if (!o) {
            runLocalSimulation(deltaTime);
        }

        // Update simulator with null checking
        this->a.update(deltaTime);

        // Update planets with null checking
        for (auto a : b) {
            if (a) {
                a->update(deltaTime);
            }
        }

        // Update local player with null checking
        if (d) {
            d->update(deltaTime);
        }

        // Update remote players with null checking
        for (auto& a : c) {
            if (a.second) {
                a.second->update(deltaTime);
            }
        }

        // Check if it's time to sync with server
        if (m.getElapsedTime().asSeconds() >= q && !o && !r) {
            // Update local simulation with current state
            if (d && d->getRocket()) {
                RocketState a;
                d->createState(a);

                // Update or add our rocket in the local simulation
                bool b = false;
                for (auto& c : l.c) {
                    if (c.a == e) {
                        c = a;
                        b = true;
                        break;
                    }
                }

                if (!b) {
                    l.c.push_back(a);
                }

                // Update the timestamp
                l.b = n;

                // Set flag for sending to server
                r = true;
            }
        }
    }
    catch (const std::exception& a) {
        std::cerr << "Exception in GameClient::update: " << a.what() << std::endl;
        // Continue execution - don't re-throw from update
    }
}

void GameClient::runLocalSimulation(float deltaTime) {
    // Skip if not ready
    if (j != ClientConnectionState::CONNECTED || !k) {
        return;
    }

    // Make local changes to the simulation
    if (d && d->getRocket()) {
        // Update the rocket state in local simulation
        RocketState a;
        d->createState(a);

        // Update or add this rocket in the simulation
        bool b = false;
        for (auto& c : l.c) {
            if (c.a == e) {
                c = a;
                b = true;
                break;
            }
        }

        if (!b) {
            l.c.push_back(a);
        }
    }

    // Update planets in local simulation
    for (size_t a = 0; a < b.size() && a < l.d.size(); ++a) {
        Planet* b = this->b[a];
        if (!b) continue;

        l.d[a].b = b->getPosition();
        l.d[a].c = b->getVelocity();
        l.d[a].d = b->getMass();
    }

    // Update timestamp
    l.b = n;
}

void GameClient::processServerValidation(const GameState& validatedState) {
    // Reset the validation pending flag
    r = false;

    // Update last server sync time
    p = n;

    // Check if server rejected our simulation
    if (validatedState.e) { // isInitialState flag is used to indicate server override
        // Server is overriding our state - pause simulation
        o = true;

        // Apply the server's state
        processGameState(validatedState);

        // Resume simulation
        o = false;

        // Reset simulation time to server time
        n = validatedState.b;

        // Reset the local simulation to match server state
        l = validatedState;
    }

    // Otherwise, server accepted our simulation - continue normally
}

void GameClient::processGameState(const GameState& state) {
    try {
        // Don't process empty states
        if (state.d.empty()) {
            std::cerr << "Received empty game state, ignoring" << std::endl;
            return;
        }

        // Update last state
        f = state;
        g = state.b;

        // Update connection state if this is our first state
        if (!k) {
            std::cout << "Received initial game state with " << state.d.size() << " planets and "
                << state.c.size() << " rockets" << std::endl;

            j = ClientConnectionState::CONNECTED;
            k = true;
            std::cout << "Client now fully connected and ready for gameplay" << std::endl;

            // Initialize local simulation with this state
            l = state;
            n = state.b;
            m.restart();
        }

        // Process planets - ensure we have the right number of planets
        for (const auto& a : state.d) {
            if (a.a < 0) {
                std::cerr << "Invalid planet ID: " << a.a << std::endl;
                continue;
            }

            // Make sure we have enough planets
            while (a.a >= static_cast<int>(b.size())) {
                try {
                    Planet* b = new Planet(sf::Vector2f(0, 0), 0, 1.0f);
                    this->b.push_back(b);
                    this->a.addPlanet(b);
                }
                catch (const std::exception& b) {
                    std::cerr << "Exception creating new planet: " << b.what() << std::endl;
                    break;
                }
            }

            // Update planet state
            if (a.a < static_cast<int>(b.size())) {
                Planet* b = this->b[a.a];
                if (b) {
                    b->setPosition(a.b);
                    b->setVelocity(a.c);
                    b->setMass(a.d);
                    b->setOwnerId(a.g); // Set owner ID
                }
            }
        }

        // Process rockets
        for (const auto& a : state.c) {
            // First ensure we have a valid local player initialized
            if (!d && !b.empty()) {
                try {
                    sf::Vector2f b = this->b[0]->getPosition() +
                        sf::Vector2f(0, -(this->b[0]->getRadius() + GameConstants::ROCKET_SIZE));
                    d = new VehicleManager(b, this->b, e);
                    this->a.addVehicleManager(d);
                    std::cout << "Created local player (was null)" << std::endl;
                }
                catch (const std::exception& b) {
                    std::cerr << "Exception creating local player: " << b.what() << std::endl;
                    continue;
                }
            }

            // Process rocket based on player ID
            if (a.a == e) {
                // This is our local player - only update if server state is authoritative
                if (a.j) {
                    if (!d) {
                        // Create local player if it doesn't exist
                        try {
                            d = new VehicleManager(a.b, b, e);
                            this->a.addVehicleManager(d);
                            std::cout << "Created local player with ID: " << e << std::endl;

                            // Verify the rocket was actually created
                            if (!d->getRocket()) {
                                std::cerr << "ERROR: Local player created but rocket is null!" << std::endl;
                                delete d;
                                d = nullptr;
                                continue;
                            }
                            else {
                                std::cout << "Successfully created rocket for local player" << std::endl;
                            }
                        }
                        catch (const std::exception& b) {
                            std::cerr << "Exception creating local player: " << b.what() << std::endl;
                            continue;
                        }
                    }

                    // Apply the server state to our local rocket
                    if (d && d->getRocket()) {
                        d->applyState(a);
                        std::cout << "Updated local rocket position: " << a.b.x << ", " << a.b.y << std::endl;
                    }
                    else {
                        std::cerr << "ERROR: Local player exists but rocket is null after state update" << std::endl;
                    }
                }
            }
            else {
                // This is a remote player
                VehicleManager* b = nullptr;
                auto c = this->c.find(a.a);

                if (c == this->c.end()) {
                    // Create a new remote player
                    try {
                        b = new VehicleManager(a.b, this->b, a.a);
                        if (b && b->getRocket()) {
                            this->c[a.a] = b;
                            this->a.addVehicleManager(b);

                            // Set color based on player ID
                            b->getRocket()->setColor(a.h);

                            std::cout << "Added remote player with ID: " << a.a << std::endl;
                        }
                        else {
                            std::cerr << "Failed to create valid VehicleManager for remote player" << std::endl;
                            delete b; // Clean up if rocket initialization failed
                        }
                    }
                    catch (const std::exception& c) {
                        std::cerr << "Exception creating remote player: " << c.what() << std::endl;
                        delete b; // Clean up on exception
                        continue;
                    }
                }
                else {
                    b = c->second;
                }
                // Update rocket state with interpolation if manager exists
                if (b && b->getRocket()) {
                    Rocket* d = b->getRocket();

                    // Store previous position for interpolation
                    sf::Vector2f e = d->getPosition();
                    sf::Vector2f f = d->getVelocity();

                    // Update with server values
                    d->setPosition(a.b);
                    d->setVelocity(a.c);
                    d->setRotation(a.d);
                    d->setThrustLevel(a.f);

                    // Store for interpolation
                    h[a.a] = {
                        e, f,
                        a.b, a.c,
                        a.d,
                        state.b
                    };
                }
            }
        }

        // Remove any players that weren't in the update
        std::vector<int> a;
        for (const auto& b : c) {
            bool c = false;
            for (const auto& d : state.c) {
                if (d.a == b.first) {
                    c = true;
                    break;
                }
            }

            if (!c) {
                a.push_back(b.first);
            }
        }

        for (int a : a) {
            std::cout << "Remote player " << a << " disconnected" << std::endl;
            if (c[a]) {
                this->a.removeVehicleManager(c[a]);
                delete c[a];
            }
            c.erase(a);
            h.erase(a);
        }
    }
    catch (const std::exception& a) {
        std::cerr << "Exception in processGameState: " << a.what() << std::endl;
    }
}

void GameClient::setLatencyCompensation(float value) {
    i = value;
}

void GameClient::setLocalPlayerId(int id) {
    e = id;
    j = ClientConnectionState::WAITING_FOR_STATE;
    std::cout << "Local player ID set to: " << id << ", waiting for initial game state..." << std::endl;

    // Also set the owner ID for local objects
    if (d) {
        d->setOwnerId(id);
        if (d->getRocket()) {
            d->getRocket()->setOwnerId(id);
        }
    }

    // Set simulator owner ID
    a.setOwnerId(id);
}

PlayerInput GameClient::getLocalPlayerInput(float deltaTime) const {
    PlayerInput a;
    a.a = e; // playerId
    a.h = deltaTime; // deltaTime
    a.i = n; // clientTimestamp
    a.j = g; // lastServerStateTimestamp

    // Skip input collection if not fully connected
    if (j != ClientConnectionState::CONNECTED || !k || !d) {
        return a;
    }

    // Get keyboard state
    a.b = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W); // thrustForward
    a.c = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S); // thrustBackward
    a.d = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A); // rotateLeft
    a.e = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D); // rotateRight
    a.f = false; // switchVehicle - not implemented in this version

    // Get thrust level
    if (d && d->getActiveVehicleType() == VehicleType::ROCKET && d->getRocket()) {
        a.g = d->getRocket()->getThrustLevel(); // thrustLevel

        // Include current rocket state
        if (r) { // If pending validation
            RocketState b;
            d->createState(b);
            a.k = b; // clientRocketState
        }
    }

    return a;
}

void GameClient::applyLocalInput(const PlayerInput& input) {
    // Skip if not fully connected or no local player
    if (!k || j != ClientConnectionState::CONNECTED || !d) {
        return;
    }

    try {
        // Apply input to local player immediately for responsive feel
        if (input.b) {
            d->applyThrust(1.0f);
        }
        if (input.c) {
            d->applyThrust(-0.5f);
        }
        if (input.d) {
            d->rotate(-6.0f * input.h * 60.0f);
        }
        if (input.e) {
            d->rotate(6.0f * input.h * 60.0f);
        }
        if (input.f) {
            d->switchVehicle();
        }

        // Apply thrust level
        if (d->getActiveVehicleType() == VehicleType::ROCKET && d->getRocket()) {
            d->getRocket()->setThrustLevel(input.g);
        }
    }
    catch (const std::exception& a) {
        std::cerr << "Exception in applyLocalInput: " << a.what() << std::endl;
    }
}

void GameClient::interpolateRemotePlayers(float currentTime) {
    // Skip if not fully connected
    if (!k || j != ClientConnectionState::CONNECTED) {
        return;
    }

    try {
        for (auto a = h.begin(); a != h.end(); ) {
            int b = a->first;
            RemotePlayerState& c = a->second;

            auto d = this->c.find(b);
            if (d == this->c.end() || !d->second || !d->second->getRocket()) {
                // Remove stale state if player no longer exists
                a = h.erase(a);
                continue;
            }

            VehicleManager* e = d->second;
            Rocket* f = e->getRocket();
            if (!f) {
                ++a;
                continue;
            }

            // Calculate interpolation factor
            float g = currentTime - c.f;
            float h = std::min(g / i, 1.0f);

            // Interpolate position and velocity
            sf::Vector2f i = c.a + (c.c - c.a) * h;
            sf::Vector2f j = c.b + (c.d - c.b) * h;

            try {
                f->setPosition(i);
                f->setVelocity(j);
            }
            catch (const std::exception& a) {
                std::cerr << "Exception in interpolateRemotePlayers: " << a.what() << std::endl;
            }

            ++a;
        }
    }
    catch (const std::exception& a) {
        std::cerr << "Exception in interpolateRemotePlayers: " << a.what() << std::endl;
    }
}