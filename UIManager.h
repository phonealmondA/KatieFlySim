// UIManager.h
#pragma once
#include <SFML/Graphics.hpp>
#include "TextPanel.h"
#include "VehicleManager.h"
#include "Planet.h"

class UIManager {
private:
    sf::RenderWindow& window;
    sf::Font& font;
    sf::View& uiView;

    // UI panels
    TextPanel rocketInfoPanel;
    TextPanel planetInfoPanel;
    TextPanel orbitInfoPanel;
    TextPanel controlsPanel;
    TextPanel thrustMetricsPanel;
    TextPanel multiplayerPanel;

    bool isMultiplayer;
    bool isHost;

public:
    UIManager(sf::RenderWindow& window, sf::Font& font, sf::View& uiView, bool multiplayer, bool host);

    void update(VehicleManager* vehicleManager, const std::vector<Planet*>& planets, float deltaTime);
    void render();

    // Update specific panels
    void updateRocketInfo(VehicleManager* vehicleManager);
    void updatePlanetInfo(VehicleManager* vehicleManager, const std::vector<Planet*>& planets);
    void updateOrbitInfo(VehicleManager* vehicleManager, const std::vector<Planet*>& planets);
    void updateThrustMetrics(VehicleManager* vehicleManager, const std::vector<Planet*>& planets);
    void updateMultiplayerInfo(int connectedClients, bool connected, int playerId, int pingMs);
};