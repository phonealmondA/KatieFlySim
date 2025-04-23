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

    // Initialize network system if multiplayer
    NetworkWrapper networkWrapper;

    if (isMultiplayer) {
        if (!networkWrapper.initialize(isHost, address, port)) {
            std::cerr << "Failed to initialize network. Exiting." << std::endl;
            return 1;
        }

        std::cout << "Network connection established. Running in "
            << (isHost ? "server" : "client") << " mode." << std::endl;
    }

    // Initialize game manager
    GameManager gameManager(window);

    // For multiplayer, we'll need to connect to the network components
    VehicleManager* activeVehicleManager = nullptr;
    std::vector<Planet*> planets;

    if (isMultiplayer) {
        if (isHost) {
            // Server mode - use GameServer's objects
            GameServer* gameServer = networkWrapper.getServer();
            planets = gameServer->getPlanets();
            activeVehicleManager = gameServer->getPlayer(0);
        }
        else {
            // Client mode - use GameClient's objects
            GameClient* gameClient = networkWrapper.getClient();
            planets = gameClient->getPlanets();
            activeVehicleManager = gameClient->getLocalPlayer();
        }
    }
    else {
        // Single player mode - initialize game manager
        gameManager.initialize();
        activeVehicleManager = gameManager.getActiveVehicleManager();
        planets = gameManager.getPlanets();
    }

    // Initialize UI manager
    UIManager uiManager(window, font, gameManager.getUIView(), isMultiplayer, isHost);

    // Initialize input manager
    InputManager inputManager(isMultiplayer, isHost);

    // Clock for tracking time between frames
    sf::Clock clock;

    // Main game loop
    while (window.isOpen())
    {
        // Calculate delta time (limit to avoid physics issues)
        float deltaTime = std::min(clock.restart().asSeconds(), 0.1f);

        // Update network if multiplayer
        if (isMultiplayer) {
            networkWrapper.update(deltaTime);

            // Update multiplayer info in UI
            if (isHost) {
                GameServer* gameServer = networkWrapper.getServer();
                uiManager.updateMultiplayerInfo(
                    gameServer ? gameServer->getPlayers().size() - 1 : 0,
                    networkWrapper.getNetworkManager()->isConnected(),
                    0,
                    networkWrapper.getNetworkManager()->getPing());
            }
            else {
                GameClient* gameClient = networkWrapper.getClient();
                uiManager.updateMultiplayerInfo(
                    gameClient->getRemotePlayers().size(),
                    networkWrapper.getNetworkManager()->isConnected(),
                    gameClient->getLocalPlayerId(),
                    networkWrapper.getNetworkManager()->getPing());
            }
        }

        // Handle events (window events, keyboard, etc.)
        gameManager.handleEvents();

        // Process input for controlling the vehicle
        if (!isMultiplayer || isHost) {
            inputManager.processInput(activeVehicleManager, deltaTime);
        }

        // Update game simulation
        if (!isMultiplayer || isHost) {
            gameManager.update(deltaTime);
        }

        // Update UI information
        uiManager.update(activeVehicleManager, planets, deltaTime);

        // Render scene
        gameManager.render();

        // Render UI
        uiManager.render();

        // Display the frame
        window.display();
    }

    // Cleanup happens automatically via destructors for our manager classes
    return 0;
}