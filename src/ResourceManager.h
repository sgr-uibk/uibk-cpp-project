#pragma once
#include <cassert>
#include <mutex>
#include <shared_mutex>
#include <SFML/Audio/Music.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Texture.hpp>

class PathResolver
{
  public:
	explicit PathResolver(std::string_view rootName)
	{
		std::filesystem::path const cwd = std::filesystem::current_path();
		std::filesystem::path const tryBase[] = {cwd, cwd.parent_path(), cwd.parent_path().parent_path()};
		for(auto const &possibleBase : tryBase)
		{
			auto const candidate = possibleBase / rootName;
			if(std::filesystem::exists(candidate) && std::filesystem::is_directory(candidate))
			{
				// That exists, take it!
				m_rootPath = candidate;
				return;
			}
		}
		throw std::runtime_error("None of the candidate directories exist.");
	}

	[[nodiscard]] std::filesystem::path resolveRelative(std::string_view rel) const
	{
		// the / is the path append operator :)
		std::filesystem::path abs = std::filesystem::absolute(m_rootPath / rel);
		if(!std::filesystem::exists(abs))
		{
			std::string const msg = abs.string() + " doesn't exist at " + m_rootPath.string();
			throw std::runtime_error(msg);
		}
		return abs;
	}

	[[nodiscard]] std::filesystem::path const &getAssetsRoot() const
	{
		return m_rootPath;
	}

  private:
	std::filesystem::path m_rootPath;
};

inline PathResolver g_assetPathResolver("assets");

// Generic resource manager for types that have a loadFromFile method (sf::Texture, sf::SoundBuffer),
// as well as sf::Font, sf::Music that have a openFromFile method. (Note that the ResourceManager must outlive the
// clients using the object for these.)

// For storage, this now uses an unordered_map containing unique_ptrs to resources on the heap,
// returned are references to the heap object owned by the unique_ptr.

// Can't directly store the Resources in a unordered_map<R> as lazy loading causes rehashes,
// and that kills the R& already given to the clients.

template <typename R>
concept ConstructibleFromPath = std::is_constructible_v<R, std::filesystem::path>;
template <ConstructibleFromPath R> class ResourceManager
{
  public:
	ResourceManager(ResourceManager const &) = delete;
	ResourceManager &operator=(ResourceManager const &) = delete;
	ResourceManager() = default;

	static ResourceManager &inst()
	{
		static ResourceManager s_instance;
		return s_instance;
	}

	R &load(std::string const &key)
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

	R &get(std::string const &key)
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
