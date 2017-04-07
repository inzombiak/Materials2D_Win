#define _USE_MATH_DEFINES
#include <cmath>
#include "LevelManager.h"
#include "IHero.h"
#include "../LuaWrapper/LUAWrapper.h"
#include "GameDefs.h"

LevelManager::LevelManager()
{
	m_hero = 0;

}


LevelManager::~LevelManager()
{
	Clear();
}

void LevelManager::LoadLevel(const std::string& level)
{
	if (m_hero)
	{
		delete m_hero;
		m_hero = 0;
	}
    MaterialManager::LoadAllMaterials();

	LuaWrapper::GetInstance().LoadScript("Level", level);

	m_columns = LuaWrapper::GetInstance().RunFunction<int>("Level", FuncNameInfo("GetColumns", "thisLevel"));
	m_rows = LuaWrapper::GetInstance().RunFunction<int>("Level", FuncNameInfo("GetRows", "thisLevel"));
    
    int heroY = LuaWrapper::GetInstance().RunFunction<int>("Level", FuncNameInfo("GetHeroY", "thisLevel"));
    int heroX = LuaWrapper::GetInstance().RunFunction<int>("Level", FuncNameInfo("GetHeroX", "thisLevel"));

	m_beatPause = LuaWrapper::GetInstance().RunFunction<double>("Level", FuncNameInfo("GetBeatPause", "thisLevel"));
	m_beatBuffer = LuaWrapper::GetInstance().RunFunction<double>("Level", FuncNameInfo("GetBeatBuffer", "thisLevel"));
	m_tmxPath = LuaWrapper::GetInstance().RunFunction<std::string>("Level", FuncNameInfo("GetTmxPath", "thisLevel"));

    LoadTmx();
    
    m_enemyManager.SetWorldBounds(std::pair<int, int>(m_columns, m_rows));
    m_enemyManager.SetLevelCollisionBodies(m_collisionBodies);
    m_enemyManager.Init("Level", "_enemies", "thisLevel");
    m_hero = new IHero(1, "Scripts/Heroes/Warrior Hero/WarriorHero.lua");

    m_hero->ApplyMaterial(MaterialManager::GetMaterialByType(MaterialType::Default));
    m_hero->SetPos(std::pair<int, int>(heroX, heroY));
    
	m_movementMarker.setRadius(12);
	m_movementMarker.setFillColor(sf::Color::Blue);
	m_markerHidden = false;
}

void LevelManager::UpdateRender()
{
	m_hero->UpdateRender();
	m_enemyManager.UpdateRender();
}

void LevelManager::Update()
{
    m_enemyManager.Update();
    if(!m_movementPath.empty())
    {
        std::pair<int, int> nextPoint = m_movementPath.top();
        m_hero->MoveToPoint(nextPoint);
        m_movementPath.pop();
        if(m_movementPath.empty())
			m_markerHidden = true;
        m_gridItems[nextPoint.second][nextPoint.first]->ApplyMaterial(m_hero->GetMaterial());
        LuaWrapper::GetInstance().RunFunction<void>("Level", FuncNameInfo("SetHeroX", "thisLevel"), nextPoint.first);
        LuaWrapper::GetInstance().RunFunction<void>("Level", FuncNameInfo("SetHeroY", "thisLevel"), nextPoint.second);
    }
    
	m_hero->Update();
    m_collisionBodies = m_enemyManager.GetCollisionData();
    LuaWrapper::GetInstance().RunFunction<void>("Level", FuncNameInfo("UpdateTriggers", "thisLevel"));
	ProcessUpdate();
}

void LevelManager::Draw(sf::RenderWindow& rw)
{
	for (int i = 0; i < m_rows; ++i)
		for (int j = 0; j < m_columns; ++j)
			m_gridItems[i][j]->Draw(rw);

	auto drawData = GetDrawingData();
	for (unsigned int i = 0; i < drawData.size(); ++i)
	{
		if (drawData[i].row < 0 || drawData[i].row >= m_rows || drawData[i].column < 0 || drawData[i].column >= m_columns)
			continue;

		//m_gridItems[drawData[i].row][drawData[i].column]->DrawCell(sf::Color(drawData[i].R, drawData[i].G, drawData[i].B), rw);
	}

	if (!m_markerHidden)
		rw.draw(m_movementMarker);

	m_hero->Draw(rw);
	m_enemyManager.Draw(rw);
}


