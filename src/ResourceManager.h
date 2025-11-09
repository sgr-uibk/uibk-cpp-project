#pragma once
#include <cassert>
#include <mutex>
#include <shared_mutex>
#include <SFML/Audio/Music.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Texture.hpp>

class AssetPathResolver
{
  public:
	explicit AssetPathResolver(std::vector<std::string> const &tryRoots = {"./assets", "../assets", "../../assets"})
	{
		for(std::string const &possibleRoot : tryRoots)
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
	std::filesystem::path m_assetsRoot;
};

inline AssetPathResolver g_assetPathResolver;

// Generic resource manager for types that have a loadFromFile method (sf::Texture, sf::SoundBuffer),
// as well as sf::Font, sf::Music that have a openFromFile method. (Note that the ResourceManager must outlive the
// clients using the object for these.)

// For storage, this now uses an unordered_map containing unique_ptrs to resources on the heap,
// returned are references to the heap object owned by the unique_ptr.

// Can't directly store the Resources in a unordered_map<R> as lazy loading causes rehashes,
// and that kills the R& already given to the clients.

template <typename R> class ResourceManager
{
  public:
	ResourceManager(const ResourceManager &) = delete;
	ResourceManager &operator=(const ResourceManager &) = delete;
	ResourceManager() = default;

	static ResourceManager &inst()
	{
		static ResourceManager s_instance;
		return s_instance;
	}

	R &load(const std::string &key)
	{
		{
			// Was the resource already loaded ?
			std::shared_lock readLock(m_mutex);
			auto it = m_map.find(key);
			if(it != m_map.end() && it->second)
				return *it->second;
		}

		// Not in hashtable, go find it after expanding the asset path
		auto fullPath = g_assetPathResolver.resolveRelative(key);
		auto newRes = std::make_unique<R>(fullPath);

		{
			// now take exclusive lock to insert (and re-check to avoid races)
			std::unique_lock writeLock(m_mutex);
			auto it = m_map.find(key);
			if(it != m_map.end() && it->second)
			{
				// Another thread inserted while we were building resource
				return *it->second;
			}
			auto [newIt, inserted] = m_map.emplace(key, std::move(newRes));
			assert(inserted);
			return *newIt->second;
		}
	}

	R &get(const std::string &key)
	{
		std::shared_lock readLock(m_mutex);
		auto it = m_map.find(key);
		if(it == m_map.end() || !it->second)
			throw std::out_of_range("Resource not loaded: " + key);
		return *it->second;
	}

  private:
	std::unordered_map<std::string, std::unique_ptr<R>> m_map;
	mutable std::shared_mutex m_mutex;
};

using TextureManager = ResourceManager<sf::Texture>;
using FontManager = ResourceManager<sf::Font>;
using SoundBufferManager = ResourceManager<sf::SoundBuffer>;
using MusicManager = ResourceManager<sf::Music>;
