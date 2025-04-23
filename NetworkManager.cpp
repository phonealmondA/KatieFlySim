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
    if (!connected) return;

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
                client->send(heartbeatPacket);
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

            // Create a new player for this client
            if (gameServer) {
                sf::Vector2f spawnPos = gameServer->getPlanets()[0]->getPosition() +
                    sf::Vector2f(0, -(gameServer->getPlanets()[0]->getRadius() + GameConstants::ROCKET_SIZE + 30.0f));
                gameServer->addPlayer(clientId, spawnPos, sf::Color::Red);
            }

            std::cout << "New client connected with ID: " << clientId << std::endl;
        }
        else {
            delete newClient;
        }

        // Check for messages from clients
        for (size_t i = 0; i < clients.size(); ++i) {
            sf::Packet packet;
            sf::Socket::Status status = clients[i]->receive(packet);

            if (status == sf::Socket::Status::Done) {
                lastPacketTime.restart();

                // Parse message type
                uint32_t msgType;
                packet >> msgType;

                if (msgType == MSG_PLAYER_INPUT) {
                    PlayerInput input;
                    packet >> input;

                    if (onPlayerInputReceived) {
                        // Debug output
                        std::cout << "Server received input from player ID: " << input.playerId
                            << " W/Up:" << input.thrustForward
                            << " S/Down:" << input.thrustBackward
                            << " A/Left:" << input.rotateLeft
                            << " D/Right:" << input.rotateRight << std::endl;

                        // Use the playerId from the input packet
                        onPlayerInputReceived(input.playerId, input);
                    }
                }
                else if (msgType == MSG_HEARTBEAT) {
                    // Just a keep-alive, no action needed
                }
                else if (msgType == MSG_DISCONNECT) {
                    std::cout << "Client " << i + 1 << " has disconnected" << std::endl;

                    // Remove the client's player from the game
                    if (gameServer) {
                        gameServer->removePlayer(static_cast<int>(i) + 1);
                    }

                    // Clean up the socket
                    clients[i]->disconnect();
                    delete clients[i];
                    clients.erase(clients.begin() + i);
                    i--; // Adjust index since we removed an element
                }
            }
            else if (status == sf::Socket::Status::Disconnected) {
                std::cout << "Client " << i + 1 << " has disconnected" << std::endl;

                // Remove the client's player from the game
                if (gameServer) {
                    gameServer->removePlayer(static_cast<int>(i) + 1);
                }

                // Clean up the socket
                delete clients[i];
                clients.erase(clients.begin() + i);
                i--; // Adjust index since we removed an element
            }
        }

        // Periodically send updated game state to all clients
        static sf::Clock updateClock;
        if (updateClock.getElapsedTime().asMilliseconds() > 50) { // 20 updates per second
            updateClock.restart();

            if (gameServer) {
                GameState state = gameServer->getGameState();
                sendGameState(state);
            }
        }
    }
    else {
        // Check for messages from server
        sf::Packet packet;
        sf::Socket::Status status = serverConnection.receive(packet);

        if (status == sf::Socket::Status::Done) {
            lastPacketTime.restart();

            uint32_t msgType;
            packet >> msgType;

            if (msgType == MSG_PLAYER_ID) {
                uint32_t playerId;
                packet >> playerId;
                if (gameClient) {
                    gameClient->setLocalPlayerId(static_cast<int>(playerId));
                    std::cout << "Received player ID from server: " << playerId << std::endl;
                }
            }
            else if (msgType == MSG_GAME_STATE) {
                // Measure ping
                static sf::Clock pingClock;
                pingMs = pingClock.restart().asMilliseconds();

                // Parse game state from packet
                GameState state;
                packet >> state;

                if (onGameStateReceived) {
                    onGameStateReceived(state);
                }
            }
            else if (msgType == MSG_HEARTBEAT) {
                // Just a keep-alive, no action needed
            }
            else if (msgType == MSG_DISCONNECT) {
                std::cout << "Disconnected from server" << std::endl;
                connected = false;
                serverConnection.disconnect();
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

    sf::Packet packet;
    packet << static_cast<uint32_t>(MSG_PLAYER_INPUT) << input;

    sf::Socket::Status status = serverConnection.send(packet);
    if (status != sf::Socket::Status::Done) {
        packetLossCounter++;
        return false;
    }

    return true;
}

float NetworkManager::getPing() const {
    return static_cast<float>(pingMs);
}

int NetworkManager::getPacketLoss() const {
    return packetLossCounter;
}