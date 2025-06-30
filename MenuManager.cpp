#include "MenuManager.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include "nlohmann/json.hpp" 

using json = nlohmann::json;

// Static color definitions
const sf::Color MenuManager::BUTTON_NORMAL_COLOR = sf::Color(70, 70, 70, 200);
const sf::Color MenuManager::BUTTON_HOVER_COLOR = sf::Color(100, 100, 100, 200);
const sf::Color MenuManager::BUTTON_PRESSED_COLOR = sf::Color(50, 50, 50, 200);
const sf::Color MenuManager::TEXT_COLOR = sf::Color::White;
const sf::Color MenuManager::DELETE_BUTTON_COLOR = sf::Color(150, 50, 50, 200);
const sf::Color MenuManager::DELETE_BUTTON_HOVER_COLOR = sf::Color(200, 70, 70, 200);
const std::string MenuManager::PROFILES_FILE_PATH = "profiles.json";
const int MenuManager::MAX_PROFILES = 5;

MenuManager::MenuManager()
    : m_currentState(MenuState::ProfileMenu)
    , m_previousState(MenuState::ProfileMenu)
    , m_currentProfile(nullptr)
    , m_waitingForNameInput(false)
    , m_showWarning(false)
    , m_warningTimer(0.0f)
    , m_gamePaused(false)
{
}

MenuManager::~MenuManager() {
    SaveProfilesToFile();
}

void MenuManager::Initialize(sf::RenderWindow& window) {
    m_windowSize = sf::Vector2f(window.getSize());
    LoadResources();
    LoadProfilesFromFile();
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

    // Setup pause menu background
    m_pauseBackground.setSize(sf::Vector2f(400, 300));
    m_pauseBackground.setFillColor(sf::Color(0, 0, 0, 180));
    m_pauseBackground.setOutlineThickness(3);
    m_pauseBackground.setOutlineColor(sf::Color::White);
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

    // Draw pause menu if game is paused
    if (m_gamePaused && m_currentState == MenuState::GamePlay) {
        DrawPauseMenu(window);
    }
}

void MenuManager::DrawPauseMenu(sf::RenderWindow& window) {
    // Center the pause menu background
    sf::Vector2f centerPos = sf::Vector2f(
        (m_windowSize.x - m_pauseBackground.getSize().x) / 2,
        (m_windowSize.y - m_pauseBackground.getSize().y) / 2
    );
    m_pauseBackground.setPosition(centerPos);

    window.draw(m_pauseBackground);

    // Draw pause menu buttons
    for (const auto& button : m_pauseButtons) {
        if (button.isVisible) {
            window.draw(button.shape);
            window.draw(button.text);
        }
    }
}