void LevelManager::SetMaterial(MaterialType type)
{
    m_hero->ApplyMaterial(MaterialManager::GetMaterialByType(type));
};

void LevelManager::LoadTmx()
{
	m_xmlDoc.LoadFile(m_tmxPath.c_str());

	tinyxml2::XMLNode* pRoot = m_xmlDoc.FirstChildElement("map");
	tinyxml2::XMLElement* pMapParam = pRoot->ToElement();

	pMapParam->QueryAttribute("width", &m_columns);
	pMapParam->QueryAttribute("height", &m_rows);
	pMapParam->QueryAttribute("tileheight", &m_tileHeight);
	pMapParam->QueryAttribute("tilewidth", &m_tileWidth);
    
    m_collisionBodies.resize(m_rows, std::vector<int>(m_columns , -1));
    
	pMapParam = pRoot->FirstChildElement("tileset");
	tinyxml2::XMLElement* pTilesetParam = pMapParam->FirstChildElement("image");
	std::string tileSetPath = pTilesetParam->Attribute("source");
	pTilesetParam->QueryAttribute("width", &m_textureWidth);
	pTilesetParam->QueryAttribute("height", &m_textureHeight);

	tileSetPath.erase(0, 3);

	m_levelTileSet.loadFromFile(tileSetPath);
    
	auto size = m_levelTileSet.getSize();
	auto width = size.x;
    std::cout << width << std::endl;
	tinyxml2::XMLElement* pLayer = pRoot->FirstChildElement("layer");

    
	int worldIndex = 0;
	int tileIndex;
	m_gridItems.resize(m_rows);
	GridItem* temp;
	sf::Sprite gridSprite;
    int zPos = -2;
	int x, y;
    while(pLayer)
    {
        tinyxml2::XMLElement* pData = pLayer->FirstChildElement("data");
        tinyxml2::XMLElement* pTile = pData->FirstChildElement("tile");
        for (int i = 0; i < m_rows; ++i)
        {
            m_gridItems.push_back(std::vector<GridItem*>());
            m_gridItems[i].resize(m_columns);
            y = i*m_tileHeight;
            for (int j = 0; j < m_columns; ++j)
            {
                pTile->QueryAttribute("gid", &tileIndex);
                if(tileIndex == 0)
                {
                    pTile = pTile->NextSiblingElement("tile");
                    continue;

                }
                
                worldIndex++;
                x =  j * m_tileWidth;
                gridSprite = CreateGridSprite(tileIndex, worldIndex);
                temp = new GridItem(x, y, m_tileWidth, m_tileHeight, gridSprite);
                m_gridItems[i][j] = temp;
                pTile = pTile->NextSiblingElement("tile");

            }
        }
        zPos++;
        pLayer = pLayer->NextSiblingElement("layer");
    }
    tinyxml2::XMLElement* pCollRoot = pRoot->FirstChildElement("objectgroup");
    std::string collLayerName = "Collisions";
    if(collLayerName.compare(pCollRoot->Attribute("name")) == 0)
        CreateCollisionBodies(pCollRoot);
}

std::pair<int, int> LevelManager::MouseToGrid(int x, int y)
{
    std::pair<int, int> result;
    
    result.first = x/m_tileWidth;
    result.second = y / m_tileHeight;
    
    return result;
}

void LevelManager::CreateCollisionBodies(tinyxml2::XMLElement *pCollisionRoot)
{
    tinyxml2::XMLElement* pObj = pCollisionRoot->FirstChildElement("object");
    int column, row, firstRow, x, y, width, height, numTilesSpannedX, numTilesSpannedY;
    sf::Sprite spr;
    sf::IntRect rect;
    
    while(pObj)
    {
        pObj->QueryAttribute("x", &x);
        pObj->QueryAttribute("y", &y);
        pObj->QueryAttribute("width", &width);
        pObj->QueryAttribute("height", &height);
        numTilesSpannedX = width/m_tileWidth;
        numTilesSpannedY = height/m_tileHeight;
        //std::cout <<"ID: " << pObj->Attribute("id") << std::endl;
        firstRow = row = y/m_tileHeight;
        column = x/m_tileWidth;
        
        for(unsigned int i = 0; i < numTilesSpannedY; ++i)
        {
            column = x/m_tileWidth;
            for(unsigned int j = 0;  j < numTilesSpannedX; ++j)
            {
                m_collisionBodies[row][column] = 0;
               column++;
            }
            row++;
            
        }
		rect.left = x - m_tileHeight / 2;
		rect.top = y + height + m_tileWidth / 2;
		rect.width = width;
		rect.height = height;
        
		spr.setTexture(m_levelTileSet);
		spr.setTextureRect(rect);
       // m_collisionDebugSprites.push_back(spr);
        
        pObj = pObj->NextSiblingElement();
    }
    
}

