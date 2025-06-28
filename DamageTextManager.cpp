#include "DamageTextManager.h"

DamageTextManager DamageTextManager::m_Instance;

DamageTextManager::DamageTextManager() {
	m_Font.loadFromFile("Fonts/Kreon-Medium.ttf");
}

DamageTextManager::~DamageTextManager() {

}

void DamageTextManager::Update(sf::Time& rDeltaTime) {
	const int iCount = m_DamageTextList.size();
	for (int i = 0; i < iCount; i++) {
		DamageText damageText = m_DamageTextList.front();
		m_DamageTextList.pop_front();
		damageText.m_fRemainingLifeSeconds -= rDeltaTime.asSeconds();

		if (damageText.m_fRemainingLifeSeconds > 0.0f) {
			//Fade out the damage text over time
			float fPercentageThroughLife = damageText.m_fRemainingLifeSeconds / m_fDamageTextLifeInSeconds;

			sf::Color color = damageText.m_Text.getFillColor();
			color.a = static_cast <sf::Uint8> (255.0f * fPercentageThroughLife);

			sf::Color OutlineColor = damageText.m_Text.getOutlineColor();
			OutlineColor.a = static_cast <sf::Uint8> (255.0f * fPercentageThroughLife);

			damageText.m_Text.setFillColor(color);
			damageText.m_Text.setOutlineColor(OutlineColor);

			m_DamageTextList.push_back(damageText);
		}
	}
}

void DamageTextManager::Draw(sf::RenderTarget& rRenderTarget) const {
	for (const DamageText& damageText : m_DamageTextList) {
		rRenderTarget.draw(damageText.m_Text);
	}
}

void DamageTextManager::AddDamageText(int damage, const sf::Vector2f& pos) {
	sf::Text text;
	text.setFont(m_Font);
	text.setString(std::to_string(damage));
	text.setCharacterSize(36);
	text.setOutlineColor(sf::Color::Black);
	text.setOutlineThickness(2.0f);
	text.setPosition(pos);
	text.setOrigin(text.getLocalBounds().width / 2.0f, text.getLocalBounds().height / 2.0f);

	DamageText damageText;
	damageText.m_Text = text;
	damageText.m_fRemainingLifeSeconds = m_fDamageTextLifeInSeconds;

	m_DamageTextList.push_back(damageText);
}