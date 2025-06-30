#include "game.h"
#include <SFML/Graphics.hpp>
#include "MathHelpers.h"
#include <random>
#include <stdexcept>
#include <algorithm>
#include <cassert>
#include "DamageTextManager.h"
#include "SoundManager.h"
#include "MenuManager.h"

Game::Game()
    : m_Window(sf::VideoMode({ 1920 , 1080 }), "SFML window")
    , m_eGameMode(Play)
    , m_optionIndex(0)
    , m_eScrollWheelInput(None)
    , m_TowerTemplate(Entity::PhysicsData::Type::Static)
    , m_enemyTemplate(Entity::PhysicsData::Type::Dynamic)
    , m_axeTemplate(Entity::PhysicsData::Type::Dynamic)
    , m_bDrawPath(true)
    , m_iPlayerHealth(10)
    , m_iPlayerGold(10)
    , m_iCurrentLevel(1)
    , m_fTimeInPlayMode(0.0f)
    , m_fDifficulty(1.0f)
    , m_iGoldGainedThisUpdate(0)
    , m_fGoldPerSecond(0.0f)
    , m_fGoldPerSecondTimer(0.0f)
    , m_bGameRunning(true)
{

    // Initialize MenuManager first
    m_MenuManager.Initialize(m_Window);

    // Initialize SoundManager
    SoundManager::getInstance().Initialize();
    SoundManager::getInstance().PlayBackgroundMusic();


    // Load textures and check return values
    if (!towerTexture.loadFromFile("image/player.png")) {
        throw std::runtime_error("Failed to load player texture from 'image/player.png'");
    }
    if (!enemyTexture.loadFromFile("image/enemy.png")) {
        throw std::runtime_error("Failed to load enemy texture from 'image/enemy.png'");
    }
    if (!axeTexture.loadFromFile("image/axe.png")) {
        throw std::runtime_error("Failed to load axe texture from 'image/axe.png'");
    }

    // Set textures for sprites
    m_TowerTemplate.SetTexture(towerTexture);
    m_TowerTemplate.SetScale(sf::Vector2f(5, 5));
    m_TowerTemplate.SetOrigin(sf::Vector2f(8, 8));
    m_TowerTemplate.setCirclePhysics(40.f);
    m_TowerTemplate.GetPhysicsDataNonConst().setLayers(Entity::PhysicsData::Layer::Tower);

    m_enemyTemplate.SetTexture(enemyTexture);
    m_enemyTemplate.SetScale(sf::Vector2f(5, 5));
    m_enemyTemplate.SetPosition(sf::Vector2f(960, 540));
    m_enemyTemplate.SetOrigin(sf::Vector2f(8, 8));
    m_enemyTemplate.setCirclePhysics(40.f); // Set the enemy as a circle with a radius of 80 pixels
    m_enemyTemplate.GetPhysicsDataNonConst().setLayers(Entity::PhysicsData::Layer::Enemy);
    m_enemyTemplate.SetHealth(3);

    m_axeTemplate.SetTexture(axeTexture);
    m_axeTemplate.SetScale(sf::Vector2f(5, 5));
    m_axeTemplate.SetOrigin(sf::Vector2f(8, 8));
    m_axeTemplate.setCirclePhysics(40.f); // Set the axe as a circle with a radius of 80 pixels
    m_axeTemplate.GetPhysicsDataNonConst().setLayers(Entity::PhysicsData::Layer::Projectile);
    m_axeTemplate.GetPhysicsDataNonConst().setLayersToIgnore(Entity::PhysicsData::Layer::Projectile | Entity::PhysicsData::Layer::Tower);

    m_Font.loadFromFile("Fonts/Kreon-Medium.ttf");

    m_GameModeText.setPosition(sf::Vector2f(1000, 200));
    m_GameModeText.setFont(m_Font);
    m_GameModeText.setString("Menu Mode");

    m_PlayerText.setPosition(sf::Vector2f(1500, 100));
    m_PlayerText.setString("Player");
    m_PlayerText.setFont(m_Font);

    m_GameOverText.setPosition(sf::Vector2f(1080, 800));
    m_GameOverText.setString("GAME OVERRR");
    m_GameOverText.setFont(m_Font);
    m_GameOverText.setCharacterSize(100);

    m_TileMapTexture.loadFromFile("image/TileMap.png");
    for (int j = 0; j < 4; j++) {
        for (int i = 0; i < 4; i++) {
            sf::Sprite tileSprite;
            tileSprite.setTexture(m_TileMapTexture);
            tileSprite.setTextureRect(sf::IntRect(i * 16, j * 16, 16, 16));
            tileSprite.setScale(sf::Vector2f(10, 10));
            tileSprite.setOrigin(sf::Vector2f(8, 8));

            TileOptions::TileType eTileType = TileOptions::TileType::Null;

            if (j == 0) {
                eTileType = TileOptions::TileType::Aesthetic;
            }
            else {
                if (j == 1) {
                    if (i == 0) {
                        eTileType = TileOptions::TileType::Spawn;
                    }
                    else if (i == 1) {
                        eTileType = TileOptions::TileType::End;
                    }
                    else if (i == 2) {
                        eTileType = TileOptions::TileType::Path;
                    }
                }
            }

            m_MenuManager.Initialize(m_Window);

            TileOptions& tileOption = m_TileOptions.emplace_back(eTileType);
            tileOption.setSprite(tileSprite);
        }
    }
    m_MenuManager.SetExitCallback([this]() {
        this->ExitGame();
        });
}