sf::Sprite LevelManager::CreateGridSprite(int tileIndex, int worldIndex)
{
//    double margin = (double)m_tileWidth/(double)m_textureWidth;
//    int columns = m_textureWidth / m_tileWidth;
//    int rows = m_textureHeight / m_tileHeight;
//    int textureI = (int)((tileIndex - 1) / columns);
//    textureI = columns - textureI - 1;
//    int textureJ = (int)((tileIndex-1) % rows);
//   
//    double ty = textureI * margin;
//    double tx = textureJ * margin;
//    double tx2 = margin;
//    double ty2 = margin;

	sf::Sprite sprite;
	int textureI = (int)std::floor((tileIndex - 1) / (m_textureWidth / m_tileWidth));
	int textureJ = (int)(tileIndex - 1) % (m_textureHeight / m_tileHeight);

	sf::IntRect subRect;
	int ty = textureJ*m_tileHeight;
	int tx = textureI*m_tileWidth;
	subRect.top = tx;
	subRect.left = ty;
	subRect.width = m_tileWidth;
	subRect.height = m_tileHeight;

	sprite.setTexture(m_levelTileSet);
	sprite.setTextureRect(subRect);
	return sprite;
}

void LevelManager::Clear()
{
    
	for (unsigned int i = 0; i < m_gridItems.size(); ++i)
	{
		for (unsigned int j = 0; j < m_gridItems[i].size(); ++j)
		{
			delete m_gridItems[i][j];
			m_gridItems[i][j] = 0;
		}
		m_gridItems[i].clear();
	}
    
    m_collisionBodies.clear();
	m_gridItems.clear();
	delete m_hero;
	m_hero = 0;
	m_enemyManager.Clear();
	LuaWrapper::GetInstance().Clear();
}

void LevelManager::ProcessUpdate()
{
    /*for(unsigned int i = 0; i < m_collisionDebugSprites.size(); ++i)
    {
        [m_collisionDebugSprites[i] runAction:[SKAction removeFromParent]];
    };*/
    
   /* m_collisionDebugSprites.clear();
*/
	CellData cellD;
	auto enemyD = m_enemyManager.GetEnemyData();
	auto heroPos = m_hero->GetPos();

    sf::Sprite spr;
    sf::IntRect rect;
    rect.width = m_tileWidth;
    rect.height = m_tileHeight;
    
    
	/*for (unsigned int j = 0; j < m_collisionBodies.size(); ++j)
	{
        for (unsigned int k = 0; k < m_collisionBodies[j].size(); ++k)
        {
            if(m_collisionBodies[j][k] == -1)
                continue;
            rect.origin.x = m_tempOffsetX + k * m_tileWidth - m_tileHeight/2;
            rect.origin.y = m_tempOffsetY - j* m_tileHeight - m_tileWidth/2;
            spr.fillColor = [UIColor colorWithRed:0 green:0 blue:255 alpha:.5];
            spr.zPosition = 10;
            spr = [SKShapeNode shapeNodeWithRect:rect];
            [m_scene addChild:spr];
            m_collisionDebugSprites.push_back(spr);
        }
	}*/

	for (unsigned int i = 0; i < enemyD.size(); ++i)
	{
        cellD.row = ceil(enemyD[i].pos.second);
        cellD.column = ceil(enemyD[i].pos.first);
        
        if (cellD.column < 0 || cellD.column >= m_columns || cellD.row < 0 || cellD.row >= m_rows)
            continue;
        
        
        if (m_gridItems[cellD.row][cellD.column])
        {
             if(m_gridItems[cellD.row][cellD.column]->HasMaterial())
             {
                 auto mat = m_gridItems[cellD.row][cellD.column]->GetMaterial();
                 mat->OnCollide(m_enemyManager.GetEnemyByIndex(enemyD[i].index).GetLuaObj());
             }

        }
		m_drawData.push_back(cellD);

		for (unsigned int k = 0; k < enemyD[i].pattern.size(); ++k)
		{
			cellD.row = enemyD[i].pos.second + enemyD[i].pattern[k].second;
			cellD.column = enemyD[i].pos.first + enemyD[i].pattern[k].first;
			cellD.R = 255;
			cellD.G = 0;
			cellD.B = 0;
			m_drawData.push_back(cellD);
		}
       
        
	}
    
    
    
    
	m_playerAttackInfo.damage = 0;
}

