#pragma once
#include <cassert>
#include <SFML/Audio/Music.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Texture.hpp>


class AssetPathResolver
{
public:
	explicit AssetPathResolver(std::vector<std::string> tryRoots = {"./assets", "../assets", "../../assets"});

	std::filesystem::path resolveRelative(const std::string &rel) const;

	const std::filesystem::path &getAssetsRoot() const
	{
		return m_assetsRoot;
	}

private:
	std::vector<std::string> m_candidates;
	std::filesystem::path m_assetsRoot;
};

extern AssetPathResolver g_assetPathResolver;

// Generic resource manager for types that have a loadFromFile method (sf::Texture, sf::SoundBuffer),
// as well as sf::Font, sf::Music that have a openFromFile method. (Note that the ResourceManager must outlive the
// clients using the object for these.)

// For storage, this now uses an unordered_map containing unique_ptrs to resources on the heap,
// returned are references to the heap object owned by the unique_ptr.

// Can't directly store the Resources in a unordered_map<R> as lazy loading causes rehashes,
// and that kills the R& already given to the clients.

template <typename R>
class ResourceManager
{
public:
	// Need unique ownership, can't copy the Manager!
	ResourceManager(const ResourceManager &) = delete;
	ResourceManager &operator=(const ResourceManager &) = delete;
	ResourceManager() = default; // Needed for the default construction of the static member below...

	// This is a Meyers Singleton, see https://en.wikipedia.org/wiki/Singleton_pattern
	static ResourceManager &inst()
	{
		static ResourceManager s_instance;
		return s_instance;
	}

	R &load(const std::string &key)
	{
		// Was the resource already loaded ?
		auto it = m_map.find(key);
		if(it != m_map.end() && it->second)
			return *it->second; // then return a ref to the unique_ptr

		// Not in hashtable, go find it after expanding the asset path
		auto fullPath = g_assetPathResolver.resolveRelative(key);
		std::unique_ptr<R> res = std::make_unique<R>(fullPath);

		// The resource now lives in the map and we return a reference to it
		auto [newIt, bInserted] = m_map.emplace(key, std::move(res));
		assert(bInserted); // It wasn't present before
		return *newIt->second;
	}

	R &get(const std::string &key)
	{
		auto it = m_map.find(key);
		if(it == m_map.end() || !it->second)
			throw std::out_of_range("Resource not loaded: " + key);
		return *it->second;
	}

private:
	std::unordered_map<std::string, std::unique_ptr<R>> m_map;
};

using TextureManager = ResourceManager<sf::Texture>;
using FontManager = ResourceManager<sf::Font>;
using SoundBufferManager = ResourceManager<sf::SoundBuffer>;
using MusicManager = ResourceManager<sf::Music>;