Game::~Game() {
    SoundManager::getInstance().Cleanup();
}
//enum class MenuItem { Play, Setting, Exit, NewProfile, ExistingProfile, PlayAsGuest, Back, Start };

void Game::run() {
    sf::Clock clock;
    while (m_Window.isOpen()) {
        m_deltaTime = clock.restart();
        HandleInput();

        // Kiểm tra nếu đang trong menu
        if (!m_MenuManager.IsInGamePlay()) {
            m_MenuManager.Update(m_Window, m_deltaTime.asSeconds());
        }
        else {
            // Chỉ update game logic khi đang chơi và không pause
            if (!m_MenuManager.IsGamePaused()) {
                switch (m_eGameMode) {
                case Play:
                    UpdatePlay();
                    break;
                case LevelEditor:
                    UpdateLevelEditor();
                    break;
                }
            }
        }
        Draw();
    }
}

void Game::UpdatePlay() {

    //Dừng mọi hoạt động nếu game pause
    if (m_MenuManager.IsGamePaused()) {
        return;
    }

    m_fTimeInPlayMode += m_deltaTime.asSeconds();
    m_fDifficulty += m_deltaTime.asSeconds() / 10.0f;
    if (m_iPlayerHealth <= 0) {
        static bool gameOverSoundPlayed = false;
        if (!gameOverSoundPlayed) {
            SoundManager::getInstance().StopBackgroundMusic();
            SoundManager::getInstance().PlayGameOverSound();
            gameOverSoundPlayed = true;
        }
        return;
    }

    DamageTextManager::getInstanceNonConst().Update(m_deltaTime);
    UpdateTower();
    UpdateAxe();

    const int iMaxEnemies = 30;
    if (m_SpawnTiles.size() > 0 && !m_Paths.empty()) {
        m_enemyTemplate.SetPosition(m_SpawnTiles[0].GetPosition());
        if (m_enemies.size() < iMaxEnemies) {
            static float fSpawnTimer = 0.0f;
            //Speed up the Spawn Rate after 5 seconds
            float fSpawnRate = m_fDifficulty;
            // After 1 minutes, the spawn rate will be 2.2f
            fSpawnTimer += m_deltaTime.asSeconds() * fSpawnRate;
            if (fSpawnTimer > 1.0f) {
                // Randomly spawn enemies
                Entity& newEnemy = m_enemies.emplace_back(m_enemyTemplate);
                newEnemy.SetPathIndex(rand() % m_Paths.size()); // Assign a random path index
                fSpawnTimer = 0.0f;
            }
        }
    }

    for (int i = m_enemies.size() - 1; i >= 0; --i) {
        Entity& rEnemy = m_enemies[i];
        Path& path = m_Paths[rEnemy.GetPathIndex()];

        //Find closest PathTile to the enemy
        PathTile* pClosestTile = nullptr;
        float fClosestDistance = std::numeric_limits<float>::max();

        for (PathTile& tile : path) {
            sf::Vector2f vEnemyToTile = tile.pCurrentTile->GetPosition() - rEnemy.GetPosition();
            float fDistance = MathHelpers::flength(vEnemyToTile);

            if (fDistance < fClosestDistance) {
                fClosestDistance = fDistance;
                pClosestTile = &tile;
            }
        }
        // Find the next path tile
        if (!pClosestTile || !pClosestTile->pNextTile) continue;
        const Entity* pNextTile = pClosestTile->pNextTile;

        if (pNextTile->GetClosestGridCoordinates() == m_EndTiles[0].GetClosestGridCoordinates()) {
            if (fClosestDistance < 40.0f) {
                // Enemy reached the end tile, remove it
                m_enemies.erase(m_enemies.begin() + i);
                //m_iPlayerHealth -= 1;
                m_fDifficulty *= 0.9f;
                continue; // Skip to the next enemy
            }
        }

        float fEnemySpeed = 250.0f;
        sf::Vector2f vEnemyToNextTile = pNextTile->GetPosition() - rEnemy.GetPosition();
        vEnemyToNextTile = MathHelpers::normalize(vEnemyToNextTile);
        rEnemy.SetVelocity(vEnemyToNextTile * fEnemySpeed);
    }
    UpdatePhysics();
    CheckForDeletionRequest();

    m_fGoldPerSecondTimer += m_deltaTime.asSeconds();
    if (m_fGoldPerSecondTimer > 0.05f) {
        m_fGoldPerSecond = m_fGoldPerSecond * 0.9f + 0.1f * m_iGoldGainedThisUpdate / m_fGoldPerSecondTimer;
        m_fGoldPerSecondTimer = 0.0f;
        m_iGoldGainedThisUpdate = 0;
    }
}

void Game::UpdateTower() {

    //Dừng update tower nếu game pause
    if (m_MenuManager.IsGamePaused()) {
        return;
    }

    for (Entity& tower : m_Towers) {
        //Check if it is time to throw an axe
        tower.m_fAttackTimer -= m_deltaTime.asSeconds();
        if (tower.m_fAttackTimer > 0.0f) continue; // Not time to throw an axe yet

        //Find the closest enemy to the tower
        Entity* pClosestEnemy = nullptr;
        float fClosestDistance = std::numeric_limits<float>::max();
        for (Entity& enemy : m_enemies) {
            sf::Vector2f vTowerToEnemy = enemy.GetPosition() - tower.GetPosition();
            float fDistance = MathHelpers::flength(vTowerToEnemy);
            if (fDistance < fClosestDistance) {
                fClosestDistance = fDistance;
                pClosestEnemy = &enemy;
            }
        }

        if (!pClosestEnemy) {
            continue; // No enemies in range
        }

        // Rotate the tower to face the enemy
        sf::Vector2f vTowerToEnemy = pClosestEnemy->GetPosition() - tower.GetPosition();
        float fAngle = MathHelpers::Angle(vTowerToEnemy);
        tower.GetSpriteNonConst().setRotation(fAngle);

        //Create an axe and set its velocity
        Entity& newAxe = m_axes.emplace_back(m_axeTemplate);
        newAxe.SetPosition(tower.GetPosition());
        vTowerToEnemy = MathHelpers::normalize(vTowerToEnemy);
        newAxe.SetVelocity(vTowerToEnemy * 500.0f);

        // Play hit/attack sound
        SoundManager::getInstance().PlayHitSound();

        //Reset the axe throw
        tower.m_fAttackTimer = 1.0f;
    }
}

