#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
using namespace std;
#ifndef ENTITY_H	
#define ENTITY_H

class Entity : public sf::Drawable
{
public:
	struct PhysicsData {
		PhysicsData() {
			m_vImpulse = sf::Vector2f(0.0f, 0.0f);
		}

		enum Layer {
			Enemy = 1, //0b0001
			Tower = 2, //0b0010
			Projectile = 4 // 0b0100
		};
		enum class Shape {
			Circle,
			Rectangle
		};

		enum class Type {
			Static,
			Dynamic
		};
		Shape m_eShape;
		Type m_eType;

		void setLayers(int layers) {
			m_iMyLayer = layers;
		}

		void setLayersToIgnore(int layers) {
			m_iLayersToIgnore = layers;
		}

		int getLayersToIgnore() const {
			return m_iLayersToIgnore;
		}

		bool IsInAnyLayer(int layer) const {
			return (m_iMyLayer & layer) != 0;
		}

		void ClearCollisions() {
			m_EntitiesThatCollidedWithAlready.clear();
		}

		bool HasCollidedThisUpdate(Entity* pOtherEntity) const {
			for (Entity* pEntity : m_EntitiesThatCollidedWithAlready) {
				if (pEntity == pOtherEntity) {
					return true;
				}
			}
			return false;
		}

		void AddEntityCollision(Entity* pOtherEntity) {
			m_EntitiesThatCollidedWithAlready.push_back(pOtherEntity);
		}

		void ClearImpulse() {
			m_vImpulse = sf::Vector2f(0.0f, 0.0f);
		}

		void AddImpulse(const sf::Vector2f& impulse) {
			m_vImpulse += impulse;
		}

		int m_iMyLayer;
		int m_iLayersToIgnore;

		float m_fRadius; // For Circle shape
		float m_fWidth; // For Rectangle shape
		float m_fHeight; // For Rectangle shape

		sf::Vector2f m_vVelocity;
		sf::Vector2f m_vImpulse;

		vector<Entity*> m_IgnoredEntities; // Entities to ignore for collision
		vector<Entity*> m_EntitiesToIgnore;
		vector<Entity*> m_EntitiesThatCollidedWithAlready;
	};

	Entity(PhysicsData::Type ePhysicsType);
	~Entity() {};

	void setCirclePhysics(float radius) {
		m_PhysicsData.m_eShape = PhysicsData::Shape::Circle;
		m_PhysicsData.m_fRadius = radius;
	}

	void setRectanglePhysics(float width, float height) {
		m_PhysicsData.m_eShape = PhysicsData::Shape::Rectangle;
		m_PhysicsData.m_fWidth = width;
		m_PhysicsData.m_fHeight = height;
	}

	void addIgnoredEntity(Entity* entity) {
		m_PhysicsData.m_IgnoredEntities.push_back(entity);
	}

	bool shouldIgnoreEntityForPhysics(Entity* entity) const {
		for (const auto& ignoredEntity : m_PhysicsData.m_IgnoredEntities) {
			if (ignoredEntity == entity) {
				return true;
			}
		}

		if (entity->GetPhysicsData().IsInAnyLayer(m_PhysicsData.getLayersToIgnore())) {
			return true;
		}
		return false;
	}

	void SetVelocity(const sf::Vector2f& velocity) {
		m_PhysicsData.m_vVelocity = velocity;
	}

	void SetTexture(const sf::Texture& texture) {
		m_Sprite.setTexture(texture);
	}

	void SetScale(const sf::Vector2f& scale) {
		m_Sprite.setScale(scale);
	}

	void SetOrigin(const sf::Vector2f& origin) {
		m_Sprite.setOrigin(origin);
	}

	void SetPosition(const sf::Vector2f& position) {
		m_Sprite.setPosition(position);
	}

	void SetColor(const sf::Color& color) {
		m_Sprite.setColor(color);
	}

	void SetSprite(const sf::Sprite& sprite) {
		m_Sprite = sprite;
	}

	const sf::Sprite& GetSprite() const {
		return m_Sprite;
	}

	sf::Sprite& GetSpriteNonConst() {
		return m_Sprite;
	}

	void move(const sf::Vector2f& offset) {
		m_Sprite.move(offset);
	}

	void draw(sf::RenderTarget& target, sf::RenderStates states) const override {
		target.draw(m_Sprite, states);
	}

	sf::Vector2f GetPosition() const {
		return m_Sprite.getPosition();
	}

	sf::Vector2i GetClosestGridCoordinates() const {
		return sf::Vector2i(GetPosition().x / 160, GetPosition().y / 160);
	}

	PhysicsData::Type GetPhysicsShapeType() const {
		return m_PhysicsData.m_eType;
	}

	const PhysicsData& GetPhysicsData() const {
		return m_PhysicsData;
	}

	PhysicsData& GetPhysicsDataNonConst() {
		return m_PhysicsData;
	}

	void SetPathIndex(int index) {
		m_iPathIndex = index;
	}

	int GetPathIndex() const {
		return m_iPathIndex;
	}

	void OnCollision(Entity& pOtherEntity);

	void SetHealth(int health) {
		m_iHealth = health;
	}

	void DealDamage(int damage);

	bool IsDeletionRequested() const {
		return m_bDeletionRequested;
	}

	int getHealth() const {
		return m_iHealth;
	}

	void RequestDeletion() {
		m_bDeletionRequested = true;
	}

private:
	sf::Sprite m_Sprite;
	PhysicsData m_PhysicsData;
	bool m_bDeletionRequested;

	int m_iPathIndex;
	int m_iHealth;
public:
	float m_fAxeTimer;
	float m_fAttackTimer;
};

#endif; 