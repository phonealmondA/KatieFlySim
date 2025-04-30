// NetworkManager.cpp
#include "NetworkManager.h"
#include "GameServer.h"
#include "GameClient.h"
#include <iostream>
#include <thread>
#include <chrono>

NetworkManager::NetworkManager()
    : isHost(false),
    port(0),
    connected(false),
    gameServer(nullptr),
    gameClient(nullptr),
    lastPacketTime(),
    packetLossCounter(0),
    pingMs(0),
    connectionState(ConnectionState::DISCONNECTED) {

    // Initialize callbacks to nullptr to avoid calling uninitialized functions
    onPlayerInputReceived = nullptr;
    onGameStateReceived = nullptr;

    // Initialize network components
    lastPacketTime.restart();
}

NetworkManager::~NetworkManager() {
    try {
        disconnect();
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in NetworkManager destructor: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown exception in NetworkManager destructor" << std::endl;
    }
}

void NetworkManager::setGameServer(GameServer* server) {
    gameServer = server;
}

void NetworkManager::setGameClient(GameClient* client) {
    gameClient = client;
}

bool NetworkManager::hostGame(unsigned short port) {
    try {
        this->port = port;
        isHost = true;
        connectionState = ConnectionState::CONNECTING;

        // Start listening for connections
        if (listener.listen(port) != sf::Socket::Status::Done) {
            std::cerr << "Failed to bind to port " << port << std::endl;
            connectionState = ConnectionState::DISCONNECTED;
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
        try {
            auto publicIp = sf::IpAddress::getPublicAddress(sf::seconds(2));
            if (publicIp) {
                std::cout << "Public IP address: " << publicIp->toString() << std::endl;
            }
            else {
                std::cout << "Could not determine public IP address" << std::endl;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error getting public IP: " << e.what() << std::endl;
        }

        listener.setBlocking(false);
        connected = true;
        connectionState = ConnectionState::CONNECTED;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in hostGame: " << e.what() << std::endl;
        connected = false;
        connectionState = ConnectionState::DISCONNECTED;
        return false;
    }
}

bool NetworkManager::joinGame(const sf::IpAddress& address, unsigned short port) {
    try {
        isHost = false;
        connectionState = ConnectionState::CONNECTING;

        std::cout << "Connecting to " << address.toString() << ":" << port << "..." << std::endl;

        // Set a timeout for connection attempts
        serverConnection.setBlocking(true);
        sf::Socket::Status status = serverConnection.connect(address, port, sf::seconds(5));
        serverConnection.setBlocking(false);

        if (status != sf::Socket::Status::Done) {
            std::cerr << "Failed to connect to " << address.toString() << ":" << port << std::endl;
            connectionState = ConnectionState::DISCONNECTED;
            return false;
        }

        std::cout << "Successfully connected to server!" << std::endl;
        connected = true;
        connectionState = ConnectionState::AUTHENTICATING; // Move to authenticating until we get player ID
        lastPacketTime.restart();
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in joinGame: " << e.what() << std::endl;
        connected = false;
        connectionState = ConnectionState::DISCONNECTED;
        return false;
    }
}

void NetworkManager::disconnect() {
    try {
        if (connected) {
            // Send disconnect message
            if (!isHost) {
                try {
                    sf::Packet disconnectPacket;
                    disconnectPacket << static_cast<uint32_t>(static_cast<int>(MessageType::DISCONNECT));
                    serverConnection.send(disconnectPacket);
                }
                catch (...) {
                    // Ignore errors when trying to send disconnect message
                }
            }
        }

        if (isHost) {
            try {
                listener.close();
            }
            catch (...) {
                // Ignore errors when closing listener
            }

            // Disconnect all clients
            for (auto client : clients) {
                if (client) {
                    try {
                        // Send disconnect message to clients
                        sf::Packet disconnectPacket;
                        disconnectPacket << static_cast<uint32_t>(static_cast<int>(MessageType::DISCONNECT));
                        client->send(disconnectPacket);

                        client->disconnect();
                        delete client;
                    }
                    catch (...) {
                        // Ignore errors and continue cleanup
                    }
                }
            }
            clients.clear();
        }
        else {
            try {
                serverConnection.disconnect();
            }
            catch (...) {
                // Ignore errors when disconnecting
            }
        }

        connected = false;
        connectionState = ConnectionState::DISCONNECTED;
        std::cout << "Disconnected from network" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in disconnect: " << e.what() << std::endl;
        // Force disconnect state even if there was an error
        connected = false;
        connectionState = ConnectionState::DISCONNECTED;
    }
    catch (...) {
        std::cerr << "Unknown exception in disconnect" << std::endl;
        // Force disconnect state even if there was an error
        connected = false;
        connectionState = ConnectionState::DISCONNECTED;
    }
}

void NetworkManager::enableRobustNetworking() {
    try {
        // Set non-blocking sockets with timeouts
        if (isHost) {
            for (auto* client : clients) {
                if (client) {
                    client->setBlocking(false);
                }
            }
        }
        else {
            serverConnection.setBlocking(false);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in enableRobustNetworking: " << e.what() << std::endl;
    }
}

void NetworkManager::update() {
    try {
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
            try {
                sf::Packet heartbeatPacket;
                heartbeatPacket << static_cast<uint32_t>(static_cast<int>(MessageType::HEARTBEAT));

                if (isHost) {
                    for (auto client : clients) {
                        if (client) {
                            sf::Socket::Status status = client->send(heartbeatPacket);
                            if (status != sf::Socket::Status::Done) {
                                // Non-fatal error, just increment packet loss counter
                                packetLossCounter++;
                            }
                        }
                    }
                }
                else {
                    sf::Socket::Status status = serverConnection.send(heartbeatPacket);
                    if (status != sf::Socket::Status::Done) {
                        // Non-fatal error, just increment packet loss counter
                        packetLossCounter++;
                    }
                }
            }
            catch (const std::exception& e) {
                std::cerr << "Exception sending heartbeat: " << e.what() << std::endl;
            }

            heartbeatClock.restart();
        }

        if (isHost) {
            // Accept new connections
            try {
                sf::TcpSocket* newClient = new sf::TcpSocket();
                sf::Socket::Status status = listener.accept(*newClient);

                if (status == sf::Socket::Status::Done) {
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
                    idPacket << static_cast<uint32_t>(static_cast<int>(MessageType::PLAYER_ID)) << static_cast<uint32_t>(clientId);
                    sf::Socket::Status sendStatus = newClient->send(idPacket);

                    if (sendStatus != sf::Socket::Status::Done) {
                        std::cerr << "Failed to send player ID to client" << std::endl;
                    }

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
                    // No new connection, clean up allocated socket
                    delete newClient;
                }
            }
            catch (const std::exception& e) {
                std::cerr << "Exception accepting new connection: " << e.what() << std::endl;
            }

            // Check for messages from clients
            for (size_t i = 0; i < clients.size(); i++) {
                sf::TcpSocket* client = clients[i];

                if (!client) {
                    continue;
                }

                try {
                    sf::Packet packet;
                    sf::Socket::Status status = client->receive(packet);

                    if (status == sf::Socket::Status::Done) {
                        if (packet.getDataSize() > 0) {
                            uint32_t msgType;
                            if (packet >> msgType) {
                                // Client ID is index+1
                                int clientId = static_cast<int>(i + 1);

                                switch (static_cast<MessageType>(msgType)) {
                                case MessageType::PLAYER_INPUT:
                                {
                                    PlayerInput input;
                                    if (packet >> input) {
                                        // Override the player ID with the client ID for security
                                        input.playerId = clientId;

                                        if (onPlayerInputReceived) {
                                            onPlayerInputReceived(clientId, input);
                                        }
                                    }
                                    break;
                                }
                                case MessageType::DISCONNECT:
                                    std::cout << "Client " << clientId << " requested disconnect" << std::endl;
                                    // Handle client disconnect - clean up client socket and game resources
                                    client->disconnect();
                                    delete client;
                                    clients[i] = nullptr;

                                    if (gameServer) {
                                        gameServer->removePlayer(clientId);
                                    }
                                    break;

                                default:
                                    std::cerr << "Received unknown message type from client: " << msgType << std::endl;
                                    break;
                                }
                            }
                        }
                    }
                    else if (status == sf::Socket::Status::Disconnected) {
                        int clientId = static_cast<int>(i + 1);
                        std::cout << "Client " << clientId << " disconnected" << std::endl;

                        // Clean up client socket and game resources
                        delete client;
                        clients[i] = nullptr;

                        if (gameServer) {
                            gameServer->removePlayer(clientId);
                        }
                    }
                }
                catch (const std::exception& e) {
                    std::cerr << "Exception processing client message: " << e.what() << std::endl;
                }
            }

            // Remove null client pointers from the vector
            clients.erase(
                std::remove_if(clients.begin(), clients.end(),
                    [](sf::TcpSocket* client) { return client == nullptr; }),
                clients.end()
            );
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
                        switch (static_cast<MessageType>(msgType)) {
                        case MessageType::PLAYER_ID:
                        {
                            uint32_t playerId;
                            if (packet >> playerId) {
                                if (gameClient) {
                                    std::cout << "Received player ID from server: " << playerId << std::endl;

                                    // Set the player ID and update connection state
                                    gameClient->setLocalPlayerId(static_cast<int>(playerId));

                                    // Explicitly transition to waiting for state
                                    connectionState = ConnectionState::CONNECTED;
                                    std::cout << "Connection state updated to waiting for game state" << std::endl;
                                }
                                else {
                                    std::cerr << "Error: Received player ID but gameClient is null" << std::endl;
                                }
                            }
                        }
                        break;
                        break;
                        case MessageType::GAME_STATE:
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
                        case MessageType::HEARTBEAT:
                            // Just a keep-alive, no action needed
                            break;
                        case MessageType::DISCONNECT:
                            std::cout << "Disconnected from server" << std::endl;
                            connected = false;
                            connectionState = ConnectionState::DISCONNECTED;
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
                connectionState = ConnectionState::DISCONNECTED;
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in update: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown exception in update" << std::endl;
    }
}

bool NetworkManager::sendGameState(const GameState& state) {
    if (!isHost || !connected) return false;

    try {
        sf::Packet packet;
        packet << static_cast<uint32_t>(static_cast<int>(MessageType::GAME_STATE)) << state;

        bool allSucceeded = true;

        for (auto client : clients) {
            if (!client) continue;

            sf::Socket::Status status = client->send(packet);
            if (status != sf::Socket::Status::Done) {
                allSucceeded = false;
                packetLossCounter++;
            }
        }

        return allSucceeded;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in sendGameState: " << e.what() << std::endl;
        return false;
    }
}

bool NetworkManager::sendPlayerInput(const PlayerInput& input) {
    if (isHost || !connected) return false;

    try {
        sf::Packet packet;
        packet << static_cast<uint32_t>(static_cast<int>(MessageType::PLAYER_INPUT)) << input;

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