void Game::UpdateAxe() {

    //Dừng update axe nếu game pause
    if (m_MenuManager.IsGamePaused()) {
        return;
    }

    for (Entity& axe : m_axes) {
        axe.m_fAxeTimer -= m_deltaTime.asSeconds();
        const float fAxeRotationSpeed = 360.0f;
        axe.GetSpriteNonConst().rotate(fAxeRotationSpeed * m_deltaTime.asSeconds());
        if (axe.m_fAxeTimer <= 0.0f) {
            axe.RequestDeletion();
        }
    }
}

void Game::CheckForDeletionRequest() {

    if (m_MenuManager.IsGamePaused()) {
        return;
    }

    for (int i = m_axes.size() - 1; i >= 0; i--) {
        Entity& axe = m_axes[i];
        if (axe.IsDeletionRequested()) {
            m_axes.erase(m_axes.begin() + i);
        }
    }

    for (int i = m_enemies.size() - 1; i >= 0; i--) {
        Entity& enemy = m_enemies[i];
        if (enemy.IsDeletionRequested()) {
            m_enemies.erase(m_enemies.begin() + i);
            //m_iPlayerGold += 1;
            AddGold(1);
            // Play enemy death sound
            SoundManager::getInstance().PlayEnemyDeathSound();
        }
    }
}

void Game::UpdateLevelEditor() {

    //m_enemies.clear(); // Clear enemies in level editor mode
    //m_axes.clear();
    //m_Towers.clear();

    m_iPlayerGold = 10;
    m_iPlayerHealth = 10;
    m_iGoldGainedThisUpdate = 0;
    m_fTimeInPlayMode = 0.0f;
    m_fDifficulty = 1.0f;
    m_fGoldPerSecond = 0.0f;
    m_fGoldPerSecondTimer = 0.0f;
}

void Game::UpdatePhysics() {

    if (m_MenuManager.IsGamePaused()) {
        return;
    }

    const float fMaxDeltaTime = 0.1f; // Cap the delta time to prevent large jumps
    const float fDeltaTime = std::min(m_deltaTime.asSeconds(), fMaxDeltaTime);

    vector <Entity*> AllEntities;

    for (Entity& tower : m_Towers) {
        AllEntities.push_back(&tower);
    }

    for (Entity& enemy : m_enemies) {
        AllEntities.push_back(&enemy);
    }

    for (Entity& axe : m_axes) {
        AllEntities.push_back(&axe);
    }

    for (Entity* entity : AllEntities) {
        entity->GetPhysicsDataNonConst().ClearCollisions();
    }

    for (Entity* entity : AllEntities) {

        if (entity->GetPhysicsData().m_eType == Entity::PhysicsData::Type::Dynamic) {
            entity->move(entity->GetPhysicsData().m_vVelocity * fDeltaTime + entity->GetPhysicsData().m_vImpulse);
            entity->GetPhysicsDataNonConst().ClearImpulse();

            // Check collisions
            for (Entity* otherEntity : AllEntities) {
                if (entity == otherEntity) continue; // Skip self-collision
                if (entity->shouldIgnoreEntityForPhysics(otherEntity)) continue; // Skip ignored entities

                if (!entity->GetPhysicsDataNonConst().HasCollidedThisUpdate(otherEntity) && isColiding(*entity, *otherEntity)) {
                    entity->OnCollision(*otherEntity);
                    otherEntity->OnCollision(*entity);

                    entity->GetPhysicsDataNonConst().AddEntityCollision(otherEntity);
                    otherEntity->GetPhysicsDataNonConst().AddEntityCollision(entity);
                }
                ProcessCollision(*entity, *otherEntity);
            }
        }
    }
}

