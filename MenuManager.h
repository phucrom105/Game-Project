#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <functional>

class MenuManager {
public:
    enum class MenuState {
        ProfileMenu,
        CreateProfile,
        ChooseProfile,
        MainMenu,
        PlayMenu,
        GamePlay
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

        Button() : state(ButtonState::Normal), isVisible(true) {}
    };

    struct PlayerProfile {
        std::string name;
        int level;
        int experience;
        int highScore;

        PlayerProfile() : name(""), level(1), experience(0), highScore(0) {}
        PlayerProfile(const std::string& playerName)
            : name(playerName), level(1), experience(0), highScore(0) {
        }
    };

public:
    MenuManager();
    ~MenuManager();

    void Initialize(sf::RenderWindow& window);
    void Update(sf::RenderWindow& window, float deltaTime);
    void Draw(sf::RenderWindow& window);
    void HandleInput(sf::Event& event, sf::RenderWindow& window);

    // Menu state management
    void SetMenuState(MenuState newState);
    MenuState GetMenuState() const { return m_currentState; }
    bool IsInGamePlay() const { return m_currentState == MenuState::GamePlay; }

    // Profile management
    void CreateNewProfile(const std::string& name);
    void SelectProfile(int index);
    const PlayerProfile* GetCurrentProfile() const { return m_currentProfile; }
    const std::vector<PlayerProfile>& GetProfiles() const { return m_profiles; }

    // Input handling for profile creation
    void SetWaitingForInput(bool waiting) { m_waitingForNameInput = waiting; }
    bool IsWaitingForInput() const { return m_waitingForNameInput; }
    void HandleTextInput(sf::Uint32 unicode);
    void ClearInputText();

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
    sf::Text m_titleText;
    sf::Text m_warningText;
    sf::Text m_inputPromptText;
    sf::Text m_inputText;
    sf::RectangleShape m_inputBox;

    // Profile management
    std::vector<PlayerProfile> m_profiles;
    PlayerProfile* m_currentProfile;
    std::string m_inputBuffer;
    bool m_waitingForNameInput;
    bool m_showWarning;
    float m_warningTimer;

    // UI State
    sf::Vector2f m_windowSize;
    static const sf::Color BUTTON_NORMAL_COLOR;
    static const sf::Color BUTTON_HOVER_COLOR;
    static const sf::Color BUTTON_PRESSED_COLOR;
    static const sf::Color TEXT_COLOR;
    static const int BUTTON_HEIGHT = 60;
    static const int BUTTON_WIDTH = 300;
    static const int BUTTON_SPACING = 80;
};