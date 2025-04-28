#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include "GameManager.h"
#include "MenuSystem.h"
#include "NetworkWrapper.h"
#include "UIManager.h"
#include "InputManager.h"
#include "TextPanel.h"
#include "OrbitalMechanics.h"
#include <iostream>
#include <string>

#ifdef _DEBUG
#pragma comment(lib, "sfml-graphics-d.lib")
#pragma comment(lib, "sfml-window-d.lib")
#pragma comment(lib, "sfml-system-d.lib")
#pragma comment(lib, "sfml-network-d.lib")
#else
#pragma comment(lib, "sfml-graphics.lib")
#pragma comment(lib, "sfml-window.lib")
#pragma comment(lib, "sfml-system.lib")
#pragma comment(lib, "sfml-network.lib")
#endif

#ifdef _WIN32
#include <windows.h>
#endif

// Safety function to check pointer validity
template<typename T>
bool isValidPtr(T* ptr) {
    return ptr != nullptr;
}

// Parse command line arguments for multiplayer setup
bool parseCommandLine(int argc, char* argv[], bool& isMultiplayer, bool& isHost,
    std::string& address, unsigned short& port) {
    if (argc < 2) return false;

    std::string mode = argv[1];
    if (mode == "--host") {
        isMultiplayer = true;
        isHost = true;
        port = 5000;
        if (argc >= 3) {
            port = static_cast<unsigned short>(std::stoi(argv[2]));
        }
        return true;
    }
    else if (mode == "--join") {
        isMultiplayer = true;
        isHost = false;
        if (argc < 3) return false;
        address = argv[2];
        port = 5000;
        if (argc >= 4) {
            port = static_cast<unsigned short>(std::stoi(argv[3]));
        }
        return true;
    }

    return false;
}

// Define an enum to track connection state
enum class AppConnectionState {
    DISCONNECTED,
    CONNECTING,
    WAITING_FOR_DATA,
    CONNECTED
};