void Game::ProcessCollision(Entity& entity1, Entity& entity2) {
    assert(entity1.GetPhysicsData().m_eType != Entity::PhysicsData::Type::Static);
    if (entity1.GetPhysicsData().m_eShape == Entity::PhysicsData::Shape::Circle) {
        // we are circle
        if (entity2.GetPhysicsData().m_eShape == Entity::PhysicsData::Shape::Circle) {
            // Both are circles
            const sf::Vector2f vEntity1ToEntity2 = entity2.GetPosition() - entity1.GetPosition();
            const float fDistanceBeeenEntities = MathHelpers::flength(vEntity1ToEntity2);
            float fSumOfRadii = entity1.GetPhysicsData().m_fRadius + entity2.GetPhysicsData().m_fRadius;

            if (fDistanceBeeenEntities < fSumOfRadii) {
                const bool isEntity2Dynamic = entity2.GetPhysicsData().m_eType == Entity::PhysicsData::Type::Dynamic;
                if (!isEntity2Dynamic) {
                    // We only need to move entity1
                    entity1.move(-MathHelpers::normalize(vEntity1ToEntity2) * (fSumOfRadii - fDistanceBeeenEntities));
                }
                else {
                    // Both entities are dynamic, we need to move both of them
                    const sf::Vector2f vEntity1ToEntity2Normalized = MathHelpers::normalize(vEntity1ToEntity2);
                    const sf::Vector2f vEntity1Movement = vEntity1ToEntity2Normalized * (fSumOfRadii - fDistanceBeeenEntities) * 0.5f;
                    entity1.move(-vEntity1Movement);
                    entity2.move(vEntity1Movement);
                }
            }
        }
        else if (entity2.GetPhysicsData().m_eShape == Entity::PhysicsData::Shape::Rectangle) {
            // We are circle, they are rectangle
            float fClosestX = std::clamp(entity1.GetPosition().x, entity2.GetPosition().x - entity2.GetPhysicsData().m_fWidth / 2, entity2.GetPosition().x + entity2.GetPhysicsData().m_fWidth / 2);
            float fClosestY = std::clamp(entity1.GetPosition().y, entity2.GetPosition().y - entity2.GetPhysicsData().m_fHeight / 2, entity2.GetPosition().y + entity2.GetPhysicsData().m_fHeight / 2);

            sf::Vector2f vClosestPoint(fClosestX, fClosestY);
            sf::Vector2f vCircleToClosestPoint = vClosestPoint - entity1.GetPosition();
            float fDistanceToClosestPoint = MathHelpers::flength(vCircleToClosestPoint);

            if (fDistanceToClosestPoint < entity1.GetPhysicsData().m_fRadius) {
                const bool isEntity2Dynamic = entity2.GetPhysicsData().m_eType == Entity::PhysicsData::Type::Dynamic;
                if (!isEntity2Dynamic) {
                    // We only need to move entity1
                    entity1.move(-MathHelpers::normalize(vCircleToClosestPoint) * (entity1.GetPhysicsData().m_fRadius - fDistanceToClosestPoint));
                }
                else {
                    const sf::Vector2f vEntity1ToEntity2Normalized = MathHelpers::normalize(vCircleToClosestPoint);
                    const sf::Vector2f vEntity1Movement = vEntity1ToEntity2Normalized * (entity1.GetPhysicsData().m_fRadius - fDistanceToClosestPoint) * 0.5f;
                    entity1.move(-vEntity1Movement);
                    entity2.move(vEntity1Movement);
                }
            }
        }
    }
    else if (entity1.GetPhysicsData().m_eShape == Entity::PhysicsData::Shape::Rectangle) {
        // we are rectangle
        if (entity2.GetPhysicsData().m_eShape == Entity::PhysicsData::Shape::Rectangle) {
            // Both are rectangles
            float fDistanceX = std::abs(entity1.GetPosition().x - entity2.GetPosition().x);
            float fDistanceY = std::abs(entity1.GetPosition().y - entity2.GetPosition().y);

            float fOverlapX = (entity1.GetPhysicsData().m_fWidth + entity2.GetPhysicsData().m_fWidth) / 2 - fDistanceX;
            float fOverlapY = (entity1.GetPhysicsData().m_fHeight + entity2.GetPhysicsData().m_fHeight) / 2 - fDistanceY;
            if (fOverlapX > 0 && fOverlapY > 0) {
                const bool isEntity2Dynamic = entity2.GetPhysicsData().m_eType == Entity::PhysicsData::Type::Dynamic;
                // Guarantee a collision
                if (fOverlapX < fOverlapY) {
                    if (entity1.GetPosition().x < entity2.GetPosition().x) {
                        if (isEntity2Dynamic) {
                            entity1.move(sf::Vector2f(-fOverlapX / 2, 0));
                            entity2.move(sf::Vector2f(fOverlapX / 2, 0));
                        }
                        else {
                            entity1.move(sf::Vector2f(-fOverlapX, 0));
                        }
                    }
                    else {
                        if (isEntity2Dynamic) {
                            entity1.move(sf::Vector2f(fOverlapX / 2, 0));
                            entity2.move(sf::Vector2f(-fOverlapX / 2, 0));
                        }
                        else {
                            entity1.move(sf::Vector2f(fOverlapX, 0));
                        }
                    }
                }
                else {
                    if (entity1.GetPosition().y < entity2.GetPosition().y) {
                        if (isEntity2Dynamic) {
                            entity1.move(sf::Vector2f(0, -fOverlapY / 2));
                            entity2.move(sf::Vector2f(0, fOverlapY / 2));
                        }
                        else {
                            entity1.move(sf::Vector2f(0, -fOverlapY));
                        }
                    }
                    else {
                        if (isEntity2Dynamic) {
                            entity1.move(sf::Vector2f(0, fOverlapY / 2));
                            entity2.move(sf::Vector2f(0, -fOverlapY / 2));
                        }
                        else {
                            entity1.move(sf::Vector2f(0, fOverlapY));
                        }
                    }
                }
            }
        }
        else if (entity2.GetPhysicsData().m_eShape == Entity::PhysicsData::Shape::Circle) {
            // We are rectangle, they are circle
            float fClosestX = std::clamp(entity2.GetPosition().x, entity1.GetPosition().x - entity1.GetPhysicsData().m_fWidth / 2, entity1.GetPosition().x + entity1.GetPhysicsData().m_fWidth / 2);
            float fClosestY = std::clamp(entity2.GetPosition().y, entity1.GetPosition().y - entity1.GetPhysicsData().m_fHeight / 2, entity1.GetPosition().y + entity1.GetPhysicsData().m_fHeight / 2);

            sf::Vector2f vClosestPoint(fClosestX, fClosestY);
            sf::Vector2f vCircleToClosestPoint = vClosestPoint - entity2.GetPosition();
            float fDistanceToClosestPoint = MathHelpers::flength(vCircleToClosestPoint);

            if (fDistanceToClosestPoint < entity2.GetPhysicsData().m_fRadius) {
                const bool isEntity2Dynamic = entity2.GetPhysicsData().m_eType == Entity::PhysicsData::Type::Dynamic;
                if (!isEntity2Dynamic) {
                    // We only need to move entity1
                    entity1.move(MathHelpers::normalize(vCircleToClosestPoint) * (entity2.GetPhysicsData().m_fRadius - fDistanceToClosestPoint));
                }
                else {
                    const sf::Vector2f vEntity2ToEntity1Normalized = MathHelpers::normalize(vCircleToClosestPoint);
                    const sf::Vector2f vEntity2Movement = vEntity2ToEntity1Normalized * (entity2.GetPhysicsData().m_fRadius - fDistanceToClosestPoint) * 0.5f;
                    entity1.move(vEntity2Movement);
                    entity2.move(-vEntity2Movement);
                }
            }
        }
    }
}

