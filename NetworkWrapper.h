// NetworkWrapper.h
#pragma once
#include "NetworkManager.h"
#include "GameServer.h"
#include "GameClient.h"

class NetworkWrapper {
private:
    NetworkManager networkManager;
    GameServer* gameServer;
    GameClient* gameClient;
    bool isMultiplayer;
    bool isHost;

public:
    NetworkWrapper();
    ~NetworkWrapper();

    bool initialize(bool host, const std::string& address = "", unsigned short port = 5000);
    void update(float deltaTime);

    // Getters
    bool isConnected() const { return networkManager.isConnected(); }
    int getPing() const { return networkManager.getPing(); }
    int getPacketLoss() const { return networkManager.getPacketLoss(); }

    // Access to components
    GameServer* getServer() { return gameServer; }
    GameClient* getClient() { return gameClient; }
    NetworkManager* getNetworkManager() { return &networkManager; }
};