void LevelManager::UseAbility(bool isTap, double angle, bool sglScreenControls)
{
	if (isTap)
		m_playerAttackInfo = m_hero->Tap(angle, sglScreenControls);
	else
		m_playerAttackInfo = m_hero->DoubleTap(angle, sglScreenControls);
}

void LevelManager::RotateHero(double angle) const
{
	m_hero->Rotate(angle);
}

void LevelManager::MoveHero(double direction) const
{
	auto oldPos = m_hero->GetPos();
	auto newPos = m_hero->Move(direction);

	if (newPos.first < 0 || newPos.first >= m_columns || newPos.second <= 0 || newPos.second > m_rows || m_collisionBodies[newPos.first][newPos.second])
	{
		m_hero->SetPos(oldPos);
		return;
	}
}

void LevelManager::MoveToPoint(int x, int y)
{	
    auto convertMouseToGrid = MouseToGrid(x, y);
    int gridCol = convertMouseToGrid.first;
    int gridRow = convertMouseToGrid.second;
    if(gridCol < 0 || gridRow < 0 || gridCol >= m_columns || gridRow >= m_rows)
        return;
    
    GridPoint start, end;
    
    start.x = m_hero->GetPos().first;
    start.y = m_hero->GetPos().second;
    start.SetID(m_columns);
    
    end.x = gridCol;
    end.y = gridRow;
    end.SetID(m_columns);
    PerformPathfinding(start, end);
    
    m_movementMarker.setPosition(gridCol * m_tileWidth, gridRow * m_tileHeight);
    m_markerHidden = false;
}

void LevelManager::PerformPathfinding(const GridPoint& start, const GridPoint& end)
{
    std::stack<std::pair<int, int>> empty;
    std::swap(m_movementPath, empty );
    
    if(start.id < 0 || end.id < 0)
        return;
    
    //m_doingPathfinding = true;
    std::priority_queue<GridPoint> frontier;
    std::map<int, GridPoint> cameFrom;
    std::vector<GridPoint> neighbors;
    std::map<int, GridPoint> costSoFar;
    GridPoint current;
    int currId;
    std::map<int, GridPoint>::iterator costMapIt;
    
    
    cameFrom[start.id] = start;
    frontier.push(start);
    costSoFar[start.id] = start;;
    while(!frontier.empty())
    {
        current = frontier.top();
        frontier.pop();
        if(current == end)
            break;
        
        neighbors = GetNeighbors(current);
        
        for(unsigned int i = 0; i < neighbors.size(); ++i)
        {
            currId = neighbors[i].id;
            costMapIt = costSoFar.find(currId);
            if(costMapIt == costSoFar.end() || neighbors[i].weight < costMapIt->second.weight)
            {
                Heuristic(neighbors[i], end);
                frontier.push(neighbors[i]);
                cameFrom[currId] = current;
                costSoFar[currId] = neighbors[i];
            }
        }
    }
    current = end;
    do
    {
        m_movementPath.push(std::pair<int, int>(current.x, current.y));
        
        current = cameFrom[current.id];
        if(current.id < 0)
        {
            std::swap(m_movementPath, empty );
            break;
        }
        
    }while (current != start);
}

std::vector<GridPoint> LevelManager::GetNeighbors(const GridPoint& point)
{
    std::vector<GridPoint> result;
    result.reserve(4);
    
    for(unsigned int i = 0; i < 4; ++i)
    {
        GridPoint neighborPoint;
        neighborPoint.x = point.x + cos(i * M_PI_2);
        neighborPoint.y = point.y + sin(i * M_PI_2);
        if(neighborPoint.x < 0 || neighborPoint.y < 0 || neighborPoint.x >= m_columns || neighborPoint.y >= m_rows || m_collisionBodies[neighborPoint.y][neighborPoint.x] != -1)
            continue;
        
        neighborPoint.SetID(m_columns);
        neighborPoint.weight = point.weight + 1;
        result.push_back(neighborPoint);
    }
    
    return result;
}

void LevelManager::Heuristic(GridPoint& point, const GridPoint& target)
{
    point.priority = abs(target.x - point.x) + abs(target.y - point.y);
}