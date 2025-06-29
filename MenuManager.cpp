#include "MenuManager.h"
#include <iostream>
#include <fstream>
#include <algorithm>

// Static color definitions
const sf::Color MenuManager::BUTTON_NORMAL_COLOR = sf::Color(70, 70, 70, 200);
const sf::Color MenuManager::BUTTON_HOVER_COLOR = sf::Color(100, 100, 100, 200);
const sf::Color MenuManager::BUTTON_PRESSED_COLOR = sf::Color(50, 50, 50, 200);
const sf::Color MenuManager::TEXT_COLOR = sf::Color::White;

MenuManager::MenuManager()
    : m_currentState(MenuState::ProfileMenu)
    , m_previousState(MenuState::ProfileMenu)
    , m_currentProfile(nullptr)
    , m_waitingForNameInput(false)
    , m_showWarning(false)
    , m_warningTimer(0.0f)
{
}

MenuManager::~MenuManager() {
}

void MenuManager::Initialize(sf::RenderWindow& window) {
    m_windowSize = sf::Vector2f(window.getSize());
    LoadResources();

    // Load existing profiles (in a real game, this would be from a file)
    // For now, create some sample profiles
    m_profiles.push_back(PlayerProfile("Player1"));
    m_profiles.push_back(PlayerProfile("Player2"));

    SetMenuState(MenuState::ProfileMenu);
}

void MenuManager::LoadResources() {
    // Load font
    if (!m_font.loadFromFile("Fonts/Kreon-Medium.ttf")) {
        std::cerr << "Warning: Could not load font. Using default font." << std::endl;
    }

    // Setup title text
    m_titleText.setFont(m_font);
    m_titleText.setCharacterSize(48);
    m_titleText.setFillColor(TEXT_COLOR);

    // Setup warning text
    m_warningText.setFont(m_font);
    m_warningText.setCharacterSize(24);
    m_warningText.setFillColor(sf::Color::Red);

    // Setup input prompt text
    m_inputPromptText.setFont(m_font);
    m_inputPromptText.setCharacterSize(32);
    m_inputPromptText.setFillColor(TEXT_COLOR);
    m_inputPromptText.setString("Enter your name:");

    // Setup input text
    m_inputText.setFont(m_font);
    m_inputText.setCharacterSize(24);
    m_inputText.setFillColor(sf::Color::Black);

    // Setup input box
    m_inputBox.setSize(sf::Vector2f(400, 50));
    m_inputBox.setFillColor(sf::Color::White);
    m_inputBox.setOutlineThickness(2);
    m_inputBox.setOutlineColor(sf::Color::Black);
}

void MenuManager::Update(sf::RenderWindow& window, float deltaTime) {
    UpdateButtons(window);

    // Update warning timer
    if (m_showWarning) {
        m_warningTimer -= deltaTime;
        if (m_warningTimer <= 0.0f) {
            m_showWarning = false;
        }
    }
}

void MenuManager::Draw(sf::RenderWindow& window) {
    // Draw background if available
    if (m_backgroundTexture.getSize().x > 0) {
        window.draw(m_backgroundSprite);
    }

    // Draw title
    window.draw(m_titleText);

    // Draw buttons
    DrawButtons(window);

    // Draw input elements if waiting for input
    if (m_waitingForNameInput) {
        window.draw(m_inputPromptText);
        window.draw(m_inputBox);
        window.draw(m_inputText);
    }

    // Draw warning message
    if (m_showWarning) {
        window.draw(m_warningText);
    }
}

