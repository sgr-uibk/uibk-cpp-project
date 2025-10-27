#pragma once
#include <unordered_map>
#include <memory>
#include <string>
#include <stdexcept>
#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>

class AssetPathResolver
{
public:
	explicit AssetPathResolver(std::vector<std::string> tryRoots = {"./assets", "../assets", "../../assets"})
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

	std::filesystem::path resolveRelative(const std::string &rel) const
	{
		// the / is the path append operator :)
		std::filesystem::path abs = std::filesystem::absolute(m_assetsRoot / rel);
		if(!std::filesystem::exists(abs))
			throw std::runtime_error("Asset can't be found at " + abs.string());
		return abs;
	}

	const std::filesystem::path &getAssetsRoot() const
	{
		return m_assetsRoot;
	}

private:
	std::vector<std::string> m_candidates;
	std::filesystem::path m_assetsRoot;
};

static AssetPathResolver s_assetPathResolver{};

// Generic resource manager for types that have a loadFromFile method
// (sf::Texture, sf::Font, sf::SoundBuffer)

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
		auto fullPath = s_assetPathResolver.resolveRelative(key);
		std::unique_ptr<R> res = std::make_unique<R>();
		if(!res->loadFromFile(fullPath))
			throw std::runtime_error("Failed to load resource: " + fullPath.string());

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

// sf::Music is different (https://www.sfml-dev.org/tutorials/3.0/audio/sounds/#loading-and-playing-a-sound)
// There is no loadFromFile, but a openFromFile, The data is only loaded later when the music is played.
// It also helps to keep in mind that the audio file has to remain available as long as it is played.
class MusicManager
{
public:
	// Return shared_ptr s.t. caller can own music while playing
	// TODO I still can't imagine the client outliving the MusicManager,
	// but the difference in naming makes it ugly to shove it into the above templated manager...
	std::shared_ptr<sf::Music> load(const std::string &key, const std::string &filename)
	{
		auto it = m_map.find(key);
		if(it != m_map.end())
			return it->second;

		std::shared_ptr<sf::Music> music = std::make_shared<sf::Music>();
		if(!music->openFromFile(filename))
			throw std::runtime_error("Failed to open music: " + filename);

		m_map.emplace(key, music);
		return music;
	}

private:
	std::unordered_map<std::string, std::shared_ptr<sf::Music>> m_map;
};

using TextureManager = ResourceManager<sf::Texture>;
using FontManager = ResourceManager<sf::Font>;
using SoundBufferManager = ResourceManager<sf::SoundBuffer>;
