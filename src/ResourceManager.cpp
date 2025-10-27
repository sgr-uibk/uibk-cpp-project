#include "ResourceManager.h"
#include <memory>
#include <string>
#include <stdexcept>
#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>

AssetPathResolver::AssetPathResolver(std::vector<std::string> tryRoots)
	: m_candidates(std::move(tryRoots))
{
	for(auto &possibleRoot : m_candidates)
	{
		std::filesystem::path abs = std::filesystem::absolute(possibleRoot);
		if(std::filesystem::exists(abs) && std::filesystem::is_directory(abs))
		{
			// That exists, take it!
			m_assetsRoot = std::move(abs);
			return;
		}
	}
	throw std::runtime_error("None of the candidate directories exist.");
}

std::filesystem::path AssetPathResolver::resolveRelative(const std::string &rel) const
{
	// the / is the path append operator :)
	std::filesystem::path abs = std::filesystem::absolute(m_assetsRoot / rel);
	if(!std::filesystem::exists(abs))
		throw std::runtime_error("Asset can't be found at " + abs.string());
	return abs;
}

AssetPathResolver g_assetPathResolver;