void MenuManager::HandleInput(sf::Event& event, sf::RenderWindow& window) {
    if (event.type == sf::Event::MouseButtonPressed) {
        if (event.mouseButton.button == sf::Mouse::Left) {
            sf::Vector2f mousePos(event.mouseButton.x, event.mouseButton.y);

            for (auto& button : m_buttons) {
                if (button.isVisible && IsMouseOverButton(button, mousePos)) {
                    button.state = ButtonState::Pressed;
                    if (button.callback) {
                        button.callback();
                    }
                    break;
                }
            }
        }
    }

    // Handle text input for profile creation
    if (m_waitingForNameInput && event.type == sf::Event::TextEntered) {
        HandleTextInput(event.text.unicode);
    }

    // Handle Enter key for profile creation
    if (m_waitingForNameInput && event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Enter) {
            if (!m_inputBuffer.empty()) {
                CreateNewProfile(m_inputBuffer);
                SetWaitingForInput(false);
                ClearInputText();
                SetMenuState(MenuState::ChooseProfile);
            }
            else {
                ShowWarningMessage("Please enter a name!");
            }
        }
        else if (event.key.code == sf::Keyboard::Escape) {
            SetWaitingForInput(false);
            ClearInputText();
            SetMenuState(MenuState::ProfileMenu);
        }
    }
}

void MenuManager::SetMenuState(MenuState newState) {
    m_previousState = m_currentState;
    m_currentState = newState;
    m_buttons.clear();

    switch (newState) {
    case MenuState::ProfileMenu:
        CreateProfileMenu();
        break;
    case MenuState::CreateProfile:
        CreateNewProfileMenu();
        break;
    case MenuState::ChooseProfile:
        CreateChooseProfileMenu();
        break;
    case MenuState::MainMenu:
        CreateMainMenu();
        break;
    case MenuState::PlayMenu:
        CreatePlayMenu();
        break;
    case MenuState::GamePlay:
        // No menu for gameplay
        break;
    }
}

void MenuManager::CreateProfileMenu() {
    m_titleText.setString("Profile Menu");
    sf::FloatRect titleBounds = m_titleText.getLocalBounds();
    m_titleText.setPosition((m_windowSize.x - titleBounds.width) / 2, 100);

    float startY = m_windowSize.y / 2 - 100;

    CreateButton("New Profile",
        sf::Vector2f((m_windowSize.x - BUTTON_WIDTH) / 2, startY),
        sf::Vector2f(BUTTON_WIDTH, BUTTON_HEIGHT),
        [this]() { SetMenuState(MenuState::CreateProfile); });

    CreateButton("Existing Profile",
        sf::Vector2f((m_windowSize.x - BUTTON_WIDTH) / 2, startY + BUTTON_SPACING),
        sf::Vector2f(BUTTON_WIDTH, BUTTON_HEIGHT),
        [this]() {
            if (m_profiles.empty()) {
                ShowWarningMessage("No profiles found! Create a new profile first.");
            }
            else {
                SetMenuState(MenuState::ChooseProfile);
            }
        });

    CreateButton("Play as Guest",
        sf::Vector2f((m_windowSize.x - BUTTON_WIDTH) / 2, startY + BUTTON_SPACING * 2),
        sf::Vector2f(BUTTON_WIDTH, BUTTON_HEIGHT),
        [this]() {
            m_currentProfile = nullptr;
            SetMenuState(MenuState::MainMenu);
        });

    CreateButton("Exit",
        sf::Vector2f((m_windowSize.x - BUTTON_WIDTH) / 2, startY + BUTTON_SPACING * 3),
        sf::Vector2f(BUTTON_WIDTH, BUTTON_HEIGHT),
        [this]() {
            // This would close the game - you can implement this in your main game class
            std::cout << "Exit game requested" << std::endl;
        });
}

void MenuManager::CreateNewProfileMenu() {
    m_titleText.setString("Create New Profile");
    sf::FloatRect titleBounds = m_titleText.getLocalBounds();
    m_titleText.setPosition((m_windowSize.x - titleBounds.width) / 2, 150);

    // Position input elements
    m_inputPromptText.setPosition((m_windowSize.x - 200) / 2, m_windowSize.y / 2 - 100);
    m_inputBox.setPosition((m_windowSize.x - 400) / 2, m_windowSize.y / 2 - 50);
    m_inputText.setPosition(m_inputBox.getPosition().x + 10, m_inputBox.getPosition().y + 10);

    SetWaitingForInput(true);

    CreateButton("Back",
        sf::Vector2f(50, m_windowSize.y - 100),
        sf::Vector2f(150, BUTTON_HEIGHT),
        [this]() {
            SetWaitingForInput(false);
            ClearInputText();
            SetMenuState(MenuState::ProfileMenu);
        });
}

