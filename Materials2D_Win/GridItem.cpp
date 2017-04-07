#include "GridItem.h"
#include "GameDefs.h"

GridItem::GridItem(int x, int y, int w, int h, sf::Sprite sprite) : m_sprite(sprite)
{
	m_sprite.setPosition((float)x, (float)y);
}

GridItem::~GridItem()
{
    
}

void GridItem::Draw(sf::RenderWindow& rw)
{
	rw.draw(m_sprite);
	if (!m_material)
		return;

	rw.draw(m_materialSprite);
}

bool GridItem::HasMaterial() const
{
    if(!m_material)
        return false;
    
    return m_material->GetType() != MaterialType::Default;
}

void GridItem::ApplyMaterial(std::shared_ptr<IMaterial> newMat)
{
    m_material = newMat;
    std::string color = m_material->GetColor();
    
    if(color.empty())
        return;
    
	m_materialSprite.setSize(sf::Vector2f(32, 32));
	m_materialSprite.setPosition(m_sprite.getPosition());
	sf::Color fillColor;
	
    if(color.compare("Red") == 0)
    {
		fillColor = sf::Color::Red;
		fillColor.a = 100;
    }
    else if(color.compare("Blue") == 0)
    {
		fillColor = sf::Color::Blue;
		fillColor.a = 100;
    }
    else if(color.compare("None") == 0)
    {
		fillColor = sf::Color::Green;
		fillColor.a = 0;
    }

	
	m_materialSprite.setFillColor(fillColor);
}