bool Game::isColiding(const Entity& entity1, const Entity& entity2) {
    if (entity1.GetPhysicsData().m_eShape == Entity::PhysicsData::Shape::Circle) {
        // we are circle
        if (entity2.GetPhysicsData().m_eShape == Entity::PhysicsData::Shape::Circle) {
            // Both are circles
            const sf::Vector2f vEntity1ToEntity2 = entity2.GetPosition() - entity1.GetPosition();
            const float fDistanceBeeenEntities = MathHelpers::flength(vEntity1ToEntity2);
            float fSumOfRadii = entity1.GetPhysicsData().m_fRadius + entity2.GetPhysicsData().m_fRadius;

            if (fDistanceBeeenEntities < fSumOfRadii) {
                return true;
            }
        }
        else if (entity2.GetPhysicsData().m_eShape == Entity::PhysicsData::Shape::Rectangle) {
            // We are circle, they are rectangle
            float fClosestX = std::clamp(entity1.GetPosition().x, entity2.GetPosition().x - entity2.GetPhysicsData().m_fWidth / 2, entity2.GetPosition().x + entity2.GetPhysicsData().m_fWidth / 2);
            float fClosestY = std::clamp(entity1.GetPosition().y, entity2.GetPosition().y - entity2.GetPhysicsData().m_fHeight / 2, entity2.GetPosition().y + entity2.GetPhysicsData().m_fHeight / 2);

            sf::Vector2f vClosestPoint(fClosestX, fClosestY);
            sf::Vector2f vCircleToClosestPoint = vClosestPoint - entity1.GetPosition();
            float fDistanceToClosestPoint = MathHelpers::flength(vCircleToClosestPoint);

            if (fDistanceToClosestPoint < entity1.GetPhysicsData().m_fRadius) {
                return true;
            }
        }
    }
    else if (entity1.GetPhysicsData().m_eShape == Entity::PhysicsData::Shape::Rectangle) {
        // we are rectangle
        if (entity2.GetPhysicsData().m_eShape == Entity::PhysicsData::Shape::Rectangle) {
            // Both are rectangles
            float fDistanceX = std::abs(entity1.GetPosition().x - entity2.GetPosition().x);
            float fDistanceY = std::abs(entity1.GetPosition().y - entity2.GetPosition().y);

            float fOverlapX = (entity1.GetPhysicsData().m_fWidth + entity2.GetPhysicsData().m_fWidth) / 2 - fDistanceX;
            float fOverlapY = (entity1.GetPhysicsData().m_fHeight + entity2.GetPhysicsData().m_fHeight) / 2 - fDistanceY;
            if (fOverlapX > 0 && fOverlapY > 0) {
                return true;
            }
        }
        else if (entity2.GetPhysicsData().m_eShape == Entity::PhysicsData::Shape::Circle) {
            // We are rectangle, they are circle
            float fClosestX = std::clamp(entity2.GetPosition().x, entity1.GetPosition().x - entity1.GetPhysicsData().m_fWidth / 2, entity1.GetPosition().x + entity1.GetPhysicsData().m_fWidth / 2);
            float fClosestY = std::clamp(entity2.GetPosition().y, entity1.GetPosition().y - entity1.GetPhysicsData().m_fHeight / 2, entity1.GetPosition().y + entity1.GetPhysicsData().m_fHeight / 2);

            sf::Vector2f vClosestPoint(fClosestX, fClosestY);
            sf::Vector2f vCircleToClosestPoint = vClosestPoint - entity2.GetPosition();
            float fDistanceToClosestPoint = MathHelpers::flength(vCircleToClosestPoint);

            if (fDistanceToClosestPoint < entity2.GetPhysicsData().m_fRadius) {
                return true;
            }
        }
    }
    return false;
}

void Game::DrawPlay() {
    sf::Vector2f vMousePosition = (sf::Vector2f)sf::Mouse::getPosition(m_Window);
    m_TowerTemplate.SetPosition(vMousePosition);

    if (CanPlaceTowerAtPosition(vMousePosition)) {
        m_TowerTemplate.SetColor(sf::Color::Green);
    }
    else {
        m_TowerTemplate.SetColor(sf::Color::Red);
    }

    for (const Entity& tower : m_Towers) {
        m_Window.draw(tower);
    }

    for (const Entity& enemy : m_enemies) {
        m_Window.draw(enemy);
    }

    for (const Entity& axe : m_axes) {
        m_Window.draw(axe);
    }

    DamageTextManager::getInstanceConst().Draw(m_Window);


    m_Window.draw(m_TowerTemplate); // Draw the tower template

    if (m_iPlayerHealth <= 0) {
        //draw the game over text
        m_Window.draw(m_GameOverText);
    }

    m_PlayerText.setString("Difficulty: " + to_string(m_fDifficulty) +
        "\nPlayer's Gold: " + to_string(m_iPlayerGold) +
        "\nGold Per Second: " + to_string(m_fGoldPerSecond));
    m_Window.draw(m_PlayerText);
}

