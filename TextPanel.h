// TextPanel.h
#pragma once
#include <SFML/Graphics.hpp>
#include <string>

class TextPanel {
private:
    sf::Text text;
    sf::RectangleShape background;

public:
    TextPanel() = default;
    TextPanel(const sf::Font& font, unsigned int characterSize, sf::Vector2f position,
        sf::Vector2f size, sf::Color bgColor = sf::Color(0, 0, 0, 180));

    void setText(const std::string& str);
    void draw(sf::RenderWindow& window);
    void setPosition(sf::Vector2f position);
    void setSize(sf::Vector2f size);
    sf::Vector2f getPosition() const;
    sf::Vector2f getSize() const;
};