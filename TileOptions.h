#ifndef TILEOPTIONS
#define TILEOPTIONS

#include <SFML/Graphics.hpp>

class TileOptions : public sf::Drawable
{
public:
	enum TileType {
		Null = -1,
		Aesthetic, //0
		Spawn, //1
		End, //2
		Path, //3
		NumTileTypes //4
	};

	TileOptions(TileType m_tileType);

	void setSprite(const sf::Sprite& sprite) { m_sprite = sprite; }
	const sf::Sprite& getSprite() const { return m_sprite; }
	void setPosition(const sf::Vector2f& position) { m_sprite.setPosition(position); }
	TileType getTileType() const { return m_tileType; }

	void draw(sf::RenderTarget& target, sf::RenderStates states) const override {
		target.draw(m_sprite, states);
	}
private:
	sf::Sprite m_sprite;
	TileType m_tileType;
};
#endif // !TILEOPTIONS