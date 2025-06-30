#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include "Entity.h"
#include <functional>

class MenuManager {
public:
    enum class MenuState {
        ProfileMenu,
        CreateProfile,
        ChooseProfile,
        MainMenu,
        PlayMenu,
        GamePlay,
        PauseMenu,
        Settings
    };

    enum class ButtonState {
        Normal,
        Hovered,
        Pressed
    };

    struct Button {
        sf::RectangleShape shape;
        sf::Text text;
        std::function<void()> callback;
        ButtonState state;
        bool isVisible;
        bool isDeleteButton;

        Button() : state(ButtonState::Normal), isVisible(true), isDeleteButton(false) {}
    };

    struct TowerData {
        int x, y;
        int type;
        int level;
    };

    struct PathPoint {
        int x, y;
    };

    struct PlayerProfile {
        std::string name;
        int level = 1;
        int experience = 0;
        int highScore = 0;
        int savedLevel = 1;
        float savedDifficulty = 1.0f;
        int savedGold = 10;

        // New fields for saving game state
        std::vector<TowerData> savedTowers;
        std::vector<std::vector<int>> savedMapLayout;
        std::vector<PathPoint> savedEnemyPath;

        PlayerProfile() = default;
        PlayerProfile(const std::string& playerName) : name(playerName) {}
    };

public:
    MenuManager();
    ~MenuManager();

    void Initialize(sf::RenderWindow& window);
    void Update(sf::RenderWindow& window, float deltaTime);
    void Draw(sf::RenderWindow& window);
    void HandleInput(sf::Event& event, sf::RenderWindow& window);
    void ReturnToMenu();
    void DrawPauseMenu(sf::RenderWindow& window); // Added sf::RenderWindow& parameter
    void TogglePauseMenu();
    void CreatePauseMenu();
    bool IsGamePaused() const;

    // Menu state management
    void SetMenuState(MenuState newState);
    MenuState GetMenuState() const { return m_currentState; }
    bool IsInGamePlay() const { return m_currentState == MenuState::GamePlay; }

    // Profile management
    void CreateNewProfile(const std::string& name);
    void SelectProfile(int index);
    void DeleteProfile(int index);
    const PlayerProfile* GetCurrentProfile() const { return m_currentProfile; }
    const std::vector<PlayerProfile>& GetProfiles() const { return m_profiles; }

    // Profile save/load functions
    void SaveProfilesToFile();
    void LoadProfilesFromFile();

    // Input handling for profile creation
    void SetWaitingForInput(bool waiting) { m_waitingForNameInput = waiting; }
    bool IsWaitingForInput() const { return m_waitingForNameInput; }
    void HandleTextInput(sf::Uint32 unicode);
    void ClearInputText();
    void SetExitCallback(std::function<void()> callback) { m_exitCallback = callback; }

private:
    // Menu creation functions
    void CreateProfileMenu();
    void CreateChooseProfileMenu();
    void CreateNewProfileMenu();
    void CreateMainMenu();
    void CreatePlayMenu();

    // Button management
    void CreateButton(const std::string& text, sf::Vector2f position, sf::Vector2f size,
        std::function<void()> callback);
    void CreateDeleteButton(const std::string& text, sf::Vector2f position, sf::Vector2f size,
        std::function<void()> callback);
    void CreatePauseButton(const std::string& text, sf::Vector2f position, sf::Vector2f size,
        std::function<void()> callback); // Added missing pause button creation function
    void UpdateButtons(sf::RenderWindow& window);
    void DrawButtons(sf::RenderWindow& window);
    bool IsMouseOverButton(const Button& button, sf::Vector2f mousePos);

    // Helper functions
    void CenterText(sf::Text& text, const sf::RectangleShape& shape);
    void LoadResources();
    void ShowWarningMessage(const std::string& message);

private:
    MenuState m_currentState;
    MenuState m_previousState;

    // Resources
    sf::Font m_font;
    sf::Texture m_backgroundTexture;
    sf::Sprite m_backgroundSprite;

    // UI Elements
    std::vector<Button> m_buttons;
    std::vector<Button> m_pauseButtons; // Added missing pause buttons vector
    sf::Text m_titleText;
    sf::Text m_warningText;
    sf::Text m_inputPromptText;
    sf::Text m_inputText;
    sf::RectangleShape m_inputBox;
    sf::RectangleShape m_pauseBackground; // Added missing pause background

    // Profile management
    std::vector<PlayerProfile> m_profiles;
    PlayerProfile* m_currentProfile;
    std::string m_inputBuffer;
    bool m_waitingForNameInput;
    bool m_showWarning;
    float m_warningTimer;
    bool m_gamePaused; // Added missing game paused state

    // Callback functions
    std::function<void()> m_exitCallback;

    // UI State
    sf::Vector2f m_windowSize;

    // Static constants
    static const sf::Color BUTTON_NORMAL_COLOR;
    static const sf::Color BUTTON_HOVER_COLOR;
    static const sf::Color BUTTON_PRESSED_COLOR;
    static const sf::Color TEXT_COLOR;
    static const sf::Color DELETE_BUTTON_COLOR;
    static const sf::Color DELETE_BUTTON_HOVER_COLOR;
    static const int BUTTON_HEIGHT = 60;
    static const int BUTTON_WIDTH = 300;
    static const int BUTTON_SPACING = 80;
    static const int MAX_PROFILES;

    // Profile file path
    static const std::string PROFILES_FILE_PATH;
};