void MenuManager::CreateChooseProfileMenu() {
    m_titleText.setString("Choose Profile");
    sf::FloatRect titleBounds = m_titleText.getLocalBounds();
    m_titleText.setPosition((m_windowSize.x - titleBounds.width) / 2, 100);

    float startY = 200;

    for (size_t i = 0; i < m_profiles.size(); ++i) {
        std::string buttonText = m_profiles[i].name + " (Level " + std::to_string(m_profiles[i].level) + ")";
        CreateButton(buttonText,
            sf::Vector2f((m_windowSize.x - BUTTON_WIDTH) / 2, startY + i * BUTTON_SPACING),
            sf::Vector2f(BUTTON_WIDTH, BUTTON_HEIGHT),
            [this, i]() {
                SelectProfile(i);
                SetMenuState(MenuState::MainMenu);
            });
    }

    CreateButton("Back",
        sf::Vector2f(50, m_windowSize.y - 100),
        sf::Vector2f(150, BUTTON_HEIGHT),
        [this]() { SetMenuState(MenuState::ProfileMenu); });
}

void MenuManager::CreateMainMenu() {
    std::string title = "Main Menu";
    if (m_currentProfile) {
        title = "Welcome, " + m_currentProfile->name + "!";
    }
    m_titleText.setString(title);
    sf::FloatRect titleBounds = m_titleText.getLocalBounds();
    m_titleText.setPosition((m_windowSize.x - titleBounds.width) / 2, 100);

    float startY = m_windowSize.y / 2 - 50;

    CreateButton("Play",
        sf::Vector2f((m_windowSize.x - BUTTON_WIDTH) / 2, startY),
        sf::Vector2f(BUTTON_WIDTH, BUTTON_HEIGHT),
        [this]() { SetMenuState(MenuState::PlayMenu); });

    CreateButton("Settings",
        sf::Vector2f((m_windowSize.x - BUTTON_WIDTH) / 2, startY + BUTTON_SPACING),
        sf::Vector2f(BUTTON_WIDTH, BUTTON_HEIGHT),
        [this]() {
            // Implement settings menu
            ShowWarningMessage("Settings menu not implemented yet!");
        });

    CreateButton("Exit",
        sf::Vector2f((m_windowSize.x - BUTTON_WIDTH) / 2, startY + BUTTON_SPACING * 2),
        sf::Vector2f(BUTTON_WIDTH, BUTTON_HEIGHT),
        [this]() { SetMenuState(MenuState::ProfileMenu); });
}

void MenuManager::CreatePlayMenu() {
    m_titleText.setString("Select Level");
    sf::FloatRect titleBounds = m_titleText.getLocalBounds();
    m_titleText.setPosition((m_windowSize.x - titleBounds.width) / 2, 100);

    float startY = 200;

    for (int i = 1; i <= 4; ++i) {
        std::string levelText = "Level " + std::to_string(i);
        CreateButton(levelText,
            sf::Vector2f((m_windowSize.x - BUTTON_WIDTH) / 2, startY + (i - 1) * BUTTON_SPACING),
            sf::Vector2f(BUTTON_WIDTH, BUTTON_HEIGHT),
            [this, i]() {
                // Start the game at the selected level
                SetMenuState(MenuState::GamePlay);
                std::cout << "Starting Level " << i << std::endl;
            });
    }

    CreateButton("Settings",
        sf::Vector2f((m_windowSize.x - BUTTON_WIDTH) / 2, startY + 4 * BUTTON_SPACING),
        sf::Vector2f(BUTTON_WIDTH, BUTTON_HEIGHT),
        [this]() {
            ShowWarningMessage("Settings menu not implemented yet!");
        });

    CreateButton("Back",
        sf::Vector2f(50, m_windowSize.y - 100),
        sf::Vector2f(150, BUTTON_HEIGHT),
        [this]() { SetMenuState(MenuState::MainMenu); });
}