void MenuManager::HandleInput(sf::Event& event, sf::RenderWindow& window) {
    if (event.type == sf::Event::MouseButtonPressed) {
        if (event.mouseButton.button == sf::Mouse::Left) {
            sf::Vector2f mousePos(event.mouseButton.x, event.mouseButton.y);

            // Handle pause menu buttons if game is paused
            if (m_gamePaused && m_currentState == MenuState::GamePlay) {
                for (auto& button : m_pauseButtons) {
                    if (button.isVisible && IsMouseOverButton(button, mousePos)) {
                        button.state = ButtonState::Pressed;
                        if (button.callback) {
                            button.callback();
                        }
                        break;
                    }
                }
            }
            else {
                // Handle regular menu buttons
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
    }

    // Handle text input for profile creation
    if (m_waitingForNameInput && event.type == sf::Event::TextEntered) {
        HandleTextInput(event.text.unicode);
    }

    // Handle keyboard input
    if (event.type == sf::Event::KeyPressed) {
        // Handle Enter key for profile creation
        if (m_waitingForNameInput && event.key.code == sf::Keyboard::Enter) {
            if (!m_inputBuffer.empty()) {
                CreateNewProfile(m_inputBuffer);
                SetWaitingForInput(false);
                ClearInputText();
                // Automatically go to main menu after creating profile
                SetMenuState(MenuState::MainMenu);
            }
            else {
                ShowWarningMessage("Please enter a name!");
            }
        }
        // Handle Esc key for going back to previous menu or main menu
        else if (event.key.code == sf::Keyboard::Escape) {
            if (m_waitingForNameInput) {
                SetWaitingForInput(false);
                ClearInputText();
                SetMenuState(MenuState::ProfileMenu);
            }
            else {
                switch (m_currentState) {
                case MenuState::ProfileMenu:
                    // At profile menu, Esc closes the game or does nothing
                    std::cout << "Exit game requested" << std::endl;
                    break;
                case MenuState::CreateProfile:
                    SetMenuState(MenuState::ProfileMenu);
                    break;
                case MenuState::ChooseProfile:
                    SetMenuState(MenuState::ProfileMenu);
                    break;
                case MenuState::MainMenu:
                    SetMenuState(MenuState::ProfileMenu);
                    break;
                case MenuState::PlayMenu:
                    SetMenuState(MenuState::MainMenu);
                    break;
                case MenuState::GamePlay:
                    // Toggle pause menu when in gameplay
                    TogglePauseMenu();
                    break;
                }
            }
        }
    }
}

void MenuManager::TogglePauseMenu() {
    m_gamePaused = !m_gamePaused;

    if (m_gamePaused) {
        CreatePauseMenu();
        std::cout << "Game paused" << std::endl;
    }
    else {
        m_pauseButtons.clear();
        std::cout << "Game resumed" << std::endl;
    }
}

void MenuManager::CreatePauseMenu() {
    m_pauseButtons.clear();

    // Calculate center position for pause menu
    sf::Vector2f centerPos = sf::Vector2f(
        (m_windowSize.x - m_pauseBackground.getSize().x) / 2,
        (m_windowSize.y - m_pauseBackground.getSize().y) / 2
    );

    float buttonWidth = 200;
    float buttonHeight = 50;
    float buttonSpacing = 70;
    float startY = centerPos.y + 40;

    // Continue button
    CreatePauseButton("Continue",
        sf::Vector2f(centerPos.x + (m_pauseBackground.getSize().x - buttonWidth) / 2, startY),
        sf::Vector2f(buttonWidth, buttonHeight),
        [this]() {
            TogglePauseMenu(); // This will unpause the game
        });

    // Settings button
    CreatePauseButton("Settings",
        sf::Vector2f(centerPos.x + (m_pauseBackground.getSize().x - buttonWidth) / 2, startY + buttonSpacing),
        sf::Vector2f(buttonWidth, buttonHeight),
        [this]() {
            ShowWarningMessage("Settings menu not implemented yet!");
        });

    // Exit to Profile Menu button
    CreatePauseButton("Exit",
        sf::Vector2f(centerPos.x + (m_pauseBackground.getSize().x - buttonWidth) / 2, startY + buttonSpacing * 2),
        sf::Vector2f(buttonWidth, buttonHeight),
        [this]() {
            m_gamePaused = false;
            m_pauseButtons.clear();
            SetMenuState(MenuState::ProfileMenu);
            std::cout << "Exiting to Profile Menu" << std::endl;
        });
}

void MenuManager::CreatePauseButton(const std::string& text, sf::Vector2f position, sf::Vector2f size,
    std::function<void()> callback) {
    Button button;
    button.shape.setPosition(position);
    button.shape.setSize(size);
    button.shape.setFillColor(BUTTON_NORMAL_COLOR);
    button.shape.setOutlineThickness(2);
    button.shape.setOutlineColor(sf::Color::White);
    button.isDeleteButton = false;

    button.text.setFont(m_font);
    button.text.setString(text);

    // Adjust font size for better text fitting
    int fontSize = 20;
    if (text.length() > 20) {
        fontSize = 16;
    }
    else if (text.length() > 15) {
        fontSize = 18;
    }
    button.text.setCharacterSize(fontSize);
    button.text.setFillColor(TEXT_COLOR);
    CenterText(button.text, button.shape);

    button.callback = callback;
    button.state = ButtonState::Normal;
    button.isVisible = true;

    m_pauseButtons.push_back(button);
}

void MenuManager::SetMenuState(MenuState newState) {
    m_previousState = m_currentState;
    m_currentState = newState;
    m_buttons.clear();

    // Clear pause menu when changing states
    m_gamePaused = false;
    m_pauseButtons.clear();

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
        [this]() {
            if (m_profiles.size() >= MAX_PROFILES) {
                ShowWarningMessage("Maximum number of profiles reached!");
            }
            else {
                SetMenuState(MenuState::CreateProfile);
            }
        });

    CreateButton("Existing Profile",
        sf::Vector2f((m_windowSize.x - BUTTON_WIDTH) / 2, startY + BUTTON_SPACING),
        sf::Vector2f(BUTTON_WIDTH, BUTTON_HEIGHT),
        [this]() { SetMenuState(MenuState::ChooseProfile); });

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
            if (m_exitCallback) {
                m_exitCallback();
            }
        });
}

