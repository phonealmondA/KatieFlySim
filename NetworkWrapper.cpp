// NetworkWrapper.cpp
#include "NetworkWrapper.h"
#include <iostream>

NetworkWrapper::NetworkWrapper()
    : gameServer(nullptr),
    gameClient(nullptr),
    isMultiplayer(false),
    isHost(false)
{
}

NetworkWrapper::~NetworkWrapper()
{
    if (gameServer) {
        delete gameServer;
        gameServer = nullptr;
    }

    if (gameClient) {
        delete gameClient;
        gameClient = nullptr;
    }

    networkManager.disconnect();
}
// NetworkWrapper.cpp - the relevant section to fix

bool NetworkWrapper::initialize(bool host, const std::string& address, unsigned short port)
{
    isMultiplayer = true;
    isHost = host;

    try {
        if (isHost) {
            // Server code remains unchanged
            gameServer = new GameServer();
            if (!gameServer) {
                std::cerr << "Failed to create GameServer instance" << std::endl;
                return false;
            }

            try {
                gameServer->initialize();
            }
            catch (const std::exception& e) {
                std::cerr << "Exception during server initialization: " << e.what() << std::endl;
                delete gameServer;
                gameServer = nullptr;
                return false;
            }

            networkManager.setGameServer(gameServer);

            if (!networkManager.hostGame(port)) {
                std::cerr << "Failed to host game on port " << port << std::endl;
                return false;
            }

            networkManager.onPlayerInputReceived = [this](int clientId, const PlayerInput& input) {
                if (gameServer) {
                    gameServer->handlePlayerInput(clientId, input);
                }
                };

            std::cout << "Successfully hosting game on port " << port << std::endl;
        }
        else {
            // Client code - Fixed with better error handling and safety checks
            gameClient = new GameClient();
            if (!gameClient) {
                std::cerr << "Failed to create GameClient instance" << std::endl;
                return false;
            }

            // Initialize gameClient first before setting up callbacks
            try {
                gameClient->initialize();
            }
            catch (const std::exception& e) {
                std::cerr << "Exception during client initialization: " << e.what() << std::endl;
                delete gameClient;
                gameClient = nullptr;
                return false;
            }

            // Set up network callbacks after client is initialized
            networkManager.setGameClient(gameClient);

            // Improved callback with better error handling
            networkManager.onGameStateReceived = [this](const GameState& state) {
                if (gameClient) {
                    try {
                        gameClient->processGameState(state);
                    }
                    catch (const std::exception& e) {
                        std::cerr << "Exception processing game state: " << e.what() << std::endl;
                    }
                }
                };

            // Validate IP address
            sf::IpAddress serverAddress = sf::IpAddress::LocalHost;
            if (!address.empty()) {
                auto resolvedAddress = sf::IpAddress::resolve(address);
                if (resolvedAddress) {
                    serverAddress = *resolvedAddress;
                }
                else {
                    std::cerr << "Invalid IP address format: " << address << std::endl;
                    return false;
                }
            }

            // Try connecting with retry logic
            int maxRetries = 3;
            bool connected = false;

            for (int attempt = 1; attempt <= maxRetries; attempt++) {
                std::cout << "Connection attempt " << attempt << " of " << maxRetries << "..." << std::endl;

                if (networkManager.joinGame(serverAddress, port)) {
                    connected = true;
                    break;
                }

                if (attempt < maxRetries) {
                    std::cout << "Retrying in 1 second..." << std::endl;
                    sf::sleep(sf::seconds(1));
                }
            }

            if (!connected) {
                std::cerr << "Failed to connect after " << maxRetries << " attempts." << std::endl;
                delete gameClient;
                gameClient = nullptr;
                return false;
            }

            // Set a default player ID (will be updated by server)
            gameClient->setLocalPlayerId(1);
            std::cout << "Successfully connected to server at " << serverAddress.toString() << ":" << port << std::endl;
        }

        // Enable robust networking
        networkManager.enableRobustNetworking();

        // Set client latency compensation
        if (!isHost && gameClient) {
            gameClient->setLatencyCompensation(0.2f);
        }

        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception during network initialization: " << e.what() << std::endl;

        // Clean up resources on error
        if (gameServer) {
            delete gameServer;
            gameServer = nullptr;
        }
        if (gameClient) {
            delete gameClient;
            gameClient = nullptr;
        }

        networkManager.disconnect();
        return false;
    }
}

void NetworkWrapper::update(float deltaTime)
{
    if (!isMultiplayer) return;

    // Update network state
    networkManager.update();

    // Track game time for interpolation
    static float gameTime = 0.0f;
    gameTime += deltaTime;

    if (isHost && gameServer) {
        // Update server simulation
        gameServer->update(deltaTime);

        // Send updated game state to clients every 50ms (20 times per second)
        static sf::Clock stateUpdateClock;
        if (stateUpdateClock.getElapsedTime().asMilliseconds() > 50) {
            GameState state = gameServer->getGameState();
            networkManager.sendGameState(state);
            stateUpdateClock.restart();
        }
    }
    else if (gameClient) {
        // Client-side prediction and interpolation
        gameClient->update(deltaTime);
        gameClient->interpolateRemotePlayers(gameTime);
    }
}