void MenuManager::CreateButton(const std::string& text, sf::Vector2f position, sf::Vector2f size,
    std::function<void()> callback) {
    Button button;
    button.shape.setPosition(position);
    button.shape.setSize(size);
    button.shape.setFillColor(BUTTON_NORMAL_COLOR);
    button.shape.setOutlineThickness(2);
    button.shape.setOutlineColor(sf::Color::White);

    button.text.setFont(m_font);
    button.text.setString(text);
    button.text.setCharacterSize(24);
    button.text.setFillColor(TEXT_COLOR);
    CenterText(button.text, button.shape);

    button.callback = callback;
    button.state = ButtonState::Normal;
    button.isVisible = true;

    m_buttons.push_back(button);
}

void MenuManager::UpdateButtons(sf::RenderWindow& window) {
    sf::Vector2f mousePos = sf::Vector2f(sf::Mouse::getPosition(window));

    for (auto& button : m_buttons) {
        if (!button.isVisible) continue;

        if (IsMouseOverButton(button, mousePos)) {
            if (button.state != ButtonState::Pressed) {
                button.state = ButtonState::Hovered;
                button.shape.setFillColor(BUTTON_HOVER_COLOR);
            }
        }
        else {
            button.state = ButtonState::Normal;
            button.shape.setFillColor(BUTTON_NORMAL_COLOR);
        }

        // Reset pressed state after one frame
        if (button.state == ButtonState::Pressed) {
            button.state = ButtonState::Normal;
            button.shape.setFillColor(BUTTON_NORMAL_COLOR);
        }
    }
}

void MenuManager::DrawButtons(sf::RenderWindow& window) {
    for (const auto& button : m_buttons) {
        if (button.isVisible) {
            window.draw(button.shape);
            window.draw(button.text);
        }
    }
}

bool MenuManager::IsMouseOverButton(const Button& button, sf::Vector2f mousePos) {
    return button.shape.getGlobalBounds().contains(mousePos);
}

void MenuManager::CenterText(sf::Text& text, const sf::RectangleShape& shape) {
    sf::FloatRect textBounds = text.getLocalBounds();
    sf::FloatRect shapeBounds = shape.getGlobalBounds();

    text.setPosition(
        shapeBounds.left + (shapeBounds.width - textBounds.width) / 2,
        shapeBounds.top + (shapeBounds.height - textBounds.height) / 2
    );
}

void MenuManager::CreateNewProfile(const std::string& name) {
    // Check if profile name already exists
    for (const auto& profile : m_profiles) {
        if (profile.name == name) {
            ShowWarningMessage("Profile name already exists!");
            return;
        }
    }

    m_profiles.push_back(PlayerProfile(name));
    std::cout << "Created new profile: " << name << std::endl;
}

void MenuManager::SelectProfile(int index) {
    if (index >= 0 && index < m_profiles.size()) {
        m_currentProfile = &m_profiles[index];
        std::cout << "Selected profile: " << m_currentProfile->name << std::endl;
    }
}

void MenuManager::HandleTextInput(sf::Uint32 unicode) {
    if (unicode == 8) { // Backspace
        if (!m_inputBuffer.empty()) {
            m_inputBuffer.pop_back();
        }
    }
    else if (unicode >= 32 && unicode <= 126) { // Printable characters
        if (m_inputBuffer.length() < 20) { // Limit name length
            m_inputBuffer += static_cast<char>(unicode);
        }
    }

    m_inputText.setString(m_inputBuffer);
}

void MenuManager::ClearInputText() {
    m_inputBuffer.clear();
    m_inputText.setString("");
}

void MenuManager::ShowWarningMessage(const std::string& message) {
    m_warningText.setString(message);
    sf::FloatRect textBounds = m_warningText.getLocalBounds();
    m_warningText.setPosition((m_windowSize.x - textBounds.width) / 2, m_windowSize.y - 150);
    m_showWarning = true;
    m_warningTimer = 3.0f; // Show warning for 3 seconds
}