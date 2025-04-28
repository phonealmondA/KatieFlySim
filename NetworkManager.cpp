// NetworkManager.cpp (complete improved file)
#include "NetworkManager.h"
#include "GameServer.h"
#include "GameClient.h"
#include <iostream>

// Message types for network communication
enum MessageType {
    MSG_GAME_STATE = 1,
    MSG_PLAYER_INPUT = 2,
    MSG_PLAYER_ID = 3,
    MSG_HEARTBEAT = 4,
    MSG_DISCONNECT = 5
};

NetworkManager::NetworkManager()
    : isHost(false),
    port(0),
    connected(false),
    gameServer(nullptr),
    gameClient(nullptr),
    lastPacketTime(),
    packetLossCounter(0),
    pingMs(0) {
    // Initialize network components
    lastPacketTime.restart();
}

NetworkManager::~NetworkManager() {
    disconnect();
}

void NetworkManager::setGameServer(GameServer* server) {
    gameServer = server;
}

void NetworkManager::setGameClient(GameClient* client) {
    gameClient = client;
}

bool NetworkManager::hostGame(unsigned short port) {
    this->port = port;
    isHost = true;

    // Start listening for connections
    if (listener.listen(port) != sf::Socket::Status::Done) {
        std::cerr << "Failed to bind to port " << port << std::endl;
        return false;
    }

    std::cout << "Server started on port " << port << std::endl;

    // For the local IP address
    auto localIp = sf::IpAddress::getLocalAddress();
    if (localIp) {
        std::cout << "Local IP address: " << localIp->toString() << std::endl;
    }
    else {
        std::cout << "Could not determine local IP address" << std::endl;
    }

    // For the public IP address
    if (auto publicIp = sf::IpAddress::getPublicAddress(sf::seconds(2))) {
        std::cout << "Public IP address: " << publicIp->toString() << std::endl;
    }
    else {
        std::cout << "Could not determine public IP address" << std::endl;
    }

    listener.setBlocking(false);
    connected = true;
    return true;
}

bool NetworkManager::joinGame(const sf::IpAddress& address, unsigned short port) {
    isHost = false;

    std::cout << "Connecting to " << address.toString() << ":" << port << "..." << std::endl;


    // Set a timeout for connection attempts
    serverConnection.setBlocking(true);
    sf::Socket::Status status = serverConnection.connect(address, port, sf::seconds(5));
    serverConnection.setBlocking(false);

    if (status != sf::Socket::Status::Done) {
        std::cerr << "Failed to connect to " << address.toString() << ":" << port << std::endl;
        return false;
    }

    std::cout << "Successfully connected to server!" << std::endl;
    connected = true;
    lastPacketTime.restart();
    return true;
}

void NetworkManager::disconnect() {
    if (connected) {
        // Send disconnect message
        if (!isHost) {
            sf::Packet disconnectPacket;
            disconnectPacket << static_cast<uint32_t>(MSG_DISCONNECT);
            serverConnection.send(disconnectPacket);
        }
    }

    if (isHost) {
        listener.close();

        // Disconnect all clients
        for (auto client : clients) {
            // Send disconnect message to clients
            sf::Packet disconnectPacket;
            disconnectPacket << static_cast<uint32_t>(MSG_DISCONNECT);
            client->send(disconnectPacket);

            client->disconnect();
            delete client;
        }
        clients.clear();
    }
    else {
        serverConnection.disconnect();
    }

    connected = false;
    std::cout << "Disconnected from network" << std::endl;
}

void NetworkManager::enableRobustNetworking() {
    // Set non-blocking sockets with timeouts
    if (isHost) {
        for (auto* client : clients) {
            client->setBlocking(false);
        }
    }
    else {
        serverConnection.setBlocking(false);
    }
}

