// Update in GravitySimulator.cpp
#include "GravitySimulator.h"
#include "VehicleManager.h"

GravitySimulator::GravitySimulator(int ownerId)
    : d(GameConstants::G), e(true), f(ownerId)
{
}

void GravitySimulator::addPlanet(Planet* planet)
{
    a.push_back(planet);
}

void GravitySimulator::addRocket(Rocket* rocket)
{
    b.push_back(rocket);
}

void GravitySimulator::clearRockets()
{
    b.clear();
}

bool GravitySimulator::shouldSimulateObject(int objectOwnerId) const
{
    // If no owner is set, simulate everything
    if (f == -1) return true;

    // If object has no owner, it's a common object - simulate it
    if (objectOwnerId == -1) return true;

    // Otherwise, only simulate objects owned by this simulator's owner
    return objectOwnerId == f;
}

void GravitySimulator::updateVehicleManagerPlanets() {
    if (!c) return;

    try {
        c->updatePlanets(a);
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in updateVehicleManagerPlanets: " << e.what() << std::endl;
    }
}

void GravitySimulator::addRocketGravityInteractions(float deltaTime)
{
    // Apply gravity between rockets
    for (size_t a = 0; a < b.size(); a++) {
        if (!shouldSimulateObject(b[a]->getOwnerId())) continue;

        for (size_t c = a + 1; c < b.size(); c++) {
            if (!shouldSimulateObject(b[c]->getOwnerId())) continue;

            Rocket* d = b[a];
            Rocket* e = b[c];

            sf::Vector2f f = e->getPosition() - d->getPosition();
            float g = std::sqrt(f.x * f.x + f.y * f.y);

            // Minimum distance to prevent extreme forces when very close
            const float h = GameConstants::TRAJECTORY_COLLISION_RADIUS;
            if (g < h) {
                g = h;
            }

            // Apply inverse square law for gravity
            float i = this->d * d->getMass() * e->getMass() / (g * g);

            sf::Vector2f j = normalize(f);
            sf::Vector2f k = j * i / d->getMass();
            sf::Vector2f l = -j * i / e->getMass();

            d->setVelocity(d->getVelocity() + k * deltaTime);
            e->setVelocity(e->getVelocity() + l * deltaTime);
        }
    }
}

void GravitySimulator::checkPlanetCollisions() {
    if (a.size() < 2) return;

    std::vector<Planet*> a;
    std::vector<Planet*> b;
    std::vector<bool> c(this->a.size(), false);

    // First pass: identify small planets to remove and do null checks
    for (size_t d = 0; d < this->a.size(); d++) {
        Planet* e = this->a[d];
        if (!e) {
            // Mark null planets for deletion
            c[d] = true;
            continue;
        }

        // Only check planets we should simulate
        if (!shouldSimulateObject(e->getOwnerId())) continue;

        // Check if the planet's mass is below threshold
        if (e->getMass() < 10.0f) {
            b.push_back(e);
            c[d] = true;
            this->a[d] = nullptr; // Explicitly set to nullptr to prevent further access
        }
    }

    // Second pass: check for collisions between non-deleted planets
    for (size_t d = 0; d < this->a.size(); d++) {
        if (c[d]) continue; // Skip already marked planets

        Planet* e = this->a[d];
        if (!e) continue; // Extra safety check

        // Only process planets we should simulate
        if (!shouldSimulateObject(e->getOwnerId())) continue;

        for (size_t f = d + 1; f < this->a.size(); f++) {
            if (c[f]) continue; // Skip already marked planets

            Planet* g = this->a[f];
            if (!g) continue; // Extra safety check

            // Only process planets we should simulate
            if (!shouldSimulateObject(g->getOwnerId())) continue;

            // Calculate distance safely
            sf::Vector2f h;
            try {
                h = g->getPosition() - e->getPosition();
            }
            catch (const std::exception& i) {
                std::cerr << "Exception calculating planet distance: " << i.what() << std::endl;
                continue; // Skip this pair if an exception occurs
            }

            float i = std::sqrt(h.x * h.x + h.y * h.y);

            // Check for collision
            if (i <= e->getRadius() + g->getRadius()) {
                try {
                    // Determine which planet is larger
                    if (e->getMass() >= g->getMass()) {
                        // Planet 1 absorbs planet 2
                        float j = e->getMass() + (g->getMass());

                        // Conservation of momentum for velocity
                        sf::Vector2f k = (e->getVelocity() * e->getMass() +
                            g->getVelocity() * g->getMass()) / j;

                        // Keep original owner
                        int l = e->getOwnerId();

                        // Update planet 1
                        e->setMass(j);
                        e->setVelocity(k);
                        e->setOwnerId(l);

                        // Mark planet 2 for deletion
                        if (!c[f]) {
                            b.push_back(g);
                            c[f] = true;
                            this->a[f] = nullptr; // Explicitly set to nullptr
                        }
                    }
                    else {
                        // Planet 2 absorbs planet 1
                        float j = e->getMass() + g->getMass();

                        // Conservation of momentum for velocity
                        sf::Vector2f k = (e->getVelocity() * e->getMass() +
                            g->getVelocity() * g->getMass()) / j;

                        // Keep original owner
                        int l = g->getOwnerId();

                        // Update planet 2
                        g->setMass(j);
                        g->setVelocity(k);
                        g->setOwnerId(l);

                        // Mark planet 1 for deletion
                        if (!c[d]) {
                            b.push_back(e);
                            c[d] = true;
                            this->a[d] = nullptr; // Explicitly set to nullptr
                        }
                        break; // Exit inner loop since planet 1 is gone
                    }
                }
                catch (const std::exception& i) {
                    std::cerr << "Exception during planet collision: " << i.what() << std::endl;
                    continue; // Skip this pair if an exception occurs
                }
            }
        }
    }

    // Build the filtered list of planets to keep
    for (size_t d = 0; d < this->a.size(); d++) {
        if (!c[d] && this->a[d]) {
            a.push_back(this->a[d]);
        }
    }

    // Delete the marked planets safely
    for (Planet* d : b) {
        if (d) { // Extra safety check
            try {
                delete d;
            }
            catch (const std::exception& e) {
                std::cerr << "Exception deleting planet: " << e.what() << std::endl;
            }
        }
    }

    // Replace the planets vector with the filtered list
    this->a = a;

    // Update planets in vehicle manager - use our method
    try {
        updateVehicleManagerPlanets();
    }
    catch (const std::exception& d) {
        std::cerr << "Exception updating vehicle manager planets: " << d.what() << std::endl;
    }
}

