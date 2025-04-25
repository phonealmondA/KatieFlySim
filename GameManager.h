// GameManager.h
#pragma once
#include <SFML/Graphics.hpp>
#include "VehicleManager.h"
#include "GravitySimulator.h"
#include "NetworkManager.h"

// Forward declaration
class UIManager;

class GameManager {
private:
    sf::RenderWindow& window;
    sf::View gameView;
    sf::View uiView;
    float zoomLevel;
    float targetZoom;
    sf::Clock clock;

    // Game objects
    std::vector<Planet*> planets;
    VehicleManager* activeVehicleManager;
    GravitySimulator gravitySimulator;

    UIManager* uiManager;  // Reference to UI manager

public:
    GameManager(sf::RenderWindow& window, UIManager* ui = nullptr);
    ~GameManager();

    void initialize();
    void update(float deltaTime);
    void updateCamera(float deltaTime);
    void render();
    void handleEvents();
    void cleanup();
    // Add to GameManager.h public section:
    void setUIManager(UIManager* ui) { uiManager = ui; }
    // Getters
    VehicleManager* getActiveVehicleManager();
    const std::vector<Planet*>& getPlanets() const;
    sf::View& getGameView();
    sf::View& getUIView();
};