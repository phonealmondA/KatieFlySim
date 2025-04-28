// NetworkWrapper.cpp
#include "NetworkWrapper.h"
#include "GameServer.h"
#include "GameClient.h"
#include <iostream>

NetworkWrapper::NetworkWrapper()
    : gameServer(nullptr),
    gameClient(nullptr),
    isMultiplayer(false),
    isHost(false)
{
    // Initialize components
}

NetworkWrapper::~NetworkWrapper()
{
    try {
        // First disconnect network to stop any threads
        if (networkManager.isConnected()) {
            networkManager.disconnect();
        }

        // Then clean up resources
        if (gameServer) {
            delete gameServer;
            gameServer = nullptr;
        }

        if (gameClient) {
            delete gameClient;
            gameClient = nullptr;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in NetworkWrapper destructor: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown exception in NetworkWrapper destructor" << std::endl;
    }
}

bool NetworkWrapper::initialize(bool host, const std::string& address, unsigned short port)
{
    try {
        isMultiplayer = true;
        isHost = host;

        std::cout << "Initializing network in " << (host ? "host" : "client") << " mode..." << std::endl;

        if (host) {
            // Server mode
            try {
                gameServer = new GameServer();
                if (!networkManager.hostGame(port)) {
                    std::cerr << "Failed to start server on port " << port << std::endl;
                    delete gameServer;
                    gameServer = nullptr;
                    return false;
                }
                networkManager.setGameServer(gameServer);
                std::cout << "Server started successfully!" << std::endl;
            }
            catch (const std::exception& e) {
                std::cerr << "Exception creating GameServer: " << e.what() << std::endl;
                delete gameServer;
                gameServer = nullptr;
                return false;
            }
        }
        else {
            // Client mode
            try {
                gameClient = new GameClient();
                gameClient->initialize();

                std::cout << "Connecting to " << address << ":" << port << "..." << std::endl;

                // Convert string address to IpAddress
                auto serverAddressOpt = sf::IpAddress::resolve(address);
                if (!serverAddressOpt) {
                    std::cerr << "Failed to resolve address: " << address << std::endl;
                    delete gameClient;
                    gameClient = nullptr;
                    return false;
                }

                if (!networkManager.joinGame(*serverAddressOpt, port)) {
                    std::cerr << "Failed to connect to server" << std::endl;
                    delete gameClient;
                    gameClient = nullptr;
                    return false;
                }

                // Set up callback for game state processing
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

                networkManager.setGameClient(gameClient);
                std::cout << "Successfully connected to server!" << std::endl;
                std::cout << "Network connection established." << std::endl;
            }
            catch (const std::exception& e) {
                std::cerr << "Exception creating GameClient: " << e.what() << std::endl;
                delete gameClient;
                gameClient = nullptr;
                return false;
            }
        }

        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in initialize: " << e.what() << std::endl;
        // Clean up any partially initialized resources
        delete gameServer;
        gameServer = nullptr;
        delete gameClient;
        gameClient = nullptr;
        return false;
    }
    catch (...) {
        std::cerr << "Unknown exception in initialize" << std::endl;
        // Clean up any partially initialized resources
        delete gameServer;
        gameServer = nullptr;
        delete gameClient;
        gameClient = nullptr;
        return false;
    }
}

void NetworkWrapper::update(float deltaTime)
{
    try {
        // Update network manager
        networkManager.update();

        // Update game components
        if (isHost && gameServer) {
            gameServer->update(deltaTime);

            // Send game state to clients
            networkManager.sendGameState(gameServer->getGameState());
        }
        else if (!isHost && gameClient) {
            gameClient->update(deltaTime);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in NetworkWrapper::update: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown exception in NetworkWrapper::update" << std::endl;
    }
}