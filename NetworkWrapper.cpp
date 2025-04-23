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
            // Set up server
            gameServer = new GameServer();
            if (!gameServer) {
                std::cerr << "Failed to create GameServer instance" << std::endl;
                return false;
            }

            // Initialize server with proper exception handling
            try {
                gameServer->initialize();
            }
            catch (const std::exception& e) {
                std::cerr << "Exception during server initialization: " << e.what() << std::endl;
                delete gameServer;
                gameServer = nullptr;
                return false;
            }

            // Connect NetworkManager with GameServer
            networkManager.setGameServer(gameServer);

            // Start hosting
            if (!networkManager.hostGame(port)) {
                std::cerr << "Failed to host game on port " << port << std::endl;
                return false;
            }

            // Setup callback to handle player input
            networkManager.onPlayerInputReceived = [this](int clientId, const PlayerInput& input) {
                if (gameServer) {
                    gameServer->handlePlayerInput(clientId, input);
                }
                };

            std::cout << "Successfully hosting game on port " << port << std::endl;
        }
        else {
            // Set up client
            gameClient = new GameClient();
            if (!gameClient) {
                std::cerr << "Failed to create GameClient instance" << std::endl;
                return false;
            }

            // Initialize client with proper exception handling
            try {
                gameClient->initialize();
            }
            catch (const std::exception& e) {
                std::cerr << "Exception during client initialization: " << e.what() << std::endl;
                delete gameClient;
                gameClient = nullptr;
                return false;
            }

            // Connect NetworkManager with GameClient
            networkManager.setGameClient(gameClient);

            // Setup callback to handle game state updates
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

            // Parse address
            sf::IpAddress serverAddress = sf::IpAddress::LocalHost;
            if (!address.empty()) {
                if (auto resolvedAddress = sf::IpAddress::resolve(address)) {
                    serverAddress = *resolvedAddress;
                }
                else {
                    std::cerr << "Invalid IP address format: " << address << std::endl;
                    return false;
                }
            }

            // Connect to server
            if (!networkManager.joinGame(serverAddress, port)) {
                std::cerr << "Failed to connect to server at " << serverAddress.toString() << ":" << port << std::endl;
                return false;
            }

            // Set a default player ID until server assigns one
            gameClient->setLocalPlayerId(1);

            std::cout << "Successfully connected to server at " << serverAddress.toString() << ":" << port << std::endl;
        }

        // Enable robust networking
        networkManager.enableRobustNetworking();

        // Set client latency compensation
        if (!isHost && gameClient) {
            gameClient->setLatencyCompensation(0.2f); // 200ms interpolation window
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