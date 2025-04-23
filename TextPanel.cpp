// TextPanel.cpp
#include "TextPanel.h"

TextPanel::TextPanel(const sf::Font& font, unsigned int characterSize, sf::Vector2f position,
    sf::Vector2f size, sf::Color bgColor)
    : text(font)
{
    // Setup background
    background.setPosition(position);
    background.setSize(size);
    background.setFillColor(bgColor);
    background.setOutlineColor(sf::Color::White);
    background.setOutlineThickness(1.0f);

    // Setup text
    text.setCharacterSize(characterSize);
    text.setFillColor(sf::Color::White);
    text.setPosition(sf::Vector2f(position.x + 5.f, position.y + 5.f));
}

void TextPanel::setText(const std::string& str)
{
    text.setString(str);
}

void TextPanel::draw(sf::RenderWindow& window)
{
    window.draw(background);
    window.draw(text);
}

void TextPanel::setPosition(sf::Vector2f position)
{
    background.setPosition(position);
    text.setPosition(sf::Vector2f(position.x + 5.f, position.y + 5.f));
}

void TextPanel::setSize(sf::Vector2f size)
{
    background.setSize(size);
}

sf::Vector2f TextPanel::getPosition() const
{
    return background.getPosition();
}

sf::Vector2f TextPanel::getSize() const
{
    return background.getSize();
}