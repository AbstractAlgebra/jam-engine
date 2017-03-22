#include "jam-engine/Graphics/TexManager.hpp"

#ifdef JE_DEBUG
	#include <iostream>
#endif

#include <iostream>
namespace je
{

TexManager::TexManager()
	:path("img/")
{
}
    

    const sf::Texture& TexManager::get(const std::string& id)
    {
        auto it = textures.find(id);
        if (it == textures.end())
        {
            it = textures.emplace(std::pair<std::string, sf::Texture>(id, sf::Texture())).first;
            it->second.loadFromFile(path + id);
            std::cout << path << "" << id << std::endl;
#ifdef JE_DEBUG
            std::cout << "Loaded " << id << std::endl;
#endif
        }
        return (it->second);
    }

void TexManager::setPath(const std::string& pathname)
{
	path = pathname;
}

} // je

/*

 sf::Texture& tex = textures[id]; tex.loadFromFile(path + id); return terx;
*/