void Game::Draw() {
    // Erase the previous frame
    m_Window.clear();

    // Nếu đang trong menu, chỉ vẽ menu
    if (!m_MenuManager.IsInGamePlay()) {
        m_MenuManager.Draw(m_Window);
    }
    else {
        // Vẽ game content khi đang chơi
        for (const Entity& entity : m_AestheticTiles) {
            m_Window.draw(entity);
        }

        // Draw the game mode text 
        m_Window.draw(m_GameModeText);

        switch (m_eGameMode) {
        case Play:
            DrawPlay();
            break;
        case LevelEditor:
            DrawLevelEditor();
            break;
        }

        if (m_MenuManager.IsGamePaused()) {
            m_MenuManager.Draw(m_Window);
        }
    }

    m_Window.display();
}

void Game::HandleInput() {
    sf::Event event;
    m_eScrollWheelInput = None;

    while (m_Window.pollEvent(event)) {
        // Xử lý sự kiện đóng cửa sổ
        if (event.type == sf::Event::Closed) {
            m_Window.close();
            return;
        }

        // Nếu đang trong menu, chuyển input cho MenuManager
        if (!m_MenuManager.IsInGamePlay()) {
            m_MenuManager.HandleInput(event, m_Window);

            // Kiểm tra nếu game đã bắt đầu từ menu
            if (m_MenuManager.IsInGamePlay()) {
                m_eGameMode = Play; // Luôn bắt đầu ở Play mode
                m_GameModeText.setString("Play Mode");
                // Reset game state khi bắt đầu game mới
                ResetGameState();
            }
        }
        else {
            // Xử lý pause/resume music dựa trên trạng thái game
            static bool wasPaused = false;
            bool currentlyPaused = m_MenuManager.IsGamePaused();

            if (currentlyPaused && !wasPaused) {
                // Vừa chuyển sang trạng thái pause
                SoundManager::getInstance().PauseBackgroundMusic();
            }
            else if (!currentlyPaused && wasPaused) {
                // Vừa thoát khỏi trạng thái pause
                SoundManager::getInstance().ResumeBackgroundMusic();
            }
            wasPaused = currentlyPaused;

            // Nếu đang trong gameplay, kiểm tra pause menu trước
            if (m_MenuManager.IsGamePaused()) {
                // Nếu game đang pause, chuyển input cho MenuManager để xử lý pause menu
                m_MenuManager.HandleInput(event, m_Window);
            }
            else {
                // Xử lý input trong game bình thường
                HandleGameInput(event);
            }
        }
    }

    // Chỉ xử lý keyboard input khi đang trong game
    if (m_MenuManager.IsInGamePlay() && !m_MenuManager.IsGamePaused()) {
        HandleKeyboardInput();
    }
}

void Game::HandleGameInput(sf::Event& event) {
    switch (event.type) {
    case sf::Event::MouseWheelScrolled:
        if (event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel) {
            if (event.mouseWheelScroll.delta > 0) {
                m_eScrollWheelInput = ScrollUp;
            }
            else {
                m_eScrollWheelInput = ScrollDown;
            }
        }
        break;
    case sf::Event::KeyPressed:
        // ESC để quay về menu
        if (event.key.code == sf::Keyboard::Escape) {
            m_MenuManager.TogglePauseMenu();
        }
        // Thêm phím Tab để chuyển đổi giữa Play và Level Editor (chỉ khi đang trong game)
        else if (event.key.code == sf::Keyboard::Tab) {
            if (m_eGameMode == Play) {
                m_eGameMode = LevelEditor;
                m_GameModeText.setString("Level Editor Mode");
            }
            else {
                m_eGameMode = Play;
                m_GameModeText.setString("Play Mode");
            }
        }
        break;
    }
}

void Game::HandleKeyboardInput() {
    // Phím T để chuyển đổi giữa Play và Level Editor
    static bool bTwasPressedLastUpdate = false;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::T)) {
        if (!bTwasPressedLastUpdate) {  
            if (m_eGameMode == Play) {
                m_eGameMode = LevelEditor;
                m_GameModeText.setString("Level Editor Mode");
            }
            else {
                m_eGameMode = Play;
                m_GameModeText.setString("Play Mode");
            }
        }
        bTwasPressedLastUpdate = true;
    }
    else {
        bTwasPressedLastUpdate = false;
    }

    // Xử lý input theo game mode
    switch (m_eGameMode) {
    case Play:
        HandlePlayInput();
        break;
    case LevelEditor:
        HandleLevelEditorInput();
        break;
    }
}

// Thêm các hàm mới để hỗ trợ menu
void Game::StartGame(int level) {
    m_iCurrentLevel = level;
    m_MenuManager.SetMenuState(MenuManager::MenuState::GamePlay);
    m_eGameMode = Play;
    m_GameModeText.setString("Play Mode");

    ResetGameState();

    // Nếu có profile được chọn và đã lưu dữ liệu
    if (m_MenuManager.GetCurrentProfile()) {
        const MenuManager::PlayerProfile* profile = m_MenuManager.GetCurrentProfile();
        if (!profile->savedTowers.empty()) {
            m_MenuManager.SaveProfilesToFile();
        }
    }
}

