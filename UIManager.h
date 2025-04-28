// UIManager.h
#pragma once
#include <SFML/Graphics.hpp>
#include "TextPanel.h"
#include "VehicleManager.h"
#include "Planet.h"
#include "Button.h"

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

    // Planet mass control buttons
    Button increaseMassButton;
    Button decreaseMassButton;
    Planet* nearestPlanet;
    VehicleManager* activeVehicleManager;

    Planet* selectedPlanet;
    bool fuelDecreaseActive = false;
    bool fuelIncreaseActive = false;
    float fuelTransferTimer = 0.0f;
    const float fuelTransferRate = 0.1f; // How often to transfer fuel (seconds)

    Button increaseThrustButton;
    Button increaseEfficiencyButton;
    const float upgradeCost = 0.1f; // Base cost for upgrades

    bool isMultiplayer;
    bool isHost;

public:
    UIManager(sf::RenderWindow& window, sf::Font& font, sf::View& uiView, bool multiplayer, bool host);

    void update(VehicleManager* vehicleManager, const std::vector<Planet*>& planets, float deltaTime);
    void render();
    // Add this to UIManager.h in the public section:

    void updateControlsInfo();
    Planet* getSelectedPlanet() const { return selectedPlanet; }
    // Update specific panels
    void updateRocketInfo(VehicleManager* vehicleManager);
    void updatePlanetInfo(VehicleManager* vehicleManager, const std::vector<Planet*>& planets);
    void updateOrbitInfo(VehicleManager* vehicleManager, const std::vector<Planet*>& planets);
    void updateThrustMetrics(VehicleManager* vehicleManager, const std::vector<Planet*>& planets);
    void updateMultiplayerInfo(int connectedClients, bool connected, int playerId, int pingMs);
    void setSelectedPlanet(Planet* planet) { selectedPlanet = planet; }
    // Find nearest planet to vehicle
    Planet* findNearestPlanet(VehicleManager* vehicleManager, const std::vector<Planet*>& planets);
};