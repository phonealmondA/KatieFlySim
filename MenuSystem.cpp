// MenuSystem.cpp
#include "MenuSystem.h"
#include <iostream>
#include <windows.h> // Added for Windows process creation

MenuSystem::MenuSystem(sf::RenderWindow& window, sf::Font& font)
    : window(window),
    currentState(MenuGameState::MENU),
    font(font),
    titleText(font),
    inputAddress(""),
    inputPort("5000"),
    focusAddress(true)
{
    // Initialize title text
    titleText.setString("Noah's Flight Sim");
    titleText.setCharacterSize(48);
    titleText.setFillColor(sf::Color::White);
    titleText.setPosition(sf::Vector2f(640.f - titleText.getLocalBounds().size.x / 2, 150.f));

    // Initialize menu buttons - ONLY include Single Player and Join
    menuButtons.push_back(Button(
        sf::Vector2f(640.f - 100.f, 300.f),
        sf::Vector2f(200.f, 50.f),
        "Start",
        font,
        [this]() { currentState = MenuGameState::SINGLE_PLAYER; }
    ));

    menuButtons.push_back(Button(
        sf::Vector2f(640.f - 100.f, 370.f),
        sf::Vector2f(200.f, 50.f),
        "Join",
        font,
        [this]() { currentState = MenuGameState::JOIN_MENU; }
    ));

    // Initialize join menu buttons
    joinMenuButtons.push_back(Button(
        sf::Vector2f(640.f - 100.f, 440.f),
        sf::Vector2f(200.f, 50.f),
        "Connect",
        font,
        [this]() { currentState = MenuGameState::MULTIPLAYER_CLIENT; }
    ));

    joinMenuButtons.push_back(Button(
        sf::Vector2f(640.f - 100.f, 510.f),
        sf::Vector2f(200.f, 50.f),
        "Back",
        font,
        [this]() { currentState = MenuGameState::MENU; }
    ));
}

void MenuSystem::launchHostProcess() {
    // Instead of launching a new process, just set the state to host mode
    currentState = MenuGameState::MULTIPLAYER_HOST;

    // Print debug info (you can keep this from the original function)
    std::cout << "Starting host mode from menu" << std::endl;

    // No need to close the window or create a new process
    // The main function will handle initializing the server components
}


MenuGameState MenuSystem::run()
{
    sf::Clock clock;

    while (window.isOpen() && (currentState == MenuGameState::MENU || currentState == MenuGameState::JOIN_MENU))
    {
        float deltaTime = std::min(clock.restart().asSeconds(), 0.1f);
        handleEvents();
        update(deltaTime);
        render();
    }

    return currentState;
}

void MenuSystem::handleEvents()
{
    if (std::optional<sf::Event> event = window.pollEvent())
    {
        if (event->is<sf::Event::Closed>())
            window.close();

        if (event->is<sf::Event::KeyPressed>())
        {
            const auto* keyEvent = event->getIf<sf::Event::KeyPressed>();
            if (keyEvent)
            {
                if (keyEvent->code == sf::Keyboard::Key::Escape)
                {
                    if (currentState == MenuGameState::JOIN_MENU)
                        currentState = MenuGameState::MENU;
                    else
                        window.close();
                }
                else if (keyEvent->code == sf::Keyboard::Key::Q && currentState == MenuGameState::MENU)
                {
                    currentState = MenuGameState::SINGLE_PLAYER;
                }
                //else if (keyEvent->code == sf::Keyboard::Key::H && currentState == MenuGameState::MENU)
                //{
                //    launchHostProcess();
                //}
                else if (keyEvent->code == sf::Keyboard::Key::J && currentState == MenuGameState::MENU)
                {
                    currentState = MenuGameState::JOIN_MENU;
                }
                //else if (keyEvent->code == sf::Keyboard::Key::C && currentState == MenuGameState::JOIN_MENU)
                //{
                //    currentState = MenuGameState::MULTIPLAYER_CLIENT;
                //}

                // Handle text input for join menu
                if (currentState == MenuGameState::JOIN_MENU)
                {
                    // Handle backspace
                    if (keyEvent->code == sf::Keyboard::Key::Backspace)
                    {
                        if (focusAddress && !inputAddress.empty())
                            inputAddress.pop_back();
                        else if (!focusAddress && !inputPort.empty())
                            inputPort.pop_back();
                    }
                    // Handle tab to switch focus
                    else if (keyEvent->code == sf::Keyboard::Key::Tab)
                    {
                        focusAddress = !focusAddress;
                    }
                    // Handle text input for A-Z keys
                    else if (keyEvent->code >= sf::Keyboard::Key::A && keyEvent->code <= sf::Keyboard::Key::Z)
                    {
                        // Convert enum to integer index and then to character
                        int keyIndex = static_cast<int>(keyEvent->code) - static_cast<int>(sf::Keyboard::Key::A);
                        if (focusAddress && keyIndex >= 0 && keyIndex < 26)
                            inputAddress += static_cast<char>('a' + keyIndex);
                    }
                    else if (keyEvent->code >= sf::Keyboard::Key::Num0 && keyEvent->code <= sf::Keyboard::Key::Num9)
                    {
                        int digitIndex = static_cast<int>(keyEvent->code) - static_cast<int>(sf::Keyboard::Key::Num0);
                        char digit = '0' + digitIndex;
                        if (focusAddress)
                            inputAddress += digit;
                        else
                            inputPort += digit;
                    }
                    else if (keyEvent->code == sf::Keyboard::Key::Period)
                    {
                        if (focusAddress)
                            inputAddress += '.';
                    }
                }
            }
        }

        if (event->is<sf::Event::MouseButtonPressed>())
        {
            const auto* mouseEvent = event->getIf<sf::Event::MouseButtonPressed>();
            if (mouseEvent && mouseEvent->button == sf::Mouse::Button::Left)
            {
                sf::Vector2i mousePosition = sf::Mouse::getPosition(window);
                sf::Vector2f mousePos = window.mapPixelToCoords(mousePosition);

                // Check button clicks based on current menu
                if (currentState == MenuGameState::MENU)
                {
                    for (auto& button : menuButtons)
                    {
                        if (button.contains(mousePos))
                        {
                            button.handleClick();
                            break;
                        }
                    }
                }
                else if (currentState == MenuGameState::JOIN_MENU)
                {
                    for (auto& button : joinMenuButtons)
                    {
                        if (button.contains(mousePos))
                        {
                            button.handleClick();
                            break;
                        }
                    }

                    // Check if clicked on input fields
                    sf::FloatRect addressRect({ 520.f, 330.f }, { 300.f, 30.f });
                    sf::FloatRect portRect({ 520.f, 400.f }, { 150.f, 30.f });

                    if (addressRect.contains(mousePos))
                        focusAddress = true;
                    else if (portRect.contains(mousePos))
                        focusAddress = false;
                }
            }
        }
    }
}