int main(int argc, char* argv[])
{
    // Initialize SFML window
    sf::RenderWindow window(sf::VideoMode({ 1280, 720 }), "Noah's Flight Sim");

    // Load font
    sf::Font font;
    bool fontLoaded = false;

    // Try to load from common locations based on platform
#ifdef _WIN32
    // Get the Windows directory path
    char winDirPath[MAX_PATH];
    GetWindowsDirectoryA(winDirPath, MAX_PATH);
    std::string fontPath = std::string(winDirPath) + "\\Fonts\\arial.ttf";

    // Try Windows font directory first, then fall back to other locations
    if (font.openFromFile(fontPath) ||
        font.openFromFile("C:/Windows/Fonts/arial.ttf") ||
        font.openFromFile("C:/Windows/Fonts/Arial.ttf") ||
        font.openFromFile("arial.ttf")) {
        fontLoaded = true;
    }
#elif defined(__APPLE__)
    if (font.openFromFile("/Library/Fonts/Arial.ttf") ||
        font.openFromFile("/System/Library/Fonts/Arial.ttf") ||
        font.openFromFile("arial.ttf")) {
        fontLoaded = true;
    }
#elif defined(__linux__)
    if (font.openFromFile("/usr/share/fonts/truetype/msttcorefonts/Arial.ttf") ||
        font.openFromFile("/usr/share/fonts/TTF/arial.ttf") ||
        font.openFromFile("arial.ttf")) {
        fontLoaded = true;
    }
#else
    fontLoaded = font.openFromFile("arial.ttf");
#endif

    if (!fontLoaded) {
        std::cerr << "Warning: Could not load font file. Text won't display correctly." << std::endl;
    }

    // Parse command line arguments
    bool isMultiplayer = false;
    bool isHost = false;
    std::string address = "";
    unsigned short port = 5000;
    bool skipMenu = parseCommandLine(argc, argv, isMultiplayer, isHost, address, port);

    // Variable to store the game state
    MenuGameState currentState;

    // Only show menu if not started with command line arguments
    if (!skipMenu) {
        // Initialize menu system
        MenuSystem menuSystem(window, font);

        // Run menu and get the selected game state
        currentState = menuSystem.run();

        // If the window was closed during menu, exit
        if (!window.isOpen()) {
            return 0;
        }

        // Handle state transitions from menu
        if (currentState == MenuGameState::SINGLE_PLAYER) {
            isMultiplayer = false;
        }
        else if (currentState == MenuGameState::MULTIPLAYER_HOST) {
            isMultiplayer = true;
            isHost = true;
            port = 5000; // Default port for host
        }
        else if (currentState == MenuGameState::MULTIPLAYER_CLIENT) {
            isMultiplayer = true;
            isHost = false;
            address = menuSystem.getServerAddress();
            port = menuSystem.getServerPort();
        }
        else {
            // Window was closed or invalid state
            return 0;
        }
    }
    else {
        // Set game state based on command line arguments
        if (isMultiplayer && isHost) {
            currentState = MenuGameState::MULTIPLAYER_HOST;
        }
        else if (isMultiplayer && !isHost) {
            currentState = MenuGameState::MULTIPLAYER_CLIENT;
        }
        else {
            currentState = MenuGameState::SINGLE_PLAYER;
        }
    }

    // Update window title based on game mode
    std::string windowTitle = "Noah's Space Program";
    if (isMultiplayer) {
        windowTitle += isHost ? " (Server)" : " (Client)";
    }
    window.setTitle(windowTitle);

    // Initialize the app connection state
    AppConnectionState connectionState = isMultiplayer && !isHost ?
        AppConnectionState::DISCONNECTED :
        AppConnectionState::CONNECTED;

    // Initialize single player components first in all cases
    GameManager gameManager(window);
    UIManager uiManager(window, font, gameManager.getUIView(), isMultiplayer, isHost);
    gameManager.setUIManager(&uiManager);

    // For single player mode, initialize the game manager
    if (!isMultiplayer) {
        gameManager.initialize();
    }

    // Create pointers for game components
    VehicleManager* activeVehicleManager = nullptr;
    std::vector<Planet*> planets;
    NetworkWrapper networkWrapper;

    // Initialize multiplayer if needed
    if (isMultiplayer) {
        std::cout << "Initializing network in " << (isHost ? "host" : "client") << " mode..." << std::endl;
        connectionState = AppConnectionState::CONNECTING;

        try {
            if (!networkWrapper.initialize(isHost, address, port)) {
                std::cerr << "Failed to initialize network. Falling back to single player mode." << std::endl;
                isMultiplayer = false;
                isHost = false;
                connectionState = AppConnectionState::DISCONNECTED;
                gameManager.initialize();
                activeVehicleManager = gameManager.getActiveVehicleManager();
                planets = gameManager.getPlanets();
            }
            else {
                std::cout << "Network connection established." << std::endl;
                connectionState = AppConnectionState::WAITING_FOR_DATA;

                // Setup multiplayer components based on role
                if (isHost) {
                    // Server mode - use GameServer's objects
                    GameServer* gameServer = networkWrapper.getServer();
                    if (gameServer) {
                        gameServer->initialize(); // Always initialize the server
                        planets = gameServer->getPlanets();
                        activeVehicleManager = gameServer->getPlayer(0);

                        if (planets.empty() || !activeVehicleManager) {
                            std::cerr << "Warning: Invalid game objects in host mode." << std::endl;

                            if (!activeVehicleManager && !planets.empty()) {
                                std::cerr << "Creating player for server" << std::endl;
                                sf::Vector2f initialPos = planets[0]->getPosition() +
                                    sf::Vector2f(0, -(planets[0]->getRadius() + 50.0f));
                                gameServer->addPlayer(0, initialPos);
                                activeVehicleManager = gameServer->getPlayer(0);
                            }
                        }

                        // Host is always considered connected
                        connectionState = AppConnectionState::CONNECTED;
                    }
                    else {
                        std::cerr << "GameServer is null. Falling back to single player." << std::endl;
                        isMultiplayer = false;
                        isHost = false;
                        connectionState = AppConnectionState::DISCONNECTED;
                        gameManager.initialize();
                        activeVehicleManager = gameManager.getActiveVehicleManager();
                        planets = gameManager.getPlanets();
                    }
                }
                else {
                    // Client mode - remains in WAITING_FOR_DATA until data is received
                    // activeVehicleManager and planets will be updated later when data is received
                    std::cout << "Client connected, waiting for data..." << std::endl;
                }
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Exception during network initialization: " << e.what() << std::endl;
            std::cerr << "Falling back to single player mode." << std::endl;
            isMultiplayer = false;
            isHost = false;
            connectionState = AppConnectionState::DISCONNECTED;
            gameManager.initialize();
            activeVehicleManager = gameManager.getActiveVehicleManager();
            planets = gameManager.getPlanets();
        }
    }
    else {
        // Single player mode - get objects from game manager
        activeVehicleManager = gameManager.getActiveVehicleManager();
        planets = gameManager.getPlanets();
        connectionState = AppConnectionState::CONNECTED;
    }

    // For single-player or host, we should have valid objects by now
    if ((!isMultiplayer || isHost) && (!activeVehicleManager || planets.empty())) {
        std::cerr << "Failed to initialize game objects. Falling back to single player." << std::endl;
        isMultiplayer = false;
        isHost = false;
        connectionState = AppConnectionState::DISCONNECTED;
        gameManager.initialize();
        activeVehicleManager = gameManager.getActiveVehicleManager();
        planets = gameManager.getPlanets();
        connectionState = AppConnectionState::CONNECTED;
    }

    // Initialize input manager
    InputManager inputManager(isMultiplayer, isHost);

    // Clock for tracking time between frames
    sf::Clock clock;
    sf::Clock connectionTimeoutClock;

    // Connection timeout parameters
    const float connectionTimeout = 10.0f; // 10 seconds to establish connection
    const int maxConnectionAttempts = 20;
    int connectionAttempts = 0;

    // Main game loop
    while (window.isOpen())
    {
        // Calculate delta time (limit to avoid physics issues)
        float deltaTime = std::min(clock.restart().asSeconds(), 0.1f);

        // Handle network updates first
        if (isMultiplayer) {
            try {
                networkWrapper.update(deltaTime);

                // Check if client connection state has changed
                if (!isHost && connectionState != AppConnectionState::CONNECTED) {
                    GameClient* gameClient = networkWrapper.getClient();
                    bool networkConnected = networkWrapper.getNetworkManager()->isConnected();

                    if (networkConnected && gameClient) {
                        if (gameClient->getLocalPlayer() && !gameClient->getPlanets().empty()) {
                            // We have a local player and planets - transition to connected
                            activeVehicleManager = gameClient->getLocalPlayer();
                            planets = gameClient->getPlanets();
                            connectionState = AppConnectionState::CONNECTED;
                            std::cout << "Client fully connected with player and planet data" << std::endl;
                        }
                        else if (connectionState == AppConnectionState::CONNECTING) {
                            // Network is connected but waiting for data
                            connectionState = AppConnectionState::WAITING_FOR_DATA;
                        }
                    }

                    // Check for connection timeout
                    if (connectionState != AppConnectionState::CONNECTED) {
                        connectionAttempts++;

                        if (connectionTimeoutClock.getElapsedTime().asSeconds() > connectionTimeout ||
                            connectionAttempts > maxConnectionAttempts) {
                            std::cerr << "Connection timeout, falling back to single player" << std::endl;
                            isMultiplayer = false;
                            isHost = false;
                            connectionState = AppConnectionState::DISCONNECTED;
                            gameManager.initialize();
                            activeVehicleManager = gameManager.getActiveVehicleManager();
                            planets = gameManager.getPlanets();
                            connectionState = AppConnectionState::CONNECTED;
                        }
                    }
                }

                // Update UI with multiplayer info
                if (isHost) {
                    GameServer* gameServer = networkWrapper.getServer();
                    if (gameServer) {
                        uiManager.updateMultiplayerInfo(
                            gameServer->getPlayers().size() - 1,
                            networkWrapper.getNetworkManager()->isConnected(),
                            0,
                            networkWrapper.getNetworkManager()->getPing());
                    }
                    else {
                        uiManager.updateMultiplayerInfo(0, false, 0, 0);
                    }
                }
                else {
                    GameClient* gameClient = networkWrapper.getClient();
                    if (gameClient) {
                        uiManager.updateMultiplayerInfo(
                            gameClient->getRemotePlayers().size(),
                            networkWrapper.getNetworkManager()->isConnected(),
                            gameClient->getLocalPlayerId(),
                            networkWrapper.getNetworkManager()->getPing());
                    }
                    else {
                        uiManager.updateMultiplayerInfo(0, false, 0, 0);
                    }
                }
            }
            catch (const std::exception& e) {
                std::cerr << "Exception in network update: " << e.what() << std::endl;
            }
        }

        // Show connection UI if not fully connected
        if (connectionState != AppConnectionState::CONNECTED) {
            window.clear(sf::Color(20, 20, 50)); // Dark blue background

            sf::Text connectingText;
            connectingText.setFont(font);

            switch (connectionState) {
            case AppConnectionState::CONNECTING:
                connectingText.setString("Connecting to server...");
                break;
            case AppConnectionState::WAITING_FOR_DATA:
                connectingText.setString("Waiting for game data...");
                break;
            default:
                connectingText.setString("Connecting...");
                break;
            }

            connectingText.setCharacterSize(24);
            connectingText.setFillColor(sf::Color::White);

            // Center the text
            sf::FloatRect textBounds = connectingText.getLocalBounds();
            connectingText.setPosition(
                (window.getSize().x - textBounds.width) / 2,
                (window.getSize().y - textBounds.height) / 2
            );

            window.draw(connectingText);

            // Add connection status info
            sf::Text statusText;
            statusText.setFont(font);
            statusText.setCharacterSize(16);
            statusText.setFillColor(sf::Color(200, 200, 200));

            std::string statusMessage = "Attempt " + std::to_string(connectionAttempts) +
                " of " + std::to_string(maxConnectionAttempts);
            statusText.setString(statusMessage);

            sf::FloatRect statusBounds = statusText.getLocalBounds();
            statusText.setPosition(
                (window.getSize().x - statusBounds.width) / 2,
                (window.getSize().y - statusBounds.height) / 2 + 40
            );

            window.draw(statusText);
            window.display();

            // Skip the rest of the loop until we're connected
            continue;
        }

        // Update safety checks for objects (only when connected)
        if (isMultiplayer) {
            if (isHost) {
                GameServer* gameServer = networkWrapper.getServer();
                if (gameServer && (!activeVehicleManager || planets.empty())) {
                    // Try to get updated references
                    planets = gameServer->getPlanets();
                    activeVehicleManager = gameServer->getPlayer(0);
                }
            }
            else { // Client
                GameClient* gameClient = networkWrapper.getClient();
                if (gameClient) {
                    // Refresh references
                    planets = gameClient->getPlanets();
                    activeVehicleManager = gameClient->getLocalPlayer();
                }
            }
        }

        // Final pointer safety check before proceeding
        if (!activeVehicleManager || planets.empty()) {
            std::cerr << "Missing game objects despite connection, skipping frame" << std::endl;
            continue;
        }

        // Handle events
        try {
            gameManager.handleEvents();
        }
        catch (const std::exception& e) {
            std::cerr << "Exception in event handling: " << e.what() << std::endl;
        }

        // Process input for controlling the vehicle
        if (!isMultiplayer || isHost) {
            try {
                if (activeVehicleManager) {
                    inputManager.processInput(activeVehicleManager, deltaTime);
                }
            }
            catch (const std::exception& e) {
                std::cerr << "Exception in input processing: " << e.what() << std::endl;
            }
        }
        else if (isMultiplayer && !isHost) {
            // Client-side input processing
            try {
                GameClient* gameClient = networkWrapper.getClient();
                if (gameClient && gameClient->getLocalPlayer()) {
                    // Get and send player input to server
                    PlayerInput input = gameClient->getLocalPlayerInput(deltaTime);
                    // Apply input locally for responsive feel
                    gameClient->applyLocalInput(input);
                    // Send to server
                    networkWrapper.getNetworkManager()->sendPlayerInput(input);
                }
            }
            catch (const std::exception& e) {
                std::cerr << "Exception in client input processing: " << e.what() << std::endl;
            }
        }

        // Update game simulation
        if (!isMultiplayer || isHost) {
            try {
                gameManager.update(deltaTime);
                planets = gameManager.getPlanets();
            }
            catch (const std::exception& e) {
                std::cerr << "Exception in game update: " << e.what() << std::endl;
            }
        }

        // Update UI information - only with valid objects
        try {
            if (activeVehicleManager && !planets.empty()) {
                uiManager.update(activeVehicleManager, planets, deltaTime);
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Exception in UI update: " << e.what() << std::endl;
        }

        // Render scene
        try {
            // Clear the window with a dark background
            window.clear(sf::Color(20, 20, 50));

            // Render game elements
            gameManager.render();

            // Draw trajectory and other elements safely
            if (activeVehicleManager &&
                activeVehicleManager->getActiveVehicleType() == VehicleType::ROCKET &&
                activeVehicleManager->getRocket()) {
                try {
                    activeVehicleManager->getRocket()->drawTrajectory(window, planets,
                        GameConstants::TRAJECTORY_TIME_STEP, GameConstants::TRAJECTORY_STEPS, false);
                }
                catch (const std::exception& e) {
                    std::cerr << "Exception drawing trajectory: " << e.what() << std::endl;
                }
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Exception in rendering: " << e.what() << std::endl;
        }

        // Render UI
        try {
            uiManager.render();
        }
        catch (const std::exception& e) {
            std::cerr << "Exception in UI rendering: " << e.what() << std::endl;
        }

        // Display the frame
        window.display();
    }

    // Cleanup happens automatically via destructors
    return 0;
}