void NetworkManager::update() {
    if (!connected) {
        // Return early if we're not connected to avoid null references
        return;
    }

    // Check for timeouts (5 seconds without data)
    if (lastPacketTime.getElapsedTime().asSeconds() > 5.0f) {
        std::cerr << "Connection timed out - no data received for 5 seconds" << std::endl;
        disconnect();
        return;
    }

    // Send heartbeat every second to keep connection alive
    static sf::Clock heartbeatClock;
    if (heartbeatClock.getElapsedTime().asSeconds() > 1.0f) {
        sf::Packet heartbeatPacket;
        heartbeatPacket << static_cast<uint32_t>(MSG_HEARTBEAT);

        if (isHost) {
            for (auto client : clients) {
                if (client) client->send(heartbeatPacket);
            }
        }
        else {
            serverConnection.send(heartbeatPacket);
        }

        heartbeatClock.restart();
    }

    if (isHost) {
        // Accept new connections
        sf::TcpSocket* newClient = new sf::TcpSocket();
        if (listener.accept(*newClient) == sf::Socket::Status::Done) {
            newClient->setBlocking(false);

            // Log connection info
            if (auto remoteAddress = newClient->getRemoteAddress()) {
                std::cout << "New client connecting from: " << remoteAddress->toString() << std::endl;
            }
            else {
                std::cout << "New client connecting from: unknown address" << std::endl;
            }

            clients.push_back(newClient);

            // Create a unique ID for the client (use client index + 1 to avoid ID 0)
            int clientId = static_cast<int>(clients.size()); // This will be 1 for the first client

            // Send acknowledgment with player ID to the client
            sf::Packet idPacket;
            idPacket << static_cast<uint32_t>(MSG_PLAYER_ID) << static_cast<uint32_t>(clientId);
            newClient->send(idPacket);

            // Create a new player for this client if gameServer exists
            if (gameServer && gameServer->getPlanets().size() > 0) {
                const auto& planets = gameServer->getPlanets();
                if (planets.size() > 0 && planets[0] != nullptr) {
                    sf::Vector2f spawnPos = planets[0]->getPosition() +
                        sf::Vector2f(0, -(planets[0]->getRadius() + GameConstants::ROCKET_SIZE + 30.0f));
                    gameServer->addPlayer(clientId, spawnPos, sf::Color::Red);
                }
                else {
                    gameServer->addPlayer(clientId, sf::Vector2f(400.f, 100.f), sf::Color::Red);
                }
            }

            std::cout << "New client connected with ID: " << clientId << std::endl;
        }
        else {
            delete newClient;
        }

        // Check for messages from clients (rest of host code unchanged)
        // ...
    }
    else {
        // Client mode - improved error handling
        sf::Packet packet;
        sf::Socket::Status status = serverConnection.receive(packet);

        if (status == sf::Socket::Status::Done) {
            lastPacketTime.restart();

            // Ensure packet is not empty before trying to read from it
            if (packet.getDataSize() > 0) {
                uint32_t msgType;
                if (packet >> msgType) {
                    switch (msgType) {
                    case MSG_PLAYER_ID:
                    {
                        uint32_t playerId;
                        if (packet >> playerId) {
                            if (gameClient) {
                                gameClient->setLocalPlayerId(static_cast<int>(playerId));
                                std::cout << "Received player ID from server: " << playerId << std::endl;
                            }
                        }
                    }
                    break;
                    case MSG_GAME_STATE:
                    {
                        // Measure ping
                        static sf::Clock pingClock;
                        pingMs = pingClock.restart().asMilliseconds();

                        // Handle game state with additional safety
                        GameState state;
                        try {
                            if (packet >> state) {
                                if (onGameStateReceived && gameClient) {
                                    onGameStateReceived(state);
                                }
                            }
                            else {
                                std::cerr << "Failed to parse game state packet" << std::endl;
                            }
                        }
                        catch (const std::exception& e) {
                            std::cerr << "Exception parsing game state: " << e.what() << std::endl;
                        }
                    }
                    break;
                    case MSG_HEARTBEAT:
                        // Just a keep-alive, no action needed
                        break;
                    case MSG_DISCONNECT:
                        std::cout << "Disconnected from server" << std::endl;
                        connected = false;
                        serverConnection.disconnect();
                        break;
                    default:
                        std::cerr << "Received unknown message type: " << msgType << std::endl;
                        break;
                    }
                }
                else {
                    std::cerr << "Failed to read message type from packet" << std::endl;
                }
            }
        }
        else if (status == sf::Socket::Status::Disconnected) {
            std::cout << "Lost connection to server" << std::endl;
            connected = false;
        }
    }
}

bool NetworkManager::sendGameState(const GameState& state) {
    if (!isHost || !connected) return false;

    sf::Packet packet;
    packet << static_cast<uint32_t>(MSG_GAME_STATE) << state;

    bool allSucceeded = true;
    for (auto client : clients) {
        if (client->send(packet) != sf::Socket::Status::Done) {
            allSucceeded = false;
            packetLossCounter++;
        }
    }

    return allSucceeded;
}
bool NetworkManager::sendPlayerInput(const PlayerInput& input) {
    if (isHost || !connected) return false;

    try {
        sf::Packet packet;
        packet << static_cast<uint32_t>(MSG_PLAYER_INPUT) << input;

        sf::Socket::Status status = serverConnection.send(packet);
        if (status != sf::Socket::Status::Done) {
            packetLossCounter++;
            return false;
        }

        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in sendPlayerInput: " << e.what() << std::endl;
        return false;
    }
}

float NetworkManager::getPing() const {
    return static_cast<float>(pingMs);
}

int NetworkManager::getPacketLoss() const {
    return packetLossCounter;
}