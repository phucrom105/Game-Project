#ifndef DAMAGETEXTMANAGER
#define DAMAGETEXTMANAGER

#include <SFML/Graphics.hpp>
#include <SFML/System/Time.hpp>;
#include <vector>
#include <deque>
#include <iostream>
#include <string>

namespace sf {
	class Text;
	class RenderTarget;
	class Time;
}

class DamageTextManager {
private:
	DamageTextManager();
	~DamageTextManager();
public:
	void Update(sf::Time& rDeltaTime);
	void Draw(sf::RenderTarget& rRenderTarget) const;

	void AddDamageText(int damage, const sf::Vector2f& pos);

	static const DamageTextManager& getInstanceConst() {
		return m_Instance;
	}

	static DamageTextManager& getInstanceNonConst() {
		return m_Instance;
	}
private:
	static DamageTextManager m_Instance;
	static float constexpr m_fDamageTextLifeInSeconds = 1.0f;

	struct DamageText {
		sf::Text m_Text;
		float m_fRemainingLifeSeconds;
	};
	sf::Font m_Font;
	std::deque<DamageText> m_DamageTextList;
};

#endif