void MenuSystem::update(float deltaTime)
{
    sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

    if (currentState == MenuGameState::MENU)
    {
        for (auto& button : menuButtons)
        {
            button.update(mousePos);
        }
    }
    else if (currentState == MenuGameState::JOIN_MENU)
    {
        for (auto& button : joinMenuButtons)
        {
            button.update(mousePos);
        }
    }
}


void MenuSystem::render()
{
    window.clear(sf::Color(20, 20, 50));  // Dark blue background

    if (currentState == MenuGameState::MENU)
    {
        // Draw title
        window.draw(titleText);

        // Draw buttons and text separately at the same positions
        for (size_t i = 0; i < menuButtons.size(); i++)
        {
            // Draw the button itself
            menuButtons[i].draw(window);

            // Create separate text
            sf::Text buttonText(font);
            buttonText.setFillColor(sf::Color::White);
            buttonText.setCharacterSize(20);

            // Set text content based on button index
            if (i == 0) buttonText.setString("Start");
            else if (i == 1) buttonText.setString("Join");

            // Place text at hardcoded positions matching the buttons
            if (i == 0) buttonText.setPosition(sf::Vector2f(590.f, 315.f));
            else if (i == 1) buttonText.setPosition(sf::Vector2f(590.f, 385.f));

            // Draw the text
            window.draw(buttonText);
        }

        // Add text to inform about command-line hosting
        sf::Text hostInfoText(font);
        hostInfoText.setString("To host a game, use --host command line parameter");
        hostInfoText.setCharacterSize(16);
        hostInfoText.setFillColor(sf::Color::Yellow);
        hostInfoText.setPosition(sf::Vector2f(440.f, 450.f));
        window.draw(hostInfoText);
    }
    else if (currentState == MenuGameState::JOIN_MENU)
    {
        // Draw title
        window.draw(titleText);

        // Draw join menu elements
        sf::Text addressLabel(font);
        addressLabel.setString("Server Address:");
        addressLabel.setCharacterSize(20);
        addressLabel.setFillColor(sf::Color::White);
        addressLabel.setPosition(sf::Vector2f(520.f, 300.f));
        window.draw(addressLabel);

        sf::RectangleShape addressInput;
        addressInput.setSize(sf::Vector2f(300.f, 30.f));
        addressInput.setFillColor(sf::Color(50, 50, 50));
        addressInput.setOutlineColor(focusAddress ? sf::Color::Yellow : sf::Color::White);
        addressInput.setOutlineThickness(2.f);
        addressInput.setPosition(sf::Vector2f(520.f, 330.f));
        window.draw(addressInput);

        sf::Text addressText(font);
        addressText.setString(inputAddress);
        addressText.setCharacterSize(18);
        addressText.setFillColor(sf::Color::White);
        addressText.setPosition(sf::Vector2f(525.f, 335.f));
        window.draw(addressText);

        sf::Text portLabel(font);
        portLabel.setString("Port:");
        portLabel.setCharacterSize(20);
        portLabel.setFillColor(sf::Color::White);
        portLabel.setPosition(sf::Vector2f(520.f, 370.f));
        window.draw(portLabel);

        sf::RectangleShape portInput;
        portInput.setSize(sf::Vector2f(150.f, 30.f));
        portInput.setFillColor(sf::Color(50, 50, 50));
        portInput.setOutlineColor(focusAddress ? sf::Color::White : sf::Color::Yellow);
        portInput.setOutlineThickness(2.f);
        portInput.setPosition(sf::Vector2f(520.f, 400.f));
        window.draw(portInput);

        sf::Text portText(font);
        portText.setString(inputPort);
        portText.setCharacterSize(18);
        portText.setFillColor(sf::Color::White);
        portText.setPosition(sf::Vector2f(525.f, 405.f));
        window.draw(portText);

        // Draw buttons and add separate text
        for (size_t i = 0; i < joinMenuButtons.size(); i++)
        {
            // Draw the button
            joinMenuButtons[i].draw(window);

            // Create separate text
            sf::Text buttonText(font);
            buttonText.setFillColor(sf::Color::White);
            buttonText.setCharacterSize(20);

            // Set text content based on button index
            if (i == 0) buttonText.setString("Connect");
            else if (i == 1) buttonText.setString("Back");

            // Place text at hardcoded positions matching the buttons
            if (i == 0) buttonText.setPosition(sf::Vector2f(580.f, 455.f));
            else if (i == 1) buttonText.setPosition(sf::Vector2f(590.f, 525.f));

            // Draw the text
            window.draw(buttonText);
        }
    }

    window.display();
}