void Game::ReturnToMenu() {
    m_MenuManager.SetMenuState(MenuManager::MenuState::MainMenu);
    m_GameModeText.setString("Menu Mode");

    m_eGameMode = Play;

    // Dừng nhạc nền game và phát nhạc menu
    SoundManager::getInstance().StopBackgroundMusic();
    SoundManager::getInstance().PlayBackgroundMusic();
}

void Game::ExitGame() {
    // Nếu có profile đang được chọn
    if (m_MenuManager.GetCurrentProfile()) {
        MenuManager::PlayerProfile* profile = const_cast<MenuManager::PlayerProfile*>(m_MenuManager.GetCurrentProfile());

        m_MenuManager.SaveProfilesToFile();

        std::cout << "📁 Đã lưu dữ liệu profile: " << profile->name << std::endl;
    }

    m_Window.close();
}

void Game::ResetGameState() {
    // Reset tất cả game state về trạng thái ban đầu
    m_enemies.clear();
    m_axes.clear();
    m_Towers.clear();

    m_iPlayerHealth = 10;
    m_iPlayerGold = 10;
    m_iGoldGainedThisUpdate = 0;
    m_fTimeInPlayMode = 0.0f;
    m_fDifficulty = 1.0f;
    m_fGoldPerSecond = 0.0f;
    m_fGoldPerSecondTimer = 0.0f;
    m_bGameRunning = true;
}

void Game::CreateTileAtPosition(const sf::Vector2f& pos) {
    int x = pos.x / 160;
    int y = pos.y / 160;

    TileOptions::TileType eTileType = m_TileOptions[m_optionIndex].getTileType();
    if (eTileType == TileOptions::TileType::Null) return;

    vector<Entity>& ListOfTiles = GetListOfTiles(eTileType);

    if (eTileType == TileOptions::TileType::Spawn || eTileType == TileOptions::TileType::End) {
        ListOfTiles.clear(); // Clear existing spawn or end tiles (if more than 1)
    }

    sf::Sprite tile = m_TileOptions[m_optionIndex].getSprite();
    tile.setPosition(x * 160 + 80, y * 160 + 80);

    for (int i = 0; i < ListOfTiles.size(); i++) {
        if (ListOfTiles[i].GetPosition() == tile.getPosition()) {
            ListOfTiles[i] = ListOfTiles.back(); // Move the last tile to the current position
            ListOfTiles.pop_back(); // Remove the last tile
            break; // Tile already exists at this position, do not add a duplicate
        }
    }

    Entity& new_tiles = ListOfTiles.emplace_back(Entity::PhysicsData::Type::Static);
    new_tiles.SetSprite(tile);
    new_tiles.setRectanglePhysics(160.0f, 160.0f);
    ConstructionPath();
}

void Game::DeleteTileAtPosition(const sf::Vector2f& pos) {
    int x = pos.x / 160;
    int y = pos.y / 160;

    // Calculate the tile position based on the grid size (160x160)
    sf::Vector2f tilePosition(x * 160 + 80, y * 160 + 80);

    TileOptions::TileType eTileType = m_TileOptions[m_optionIndex].getTileType();
    if (eTileType == TileOptions::TileType::Null) return;
    vector<Entity>& ListOfTiles = GetListOfTiles(eTileType);

    for (int i = 0; i < ListOfTiles.size(); i++) {
        if (ListOfTiles[i].GetPosition() == tilePosition) {
            ListOfTiles[i] = ListOfTiles.back(); // Move the last tile to the current position
            ListOfTiles.pop_back(); // Remove the last tile
            break; // Tile found and removed
        }
    }
}

void Game::ConstructionPath() {
    m_Paths.clear();
    if (m_SpawnTiles.empty() || m_EndTiles.empty()) {
        return;
    }

    Path newPath;
    PathTile& start = newPath.emplace_back();
    start.pCurrentTile = &m_SpawnTiles[0];

    sf::Vector2i vEndCoords = m_EndTiles[0].GetClosestGridCoordinates();
    VisitPathNeighbors(newPath, vEndCoords);
}

void Game::VisitPathNeighbors(Path path, const sf::Vector2i& rEndCoords) {
    const sf::Vector2i vCurrentTilePosition = path.back().pCurrentTile->GetClosestGridCoordinates();

    const sf::Vector2i vNorthCoords(vCurrentTilePosition.x, vCurrentTilePosition.y - 1);
    const sf::Vector2i vEastCoords(vCurrentTilePosition.x + 1, vCurrentTilePosition.y);
    const sf::Vector2i vSouthCoords(vCurrentTilePosition.x, vCurrentTilePosition.y + 1);
    const sf::Vector2i vWestCoords(vCurrentTilePosition.x - 1, vCurrentTilePosition.y);

    if (rEndCoords == vNorthCoords || rEndCoords == vEastCoords || rEndCoords == vSouthCoords || rEndCoords == vWestCoords) {
        // Set the last tile in our current path to point to the next tile
        path.back().pNextTile = &m_EndTiles[0];
        // Add the next tile, and set it.
        PathTile& newTile = path.emplace_back();
        newTile.pCurrentTile = &m_EndTiles[0];
        m_Paths.push_back(path);

        // If any of our paths are next to the end tile, they should probably go straight to end and terminate.
        // If we didn't return here, we could move around the end tile before going into it.
        return;
    }

    const vector<Entity>& pathTiles = GetListOfTiles(TileOptions::TileType::Path);

    for (const Entity& pathTile : pathTiles) {
        const sf::Vector2i vPathTileCoords = pathTile.GetClosestGridCoordinates();

        if (DoesPathContainCoordinates(path, vPathTileCoords)) {
            continue; // Skip if the path already contains this tile
        }

        if (vPathTileCoords == vNorthCoords || vPathTileCoords == vEastCoords || vPathTileCoords == vSouthCoords || vPathTileCoords == vWestCoords) {
            // We have a neighbor tile
            Path newPath = path; // Create a copy of the current path
            newPath.back().pNextTile = &pathTile; // Set the next tile in the path
            PathTile& newTile = newPath.emplace_back();
            newTile.pCurrentTile = &pathTile;

            if (vPathTileCoords == rEndCoords) {
                // We reached the end tile
                m_Paths.push_back(newPath);
            }
            else {
                // Continue visiting neighbors
                VisitPathNeighbors(newPath, rEndCoords);
            }
        }
    }
}