void MenuManager::CreateNewProfileMenu() {
    m_titleText.setString("Create New Profile");
    sf::FloatRect titleBounds = m_titleText.getLocalBounds();
    m_titleText.setPosition((m_windowSize.x - titleBounds.width) / 2, 150);

    // Position input elements
    float startY = m_windowSize.y / 2 - 100;
    m_inputPromptText.setPosition((m_windowSize.x - 200) / 2, m_windowSize.y / 2 - 100);
    m_inputBox.setPosition((m_windowSize.x - 400) / 2, m_windowSize.y / 2 - 50);
    m_inputText.setPosition(m_inputBox.getPosition().x + 10, m_inputBox.getPosition().y + 10);

    SetWaitingForInput(true);

    CreateButton("Back",
        sf::Vector2f((m_windowSize.x - BUTTON_WIDTH) / 2, m_windowSize.y / 2 + 50),
        sf::Vector2f(BUTTON_WIDTH, BUTTON_HEIGHT),
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
    int buttonIndex = 0;

    // Show existing profiles
    for (size_t i = 0; i < m_profiles.size(); ++i) {
        // Create profile info string that fits in button
        std::string profileInfo = m_profiles[i].name + " - Lv." + std::to_string(m_profiles[i].level);

        // Create main profile button (smaller width to make room for delete button)
        float profileButtonWidth = BUTTON_WIDTH - 80; // Leave space for delete button
        CreateButton(profileInfo,
            sf::Vector2f((m_windowSize.x - BUTTON_WIDTH) / 2, startY + buttonIndex * BUTTON_SPACING),
            sf::Vector2f(profileButtonWidth, BUTTON_HEIGHT),
            [this, i]() {
                SelectProfile(i);
                SetMenuState(MenuState::MainMenu);
            });

        // Create delete button next to profile button
        CreateDeleteButton("X",
            sf::Vector2f((m_windowSize.x - BUTTON_WIDTH) / 2 + profileButtonWidth + 10, startY + buttonIndex * BUTTON_SPACING),
            sf::Vector2f(60, BUTTON_HEIGHT),
            [this, i]() {
                DeleteProfile(i);
            });

        buttonIndex++;
    }

    // Show "New Profile" button only if not at maximum
    if (m_profiles.size() < MAX_PROFILES) {
        CreateButton("+ New Profile",
            sf::Vector2f((m_windowSize.x - BUTTON_WIDTH) / 2, startY + buttonIndex * BUTTON_SPACING),
            sf::Vector2f(BUTTON_WIDTH, BUTTON_HEIGHT),
            [this]() { SetMenuState(MenuState::CreateProfile); });
        buttonIndex++;
    }

    // Back button positioned below all profile buttons
    CreateButton("Back",
        sf::Vector2f((m_windowSize.x - BUTTON_WIDTH) / 2, startY + buttonIndex * BUTTON_SPACING),
        sf::Vector2f(BUTTON_WIDTH, BUTTON_HEIGHT),
        [this]() { SetMenuState(MenuState::ProfileMenu); });
}

void MenuManager::CreateMainMenu() {
    std::string title = "Main Menu";
    if (m_currentProfile) {
        title = "Welcome, " + m_currentProfile->name + "!";
    }
    else {
        title = "Welcome, Guest!";
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

    CreateButton("Back",
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
    button.isDeleteButton = false;

    button.text.setFont(m_font);
    button.text.setString(text);

    // Adjust font size for better text fitting
    int fontSize = 24;
    if (text.length() > 20) {
        fontSize = 18;
    }
    else if (text.length() > 15) {
        fontSize = 20;
    }
    button.text.setCharacterSize(fontSize);
    button.text.setFillColor(TEXT_COLOR);
    CenterText(button.text, button.shape);

    button.callback = callback;
    button.state = ButtonState::Normal;
    button.isVisible = true;

    m_buttons.push_back(button);
}

void MenuManager::CreateDeleteButton(const std::string& text, sf::Vector2f position, sf::Vector2f size,
    std::function<void()> callback) {
    Button button;
    button.shape.setPosition(position);
    button.shape.setSize(size);
    button.shape.setFillColor(DELETE_BUTTON_COLOR);
    button.shape.setOutlineThickness(2);
    button.shape.setOutlineColor(sf::Color::White);
    button.isDeleteButton = true;

    button.text.setFont(m_font);
    button.text.setString(text);
    button.text.setCharacterSize(20);
    button.text.setFillColor(TEXT_COLOR);
    CenterText(button.text, button.shape);

    button.callback = callback;
    button.state = ButtonState::Normal;
    button.isVisible = true;

    m_buttons.push_back(button);
}

void MenuManager::UpdateButtons(sf::RenderWindow& window) {
    sf::Vector2f mousePos = sf::Vector2f(sf::Mouse::getPosition(window));

    // Update regular buttons
    for (auto& button : m_buttons) {
        if (!button.isVisible) continue;

        if (IsMouseOverButton(button, mousePos)) {
            if (button.state != ButtonState::Pressed) {
                button.state = ButtonState::Hovered;
                if (button.isDeleteButton) {
                    button.shape.setFillColor(DELETE_BUTTON_HOVER_COLOR);
                }
                else {
                    button.shape.setFillColor(BUTTON_HOVER_COLOR);
                }
            }
        }
        else {
            button.state = ButtonState::Normal;
            if (button.isDeleteButton) {
                button.shape.setFillColor(DELETE_BUTTON_COLOR);
            }
            else {
                button.shape.setFillColor(BUTTON_NORMAL_COLOR);
            }
        }

        // Reset pressed state after one frame
        if (button.state == ButtonState::Pressed) {
            button.state = ButtonState::Normal;
            if (button.isDeleteButton) {
                button.shape.setFillColor(DELETE_BUTTON_COLOR);
            }
            else {
                button.shape.setFillColor(BUTTON_NORMAL_COLOR);
            }
        }
    }

    // Update pause menu buttons
    if (m_gamePaused && m_currentState == MenuState::GamePlay) {
        for (auto& button : m_pauseButtons) {
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

    // Check maximum profiles limit
    if (m_profiles.size() >= MAX_PROFILES) {
        ShowWarningMessage("Maximum number of profiles reached!");
        return;
    }

    PlayerProfile newProfile(name);
    m_profiles.push_back(newProfile);

    // Automatically select the newly created profile
    m_currentProfile = &m_profiles.back();

    SaveProfilesToFile();
    std::cout << "Created new profile: " << name << std::endl;
}

void MenuManager::SelectProfile(int index) {
    if (index >= 0 && index < m_profiles.size()) {
        m_currentProfile = &m_profiles[index];
        std::cout << "Selected profile: " << m_currentProfile->name << std::endl;
    }
}

void MenuManager::DeleteProfile(int index) {
    if (index >= 0 && index < m_profiles.size()) {
        std::string profileName = m_profiles[index].name;

        // If deleting the current profile, reset current profile
        if (m_currentProfile == &m_profiles[index]) {
            m_currentProfile = nullptr;
        }

        m_profiles.erase(m_profiles.begin() + index);
        SaveProfilesToFile();

        std::cout << "Deleted profile: " << profileName << std::endl;
        ShowWarningMessage("Profile deleted: " + profileName);

        // Refresh the menu to show updated profile list
        SetMenuState(MenuState::ChooseProfile);
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

void MenuManager::ReturnToMenu() {
    SetMenuState(MenuState::PlayMenu);
}

bool MenuManager::IsGamePaused() const {
    return m_gamePaused;
}


void MenuManager::SaveProfilesToFile() {
    try {
        json root;
        json profilesArray = json::array();

        for (const auto& profile : m_profiles) {
            json profileJson;
            profileJson["name"] = profile.name;
            profileJson["level"] = profile.level;
            profileJson["experience"] = profile.experience;
            profileJson["highScore"] = profile.highScore;
            profileJson["savedLevel"] = profile.savedLevel;
            profileJson["savedDifficulty"] = profile.savedDifficulty;
            profileJson["savedGold"] = profile.savedGold;

            // Save tower positions
            json towersArray = json::array();
            for (const auto& tower : profile.savedTowers) {
                json towerJson;
                towerJson["x"] = tower.x;
                towerJson["y"] = tower.y;
                towerJson["type"] = tower.type;
                towerJson["level"] = tower.level;
                towersArray.push_back(towerJson);
            }
            profileJson["savedTowers"] = towersArray;

            // Save map layout
            json mapArray = json::array();
            for (const auto& row : profile.savedMapLayout) {
                json rowArray = json::array();
                for (int cell : row) {
                    rowArray.push_back(cell);
                }
                mapArray.push_back(rowArray);
            }
            profileJson["savedMapLayout"] = mapArray;

            // Save enemy path
            json pathArray = json::array();
            for (const auto& point : profile.savedEnemyPath) {
                json pointJson;
                pointJson["x"] = point.x;
                pointJson["y"] = point.y;
                pathArray.push_back(pointJson);
            }
            profileJson["savedEnemyPath"] = pathArray;

            profilesArray.push_back(profileJson);
        }

        root["profiles"] = profilesArray;

        std::ofstream file(PROFILES_FILE_PATH);
        if (file.is_open()) {
            file << root.dump(4); // Pretty print with 4 spaces indentation
            file.close();
            std::cout << "Profiles saved to " << PROFILES_FILE_PATH << std::endl;
        }
        else {
            std::cerr << "Could not open file for writing: " << PROFILES_FILE_PATH << std::endl;
        }
    }
    catch (const json::exception& e) {
        std::cerr << "JSON error while saving profiles: " << e.what() << std::endl;
    }
}

void MenuManager::LoadProfilesFromFile() {
    std::ifstream file(PROFILES_FILE_PATH);
    if (!file.is_open()) {
        std::cout << "No existing profiles file found. Starting with empty profile list." << std::endl;
        return;
    }

    try {
        json root;
        file >> root;

        m_profiles.clear();

        if (root.contains("profiles") && root["profiles"].is_array()) {
            for (const auto& profileJson : root["profiles"]) {
                PlayerProfile profile;
                profile.name = profileJson.value("name", "");
                profile.level = profileJson.value("level", 1);
                profile.experience = profileJson.value("experience", 0);
                profile.highScore = profileJson.value("highScore", 0);
                profile.savedLevel = profileJson.value("savedLevel", 1);
                profile.savedDifficulty = profileJson.value("savedDifficulty", 1.0f);
                profile.savedGold = profileJson.value("savedGold", 10);

                // Load tower positions
                if (profileJson.contains("savedTowers") && profileJson["savedTowers"].is_array()) {
                    for (const auto& towerJson : profileJson["savedTowers"]) {
                        TowerData tower;
                        tower.x = towerJson.value("x", 0);
                        tower.y = towerJson.value("y", 0);
                        tower.type = towerJson.value("type", 0);
                        tower.level = towerJson.value("level", 1);
                        profile.savedTowers.push_back(tower);
                    }
                }

                // Load map layout
                if (profileJson.contains("savedMapLayout") && profileJson["savedMapLayout"].is_array()) {
                    for (const auto& rowJson : profileJson["savedMapLayout"]) {
                        if (rowJson.is_array()) {
                            std::vector<int> row;
                            for (const auto& cellJson : rowJson) {
                                row.push_back(cellJson.get<int>());
                            }
                            profile.savedMapLayout.push_back(row);
                        }
                    }
                }

                // Load enemy path
                if (profileJson.contains("savedEnemyPath") && profileJson["savedEnemyPath"].is_array()) {
                    for (const auto& pointJson : profileJson["savedEnemyPath"]) {
                        PathPoint point;
                        point.x = pointJson.value("x", 0);
                        point.y = pointJson.value("y", 0);
                        profile.savedEnemyPath.push_back(point);
                    }
                }

                if (!profile.name.empty()) {
                    m_profiles.push_back(profile);
                }
            }
            std::cout << "Loaded " << m_profiles.size() << " profiles from " << PROFILES_FILE_PATH << std::endl;
        }
    }
    catch (const json::exception& e) {
        std::cerr << "JSON error while loading profiles: " << e.what() << std::endl;
    }

    file.close();
}