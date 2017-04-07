#pragma once

#include "AnimationData.h"
#include <SFML\Graphics.hpp>
#include <vector>
#include <memory>

struct AbilityInfo
{
	int damage;
	std::vector<std::pair<int, int>> pattern;
};

class IMaterial;
class IAbility;
class IHero
{
public:
    IHero(unsigned int id, std::string filePath);
	~IHero();

	AbilityInfo Tap(double angle, bool useOwnDirection);
	AbilityInfo DoubleTap(double angle, bool useOwnDirection);
	std::pair<int, int> GetCooldowns() const;
	std::pair<int, int> GetBeatsPassed() const;
	void Update();
	void UpdateRender();
	void Draw(sf::RenderWindow& rw);
	void Rotate(double angle);
	double GetDirection()
	{
		return m_direction;
	}
	void SetPos(const std::pair<int, int>& newPos)
	{
		m_pos = newPos;
       
		m_sprite.setPosition((float)m_pos.first * 32, (float)m_pos.second * 32);
	}

	std::pair<int, int> Move(double direction);
    std::pair<int, int> MoveToPoint(const std::pair<int, int>& newPos);
	const std::pair<int, int>& GetPos() const
	{
		return m_pos;
	}
    
    void ApplyMaterial(std::shared_ptr<IMaterial> newMat)
    {
        if(!newMat)
            return;
        m_material = newMat;
    }
    const std::shared_ptr<IMaterial> GetMaterial() const
    {
        return m_material;
    }

protected:
	IAbility* m_sglClickAb;
	IAbility* m_dblClickAb;

	void LoadSpriteSheet();
	void LoadAnimationFile();
	void UpdateAnimation();

	double m_direction = 0;
	unsigned int m_id;
	std::pair<int, int> m_pos;
	std::string m_filePath;
    
	sf::Texture m_spriteSheet;
	sf::IntRect m_textureRect;
	sf::Sprite m_sprite;

    int m_tileWidth = 32;
    int m_tileHeight = 32;
	int m_counter = 0; //Number of times Update has bee called
	const int m_maxCounter = 4; //Animations moves forward by a a frame when counters reaches this number

	Animation::Animation m_currentAnimation; //Current naimation being played
	std::map<std::string, std::string> m_messageToAnimation; //Mapping for message to animation name
	std::map<std::string, Animation::SpriteDefinition> m_spriteDefinitions; //Contains texture bounds for each sprite
	std::map<std::string, Animation::Animation> m_animations; //Contains animaitons
    
    std::shared_ptr<IMaterial> m_material;
};

