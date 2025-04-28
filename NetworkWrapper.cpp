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
    // Clean up resources
    delete gameServer;
    delete gameClient;
}

bool NetworkWrapper::initialize(bool host, const std::string& address, unsigned short port)
{
    isMultiplayer = true;
    isHost = host;

    if (host) {
        // Server mode
        gameServer = new GameServer();
        if (!networkManager.hostGame(port)) {
            delete gameServer;
            gameServer = nullptr;
            return false;
        }
        networkManager.setGameServer(gameServer);
    }
    else {
        // Client mode
        gameClient = new GameClient();
        gameClient->initialize();

        // Convert string address to IpAddress
        auto serverAddressOpt = sf::IpAddress::resolve(address);
        if (!serverAddressOpt) {
            std::cerr << "Failed to resolve address: " << address << std::endl;
            delete gameClient;
            gameClient = nullptr;
            return false;
        }
        if (!networkManager.joinGame(*serverAddressOpt, port)) {
            delete gameClient;
            gameClient = nullptr;
            return false;
        }

        networkManager.setGameClient(gameClient);
    }

    return true;
}

void NetworkWrapper::update(float deltaTime)
{
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

// Removed the redundant getter implementations that were already defined in the header