bool Game::DoesPathContainCoordinates(const Path& path, const sf::Vector2i& coords) {
    for (const PathTile& tile : path) {
        if (tile.pCurrentTile->GetClosestGridCoordinates() == coords) {
            return true; // Found a tile with the same coordinates
        }
    }
    return false; // No tile with the same coordinates found
}

void Game::DrawLevelEditor() {
    sf::Vector2f vMousePosition = (sf::Vector2f)sf::Mouse::getPosition(m_Window);
    m_TileOptions[m_optionIndex].setPosition(vMousePosition);

    TileOptions::TileType eTileType = m_TileOptions[m_optionIndex].getTileType();

    if (m_bDrawPath) {
        for (const Entity& entity : m_SpawnTiles) {
            m_Window.draw(entity);
        }

        for (const Entity& entity : m_EndTiles) {
            m_Window.draw(entity);
        }

        for (const Entity& entity : m_PathTiles) {
            m_Window.draw(entity);
        }
    }
    m_Window.draw(m_TileOptions[m_optionIndex]);
}

void Game::HandlePlayInput() {
    if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
        sf::Vector2f vMousePosition = (sf::Vector2f)sf::Mouse::getPosition(m_Window);
        if (m_iPlayerGold >= 3) {
            if (CreateTowerAtPosition(vMousePosition)) {
                m_iPlayerGold -= 3;
            }
        }
    }
}

void Game::HandleLevelEditorInput() {

    if (m_eScrollWheelInput == ScrollUp) {
        m_optionIndex++;
        if (m_optionIndex >= m_TileOptions.size()) {
            m_optionIndex = 0;
        }
    }
    else if (m_eScrollWheelInput == ScrollDown) {
        m_optionIndex--;
        if (m_optionIndex < 0) {
            m_optionIndex = m_TileOptions.size() - 1;
        }
    }

    if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
        sf::Vector2f vMousePosition = (sf::Vector2f)sf::Mouse::getPosition(m_Window);
        CreateTileAtPosition(vMousePosition);
    }

    if (sf::Mouse::isButtonPressed(sf::Mouse::Right)) {
        sf::Vector2f vMousePosition = (sf::Vector2f)sf::Mouse::getPosition(m_Window);
        DeleteTileAtPosition(vMousePosition);
    }
}

vector<Entity>& Game::GetListOfTiles(TileOptions::TileType eTileType) {
    switch (eTileType) {
    case TileOptions::TileType::Aesthetic:
        return m_AestheticTiles;
    case TileOptions::TileType::Spawn:
        return m_SpawnTiles;
    case TileOptions::TileType::End:
        return m_EndTiles;
    case TileOptions::TileType::Path:
        return m_PathTiles;
    }
    return m_AestheticTiles; // Default return if no match found
}

bool Game::CreateTowerAtPosition(const sf::Vector2f& pos) {
    if (CanPlaceTowerAtPosition(pos)) {
        Entity newTower = m_TowerTemplate;
        newTower.SetColor(sf::Color::White);
        m_Towers.push_back(newTower);

        // Play tower placement sound
        SoundManager::getInstance().PlayTowerPlaceSound();

        return true;
    }
    return false;
}

bool Game::CanPlaceTowerAtPosition(const sf::Vector2f& pos) {
    sf::IntRect brickRect(0, 0, 16, 16);
    vector<Entity>& ListOfTiles = GetListOfTiles(TileOptions::TileType::Aesthetic);
    bool isOnBrick = false;
    Entity copyOfTowerWithRadiusOf1 = m_TowerTemplate;
    copyOfTowerWithRadiusOf1.setCirclePhysics(1.0f);

    for (const Entity& tile : ListOfTiles) {
        const sf::Sprite& rTileSprite = tile.GetSprite();
        sf::IntRect tileRect = rTileSprite.getTextureRect();

        if (tileRect != brickRect) {
            continue;
        }

        if (isColiding(tile, copyOfTowerWithRadiusOf1)) {
            isOnBrick = true;
            break;
        }
    }

    if (!isOnBrick) {
        return false;
    }

    for (const Entity& tower : m_Towers) {
        if (isColiding(tower, m_TowerTemplate)) {
            return false;
        }
    }
    return true;
}

void Game::AddGold(int gold) {
    m_iPlayerGold += gold;
    m_iGoldGainedThisUpdate += gold;
}

void Game::SetMusicVolume(float volume) {
    SoundManager::getInstance().SetMusicVolume(volume);
}

void Game::SetSoundVolume(float volume) {
    SoundManager::getInstance().SetSoundVolume(volume);
}