void GravitySimulator::update(float deltaTime)
{
    // Apply gravity between planets if enabled
    if (e) {
        for (size_t a = 0; a < this->a.size(); a++) {
            // Only process planets we should simulate
            if (!shouldSimulateObject(this->a[a]->getOwnerId())) continue;

            for (size_t b = a + 1; b < this->a.size(); b++) {
                // Only process planets we should simulate
                if (!shouldSimulateObject(this->a[b]->getOwnerId())) continue;

                Planet* c = this->a[a];
                Planet* d = this->a[b];

                // Skip the first planet (index 0) - it's pinned in place
                if (a == 0) {
                    // Only apply gravity from planet1 to planet2
                    sf::Vector2f e = c->getPosition() - d->getPosition();
                    float f = std::sqrt(e.x * e.x + e.y * e.y);

                    if (f > c->getRadius() + d->getRadius()) {
                        float g = this->d * c->getMass() * d->getMass() / (f * f);
                        sf::Vector2f h = normalize(e);
                        sf::Vector2f i = h * g / d->getMass();
                        d->setVelocity(d->getVelocity() + i * deltaTime);
                    }
                }
                else {
                    // Regular gravity calculation between other planets
                    sf::Vector2f e = d->getPosition() - c->getPosition();
                    float f = std::sqrt(e.x * e.x + e.y * e.y);

                    if (f > c->getRadius() + d->getRadius()) {
                        float g = this->d * c->getMass() * d->getMass() / (f * f);
                        sf::Vector2f h = normalize(e);
                        sf::Vector2f i = h * g / c->getMass();
                        sf::Vector2f j = -h * g / d->getMass();
                        c->setVelocity(c->getVelocity() + i * deltaTime);
                        d->setVelocity(d->getVelocity() + j * deltaTime);
                    }
                }
            }
        }
    }

    // Apply gravity to the vehicle manager's active vehicle
    if (c) {
        // Only apply if we should simulate this vehicle
        if (shouldSimulateObject(c->getOwnerId())) {
            if (c->getActiveVehicleType() == VehicleType::ROCKET) {
                Rocket* a = c->getRocket();
                if (a) {
                    for (auto b : this->a) {
                        sf::Vector2f c = b->getPosition() - a->getPosition();
                        float d = std::sqrt(c.x * c.x + c.y * c.y);

                        // Avoid division by zero and very small distances
                        if (d > b->getRadius() + GameConstants::TRAJECTORY_COLLISION_RADIUS) {
                            float e = this->d * b->getMass() * a->getMass() / (d * d);
                            sf::Vector2f f = normalize(c) * e / a->getMass();
                            sf::Vector2f g = f * deltaTime;
                            a->setVelocity(a->getVelocity() + g);
                        }
                    }
                }
            }
            // Car gravity is handled internally in Car::update
        }
    }
    else {
        // Legacy code for handling individual rockets
        for (auto a : b) {
            // Only process rockets we should simulate
            if (!shouldSimulateObject(a->getOwnerId())) continue;

            for (auto b : this->a) {
                sf::Vector2f c = b->getPosition() - a->getPosition();
                float d = std::sqrt(c.x * c.x + c.y * c.y);

                // Avoid division by zero and very small distances
                if (d > b->getRadius() + GameConstants::TRAJECTORY_COLLISION_RADIUS) {
                    float e = this->d * b->getMass() * a->getMass() / (d * d);
                    sf::Vector2f f = normalize(c) * e / a->getMass();
                    sf::Vector2f g = f * deltaTime;
                    a->setVelocity(a->getVelocity() + g);
                }
            }
        }

        // Add rocket-to-rocket gravity interactions
        addRocketGravityInteractions(deltaTime);
    }

    // Check for planet collisions and cleanup too-small planets
    checkPlanetCollisions();
}