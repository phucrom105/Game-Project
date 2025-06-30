#ifndef SOUNDMANAGER_H
#define SOUNDMANAGER_H

#include <SFML/Audio.hpp>
#include <vector>
#include <string>

class SoundManager {
private:
    SoundManager();
    ~SoundManager();

public:
    static SoundManager& getInstance() {
        return m_Instance;
    }

    // Load audio files
    void Initialize();

    // Play different types of sounds
    void PlayBackgroundMusic();
    void PauseBackgroundMusic();
    void ResumeBackgroundMusic();
    void StopBackgroundMusic();
    void PlayThrowingSound();
    void PlayHitSound();
    void PlayEnemyDeathSound();
    void PlayTowerPlaceSound();
    void PlayGameOverSound();

    // Volume control
    void SetMusicVolume(float volume); // 0.0f to 100.0f
    void SetSoundVolume(float volume); // 0.0f to 100.0f

    // Cleanup
    void Cleanup();

private:
    static SoundManager m_Instance;

    // Music
    sf::Music m_BackgroundMusic;

    // Sound effects
    sf::SoundBuffer m_ThrowingSoundBuffer;
    sf::SoundBuffer m_HitSoundBuffer;
    sf::SoundBuffer m_EnemyDeathSoundBuffer;
    sf::SoundBuffer m_TowerPlaceSoundBuffer;
    sf::SoundBuffer m_GameOverSoundBuffer;

    // Sound objects (we need multiple for overlapping sounds)
    std::vector<sf::Sound> m_ThrowingSounds;
    std::vector<sf::Sound> m_HitSounds;
    std::vector<sf::Sound> m_EnemyDeathSounds;
    sf::Sound m_TowerPlaceSound;
    sf::Sound m_GameOverSound;

    // Settings
    float m_fMusicVolume;
    float m_fSoundVolume;

    // Helper methods
    void CreateSoundPool();
    sf::Sound* GetAvailableSound(std::vector<sf::Sound>& soundPool);
};

#endif