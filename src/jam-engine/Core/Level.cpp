#include "jam-engine/Core/Level.hpp"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstring>
#include <algorithm>
#include "jam-engine/Core/Game.hpp"
#include "jam-engine/Graphics/TexManager.hpp"
#include "jam-engine/Utility/Math.hpp"

namespace je
{

Level::Level(Game * const game, int width, int height)
	:width(width)
	,height(height)
	,game(game)
	,states (sf::RenderStates::Default)
{
	this->init();
}

Level::Level(Game * const game)
	:width(0)
	,height(0)
	,game(game)
	,states (sf::RenderStates::Default)
{
	this->init();
}

Level::~Level()
{
	this->clear();
}

void Level::draw(sf::RenderTarget& target) const
{
	beforeDraw(target);
	for (Entity *entity : depthBuffer)
	{
		{
			entity->draw(target, states);
		}
	}
	onDraw(target);

#ifdef JE_DEBUG
	for (auto& p : entities)
	{
		for (Entity * entity : p.second)
			if (cameraBounds.contains(entity->getPos().x + cameraBounds.width / 2, entity->getPos().y + cameraBounds.height / 2))
				entity->debugDraw(target);
		for (const sf::RectangleShape& rect : debugDrawRects)
			target.draw(rect);
	}
#endif
	/*std::cout << "tileSprites.size() = " << tileSprites.size() << "\n";
	for (int i = 0; i < tileSprites.size(); ++i)
	{
		sf::Sprite& s = tileSprites[i];
		//std::cout << "(" << s.getGlobalBounds().width << "," << s.getGlobalBounds().height << ")";
		tileSprites[i].setPosition(sf::Vector2f(i * 16, 0));
		target.draw(tileSprites[i]);
	}*/
}

void Level::update()
{
#ifdef JE_DEBUG
	debugDrawRects.clear();
#endif
	for (const std::string& type : specificOrderEntitiesPre)
	{
		auto& entityList = entities[type];
		for (unsigned int i = 0; i < entityList.size(); )
		{
			entityList[i]->update();
			if (entityList[i]->isDead())
			{
				delete entityList[i];
				entityList[i] = entityList.back();
				entityList.pop_back();
			}
			else
				++i;
		}
	}
	for (auto& p : entities)
	{
		auto it = hasSpecificUpdateOrder.find(p.first);
		if (it != hasSpecificUpdateOrder.end() && it->second == true)
			continue;
		for (unsigned int i = 0; i < p.second.size(); )
		{
			p.second[i]->update();
			if (p.second[i]->isDead())
			{
				delete p.second[i];
				p.second[i] = p.second.back();
				p.second.pop_back();
			}
			else
				++i;
		}
	}
	for (const std::string& type : specificOrderEntitiesPost)
	{
		auto& entityList = entities[type];
		for (unsigned int i = 0; i < entityList.size(); )
		{
			entityList[i]->update();
			if (entityList[i]->isDead())
			{
				delete entityList[i];
				entityList[i] = entityList.back();
				entityList.pop_back();
			}
			else
				++i;
		}
	}
	onUpdate();
	for (auto& grid : tileLayers)
	{
		sf::Rect<int> bounds(cameraBounds);
		bounds.left -= cameraBounds.width / 2;
		bounds.top -= cameraBounds.height / 2;
		grid.second->setVisibleArea(bounds);
	}
	//	depth sort
	depthBuffer.clear();
	for (auto& p : entities)
		for (Entity *entity : p.second)
			depthBuffer.push_back(entity);
	std::sort(depthBuffer.begin(), depthBuffer.end(), [](const Entity *a, const Entity *b) -> bool {
		return a->getDepth() == b->getDepth() ? (int) a > (int) b : a->getDepth() > b->getDepth();
	});
}

Entity* Level::testCollision(const sf::Rect<int>& bBox, Entity::Type type)
{
	Entity *retVal = nullptr;
	auto mit = entities.find(type);
	if (mit != entities.end())
	{
		for (Entity *entity : mit->second)
		{
			if (entity->getType() == type && entity->intersects(bBox))
			{
				retVal = entity;
				break;
			}
		}
	}
	this->debugDrawRect(bBox, !retVal ? sf::Color::Yellow : sf::Color::Green);
	return retVal;
}

Entity* Level::testCollision(const Entity *caller, Entity::Type type, float xoffset, float yoffset)
{
	Entity *retVal = nullptr;
	auto mit = entities.find(type);
	if (mit != entities.end())
	{
		for (Entity *entity : mit->second)
		{
			if (entity != caller && entity->getType() == type && caller->intersects(*entity, xoffset, yoffset))
			{
				retVal = entity;
				break;
			}
		}
	}
	sf::Rect<int> rect = caller->getBounds();
	rect.left += xoffset;
	rect.top += yoffset;
	this->debugDrawRect(rect, !retVal ? sf::Color::Yellow : sf::Color::Green);
	return retVal;
}

void Level::findCollisions(std::vector<Entity*>& results, const Entity *caller, Entity::Type type, float xoffset, float yoffset)
{
	//	TODO: ADD CULLING
	results.clear();
	auto mit = entities.find(type);
	if (mit != entities.end())
	{
		for (Entity *entity : mit->second)
		{
			if (entity != caller && entity->getType() == type && caller->intersects(*entity, xoffset, yoffset))
				results.push_back(entity);
		}
	}
	sf::Rect<int> rect = caller->getBounds();
	rect.left += xoffset;
	rect.top += yoffset;
	this->debugDrawRect(rect, results.empty() ? sf::Color::Yellow : sf::Color::Green);
}

void Level::addEntity(Entity *instance)
{
	entities[instance->getType()].push_back(instance);
}

void Level::clear()
{
	for (auto& p : entities)
	{
		for (Entity *entity : p.second)
			delete entity;
		p.second.clear();
	}
	tileLayers.clear();
	tileSprites.clear();
}

void Level::clearEntities()
{
	for (auto& p : entities)
	{
		if (p.first == "TileGrid")
		{
			bool isTile = false;
			for (unsigned int i = 0; i < p.second.size(); ++i)
			{
				isTile = false;
				for (auto& it : tileLayers)
				{
					if (p.second[i] == it.second)
					{
						isTile = true;
						break;
					}
				}
				if (!isTile)
				{
					delete p.second[i];
					p.second[i] = p.second.back();
					p.second.pop_back();
					--i;
				}
			}
		}
		else
		{
			for (Entity *entity : p.second)
				delete entity;
			p.second.clear();
		}
	}
}

int Level::getWidth() const
{
	return width;
}

int Level::getHeight() const
{
	return height;
}

Game& Level::getGame() const
{
	return *game;
}

sf::Rect<int> Level::getCameraBounds() const
{
	return sf::Rect<int>(cameraBounds.left - cameraBounds.width / 2, cameraBounds.top - cameraBounds.height / 2, cameraBounds.width, cameraBounds.height);
}

void Level::setCameraBounds(const sf::Rect<int>& newBounds)
{
	cameraBounds = newBounds;
}

sf::Vector2f Level::getCameraPosition() const
{
	return sf::Vector2f(cameraBounds.left, cameraBounds.top);
}

void Level::setCameraPosition(const sf::Vector2f& cameraPosition)
{
	cameraBounds.left = cameraPosition.x;
	cameraBounds.top = cameraPosition.y;
	this->limitCamera();
}

void Level::moveCamera(const sf::Vector2f& cameraPosition)
{
	cameraBounds.left += cameraPosition.x;
	cameraBounds.top += cameraPosition.y;
	this->limitCamera();
}

sf::Vector2f Level::getCursorPos() const
{
	sf::Vector2i posI = game->getWindow().mapCoordsToPixel(sf::Vector2f(sf::Mouse::getPosition().x, sf::Mouse::getPosition().y)) - game->getWindow().getPosition();
	return sf::Vector2f(posI.x + cameraBounds.left - cameraBounds.width / 2, posI.y + cameraBounds.top - cameraBounds.height / 2);
}

void Level::loadMap(const std::string& filename)
{
	using namespace rapidxml;
	std::ifstream mapFile (filename);
	std::string line;
	std::stringstream ss;
	std::cout << filename << "\n";

	if (mapFile.is_open())
	{
		while (mapFile.good())
		{
			getline (mapFile, line);
			ss << line << "\n";
		}

		std::string s = ss.str();
		std::vector<char> text(s.begin(), s.end());
		text.push_back('\0');

	   	xml_document<> doc;
		doc.parse<0> (text.data());

		xml_node<> *root = doc.first_node("map");
		if (root)
		{
			xml_attribute<> *attr = root->first_attribute("version");
			std::string version = attr->value();
			std::cout << attr->value() << "\n";
			attr = attr->next_attribute("orientation");
			std::cout << attr->value() << "\n";
			std::string orientation = attr->value();
			attr = attr->next_attribute("width");
			std::cout << attr->value() << "\n";
			int mapWidth = atoi(attr->value());
			attr = attr->next_attribute("height");
			std::cout << attr->value() << "\n";
			int mapHeight = atoi(attr->value());
			attr = attr->next_attribute("tilewidth");
			std::cout << attr->value() << "\n";
			int tileWidth = atoi(attr->value());
			attr = attr->next_attribute("tileheight");
			std::cout << attr->value() << "\n";
			int tileHeight = atoi(attr->value());

			width = mapWidth * tileWidth;
			height = mapHeight * tileHeight;

			for (xml_node<> *tileset = root->first_node("tileset"); tileset; tileset = tileset->next_sibling("tileset"))
			{
				xml_attribute<> *attr = tileset->first_attribute("firstgid");
				std::cout << attr->value() << "\n";
				int firstgrid = atoi(attr->value());
				attr = attr->next_attribute("name");
				std::cout << attr->value() << "\n";
				std::string tileSetName = attr->value();
				attr = attr->next_attribute("tilewidth");
				std::cout << attr->value() << "\n";
				int tileSet_tileWidth = atoi(attr->value());
				attr = attr->next_attribute("tileheight");
				std::cout << attr->value() << "\n";
				int tileSet_tileHeight = atoi(attr->value());
				std::cout << "tileSetH: " << tileSet_tileHeight << "  tileSetW: " << tileSet_tileWidth << "\n";


				xml_node<> *img = tileset->first_node ("image");
				attr = img->first_attribute ("source");
				std::cout << attr->value() << "\n";
				std::string source = attr->value();
				attr = attr->next_attribute ("width");
				std::cout << attr->value() << "\n";
				int imgWidth = atoi(attr->value());
				attr = attr->next_attribute ("height");
				std::cout << attr->value() << "\n";
				int imgHeight = atoi(attr->value());
				createTiles(source, tileSet_tileHeight, tileSet_tileWidth, height, width);
			}

			for (xml_node<> *layer = root->first_node("layer"); layer; layer = layer->next_sibling("layer"))
			{
				xml_attribute<> *attr = layer->first_attribute();
				std::string tileLayerName = attr->value();
				attr = attr->next_attribute();
				std::cout << attr->value() << "\n";
				int layerWidth = atoi(attr->value());
				attr = attr->next_attribute();
				std::cout << attr->value() << "\n";
				int layerHeight = atoi(attr->value());
				xml_node<> *layerData = layer->first_node("data");
				attr = layerData->first_attribute();
				std::cout << attr->value() << "\n";
				std::string encodeMode = attr->value();

				std::string tileFieldText = layerData->value();
				std::stringstream sstream (tileFieldText);
				std::string fieldValue;

				unsigned int **tileLayer = new unsigned int* [layerWidth];
				for(int g = 0; g < layerWidth; ++g)
				{
					tileLayer[g] = new unsigned int[layerHeight];
				}
				std::cout << "\n\n---height: " << tileHeight << "   width: " << tileWidth << "\n";

				int i = 0;
				int j = 0;

				while (std::getline(sstream, fieldValue, ','))
				{
					//std::cout << "i: " << i << "  j: " << j << "\n";
					tileLayer[j][i] = atoi (fieldValue.c_str());
					++j;
					if (j == layerWidth)
					{
						//std::cout << "\n";
						j = 0;
						++i;
					}
				}
				//std::cout << "\n";
				this->transformTiles(tileLayerName, layerWidth, layerHeight, tileLayer);
				loadTiles(tileLayerName, tileWidth, tileHeight, layerWidth, layerHeight, tileLayer);
				/*for (int a = 0; a < layerHeight; ++a)
				{
					for (int b = 0; b < layerWidth; ++b)
					{
						std::cout << (tileLayer[a][b] ? "X" : " ");
					}
					std::cout << "\n";
				}*/

				for (int i = 0; i < layerWidth; ++i)
				{
					delete [] tileLayer[i];
				}
				delete [] tileLayer;
			}

			std::cout << "test\n";
			for (xml_node<> *objectgroup = root->first_node("objectgroup"); objectgroup; objectgroup = objectgroup->next_sibling("objectgroup"))
			{
				std::cout << "test2\n";
				xml_attribute<> *attr = objectgroup->first_attribute ("name");
				std::cout << attr->value() << "\n";
				std::string objectLayer_name = attr->value();
				attr = attr->next_attribute ("width");
				std::cout << attr->value() << "\n";
				int objectLayer_Width = atoi(attr->value());
				attr = attr->next_attribute ("height");
				std::cout << attr->value() << "\n";
				int objectLayer_Height = atoi(attr->value());

				std::vector<EntityPrototype> prototypes;

				for (xml_node<> *obj = objectgroup->first_node("object"); obj; obj = obj->next_sibling("object"))
				{
					prototypes.push_back(EntityPrototype());
					EntityPrototype& prototype = prototypes.back();

					xml_attribute<> *attr = obj->first_attribute("gid");
					if (attr)
					{
						int gid = atoi (attr->value());
						prototype.id = gid;
					}

					attr = obj->first_attribute ("x");
					if (attr)
					{
						int x = atoi (attr->value());
						prototype.x = x;
					}

					attr = obj->first_attribute ("y");
					if (attr)
					{
						int y = atoi (attr->value());
						prototype.y = y;
					}

					attr = obj->first_attribute ("width");
					if (attr)
					{
						int objWidth = atoi (attr->value());
					}


					attr = obj->first_attribute ("height");
					if (attr)
					{
						int objHeight = atoi (attr->value());
					}

					attr = obj->first_attribute("name");
					if (attr)
					{
						prototype.name = attr->value();
					}
					attr = obj->first_attribute("type");
					if (attr)
					{
						prototype.type = attr->value();
					}
				}
				if (!prototypes.empty())
					this->loadEntities(objectLayer_name, prototypes);
				}
			}
			else
				std::cerr << "error: Max sucks at writing error messages.\n";
		}
		else
		{
			std::cerr << "couldn't open map\n";
		}
}

void Level::debugDrawRect(const sf::Rect<int>& rect, sf::Color outlineColor, sf::Color fillColor, int outlineThickness)
{
#ifdef JE_DEBUG
	debugDrawRects.push_back(sf::RectangleShape(sf::Vector2f(rect.width, rect.height)));
	sf::RectangleShape& r = debugDrawRects.back();
	r.setPosition(rect.left, rect.top);
	r.setOutlineColor(outlineColor);
	r.setOutlineThickness(outlineThickness);
	r.setFillColor(fillColor);
#endif
}

void Level::setSpecificOrderEntitiesPre(std::initializer_list<std::string> order)
{
	specificOrderEntitiesPre.clear();
	for (const std::string& type : order)
		specificOrderEntitiesPre.push_back(type);
	this->fixUpdateOrder();
}

void Level::setSpecificOrderEntitiesPost(std::initializer_list<std::string> order)
{
	specificOrderEntitiesPost.clear();
	for (const std::string& type : order)
		specificOrderEntitiesPost.push_back(type);
	this->fixUpdateOrder();
}

/*		protected			*/

void Level::onUpdate()
{
	//	purposefully empty - meant for subclass-specific behaviour
}

void Level::onDraw(sf::RenderTarget& target) const
{
	//	purposefully empty - meant for subclass-specific behaviour
}

void Level::beforeDraw(sf::RenderTarget& target) const
{
	//	purposefully empty - meant for subclass-specific behaviour
}

void Level::drawGUI(sf::RenderTarget& target) const
{
	//	purposefully empty - meant for subclass-specific behaviour
}

void Level::loadTiles(const std::string& layerName, int tileWidth, int tileHeight, int tilesAcross, int tilesHigh, unsigned int const * const * tiles)
{
	//	TODO: create tilemaps here
	tileLayers[layerName] = new TileGrid(this, 0, 0, tilesAcross, tilesHigh, tileWidth, tileHeight);
	TileGrid* grid = tileLayers[layerName];
	for (int x = 0; x < tilesAcross; ++x)
	{
		for (int y = 0; y < tilesHigh; ++y)
		{
			//std::cout << "x: " << x << "  y: " << y << "\n";
			//std::cout << (tiles[x][y] ? "X" : " ");
			if (tiles[x][y])
				grid->setTexture(x, y, tileSprites[tiles[x][y]]);
		}
	}
	this->addEntity(grid);
}

void Level::loadEntities(const std::string& layerName, const std::vector<EntityPrototype>& prototypes)
{
	//	purposefully empty - meant for subclass-specific behaviour
}

void Level::createTiles(const std::string& filename, int tileWidth, int tileHeight, int tilesAcross, int tilesHigh)
{
	std::cout << "tileWidth : " << tileWidth << "\nTileHeight" << tileHeight << "\n";
	const sf::Texture& texture = getGame().getTexManager().get(filename);
	const int w = texture.getSize().x / tileWidth;
	const int h = texture.getSize().y / tileHeight;
	std::cout << "should create " << w * h << " sprites created\n";
	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			//std::cout << "x:" << x << "  y: " << y << "\n";
			tileSprites.push_back(sf::Sprite(texture));
			tileSprites.back().setTextureRect(sf::IntRect(x * tileWidth, y * tileHeight, tileWidth, tileHeight));
		}
	}
	std::cout << "\n";
}

void Level::transformTiles(const std::string& layerName, int tilesAcross, int tilesHigh, unsigned  **tiles)
{
	std::cout << "Level::transformTiles()\n";
	//	no transform is done here
}

/*		private		*/
void Level::init()
{
	tileSprites.push_back(sf::Sprite());	//	empty tile (0)
	this->setCameraBounds(sf::Rect<int>(0, 0, getWidth(), getHeight()));
	this->setCameraPosition(sf::Vector2f(getWidth() / 2, getHeight() / 2));
}

void Level::fixUpdateOrder()
{
	hasSpecificUpdateOrder.clear();

	for (const std::string& type : specificOrderEntitiesPre)
	{
		hasSpecificUpdateOrder[type] = true;
	}
	for (const std::string& type : specificOrderEntitiesPost)
	{
		hasSpecificUpdateOrder[type] = true;
	}
}

void Level::limitCamera()
{
	je::limit(cameraBounds.left, cameraBounds.width / 2, width - cameraBounds.width / 2);
	je::limit(cameraBounds.top, cameraBounds.height / 2, height - cameraBounds.height / 2);
}

}
