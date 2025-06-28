#pragma once
#include <SFML/Graphics.hpp>
#include "Entity.h"
#include "TileOptions.h"
#include <vector>
#include <string>
#include <iostream>
using namespace std;

class Game {
public:
	Game();
	~Game();

	enum GameMode {
		Play,
		LevelEditor
	};

	//Scroll wheel values
	enum ScrollWheel {
		ScrollUp,
		ScrollDown,
		None
	};

	struct PathTile {
		const Entity* pCurrentTile;
		const Entity* pNextTile;
	};

	void run();
private:
	void UpdatePlay();
	void UpdateTower();
	void UpdateAxe();
	void CheckForDeletionRequest();
	void UpdateLevelEditor();

	void UpdatePhysics();
private:
	void ProcessCollision(Entity& entity1, Entity& entity2);
	bool isColiding(const Entity& entity1, const Entity& entity2);
public:
	void Draw();
	void DrawPlay();
	void DrawLevelEditor();

	void HandlePlayInput();
	void HandleLevelEditorInput();
	void HandleInput();

	//Level Editor functions
	void CreateTileAtPosition(const sf::Vector2f& pos);
	void DeleteTileAtPosition(const sf::Vector2f& pos);
	void ConstructionPath();
	vector<Entity>& GetListOfTiles(TileOptions::TileType eTileType);

	// Play functions
	bool CreateTowerAtPosition(const sf::Vector2f& pos);
	bool CanPlaceTowerAtPosition(const sf::Vector2f& pos);

	void AddGold(int gold);
private:
	sf::RenderWindow m_Window;
	sf::Time m_deltaTime;
	GameMode m_eGameMode;

	//Play mode
	sf::Texture towerTexture;
	sf::Texture enemyTexture;
	sf::Texture axeTexture;

	Entity m_TowerTemplate;
	vector <Entity> m_Towers;

	Entity m_enemyTemplate;
	vector<Entity> m_enemies;

	Entity m_axeTemplate;
	vector<Entity> m_axes;

	//vector <Entity*> m_AllEntities;

	sf::Text m_GameModeText;
	sf::Font m_Font;
	sf::Text m_PlayerText;
	sf::Text m_GameOverText;

	//Level Editor Mode
	int m_optionIndex;
	ScrollWheel m_eScrollWheelInput;

	sf::Texture m_TileMapTexture;
	// TODO: these need to be entities, not sprites
	vector <TileOptions> m_TileOptions;
	vector <Entity> m_AestheticTiles;
	vector <Entity> m_SpawnTiles;
	vector <Entity> m_EndTiles;
	vector <Entity> m_PathTiles;

	bool m_bDrawPath;

	//GamePlay variables
	int m_iPlayerHealth;
	int m_iPlayerGold;
	int m_iGoldGainedThisUpdate;
	float m_fTimeInPlayMode;
	float m_fDifficulty;
	float m_fGoldPerSecond;
	float m_fGoldPerSecondTimer;
private:
	//PathFinding
	typedef vector<PathTile> Path;

	void VisitPathNeighbors(Path path, const sf::Vector2i& rEndCoords);
	bool DoesPathContainCoordinates(const Path& path, const sf::Vector2i& coordinates);

	vector<Path> m_Paths;
};