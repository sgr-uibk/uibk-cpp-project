#include "MapParser.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <spdlog/spdlog.h>
#include <filesystem>

#include "ResourceManager.h"

using json = nlohmann::json;

std::optional<MapBlueprint> MapParser::parse(std::string const &filePath)
{
	auto fullPath = g_assetPathResolver.resolveRelative(filePath);
	std::ifstream file(fullPath);
	if(!file.is_open())
	{
		spdlog::error("MapParser: Could not open file {}", fullPath.string());
		return std::nullopt;
	}

	json j;
	try
	{
		file >> j;
	}
	catch(json::parse_error const &e)
	{
		spdlog::error("MapParser: JSON error in {} - {}", filePath, e.what());
		return std::nullopt;
	}

	auto const tileDim = sf::Vector2i{j.value("tilewidth", 0), j.value("tileheight", 0)};

	std::optional<RawTileset> tilesets = std::nullopt;
	if(j.contains("tilesets") && !j["tilesets"].empty())
	{
		auto const &tsJson = j["tilesets"][0];
		RawTileset ts{.imagePath = tsJson.value("image", ""),
		              .tileDim = {tsJson.value("tilewidth", 0), tsJson.value("tileheight", 0)},
		              .mapTileDim = tileDim,
		              .columns = tsJson.value("columns", 0),
		              .firstGid = tsJson.value("firstgid", 1),
		              .spacing = tsJson.value("spacing", 0),
		              .margin = tsJson.value("margin", 0)};

		// Store map tile dimensions (from map root, not tileset)
		tilesets = ts;
	}

	std::vector<RawLayer> layers{};
	std::vector<RawObject> objects{};
	if(j.contains("layers"))
	{
		for(auto const &layerJson : j["layers"])
		{
			std::string type = layerJson.value("type", "");

			if(type == "tilelayer")
			{
				RawLayer layer;
				layer.name = layerJson.value("name", "Unnamed");
				layer.dim = {layerJson.value("width", 0), layerJson.value("height", 0)};
				layer.visible = layerJson.value("visible", true);

				if(layerJson.contains("data"))
				{
					layer.data = layerJson["data"].get<std::vector<TileType>>();
				}
				layers.push_back(layer);
			}
			else if(type == "objectgroup")
			{
				if(layerJson.contains("objects"))
				{
					for(auto const &objJson : layerJson["objects"])
					{
						RawObject obj;
						obj.id = objJson.value("id", -1);
						obj.name = objJson.value("name", "");
						obj.type = objJson.value("type", ""); // Crucial: "player_spawn", etc.
						obj.position = {objJson.value("x", 0.f), objJson.value("y", 0.f)};
						obj.size = {objJson.value("width", 0.f), objJson.value("height", 0.f)};
						obj.rotation = objJson.value("rotation", 0.f);

						if(objJson.contains("properties"))
						{
							for(auto const &prop : objJson["properties"])
							{
								// Tiled properties are array of objects {name, type, value}
								if(prop.contains("name") && prop.contains("value"))
								{
									std::string propName = prop["name"];
									auto val = prop["value"];

									if(val.is_string())
									{
										obj.properties[propName] = val.get<std::string>();
									}
									else
									{
										obj.properties[propName] = val.dump();
									}

									if(propName == "type" && val.is_string())
									{
										obj.type = val.get<std::string>();
									}
								}
							}
						}
						objects.push_back(obj);
					}
				}
			}
		}
	}

	return MapBlueprint({.dimInTiles = {j.value("width", 0), j.value("height", 0)},
	                     .tileDim = tileDim,
	                     .tileset = tilesets,
	                     .layers = layers,
	                     .objects = objects});
}

std::vector<sf::Vector2f> MapParser::parseSpawnsOnly(std::string const &filePath)
{
	auto fullPath = g_assetPathResolver.resolveRelative(filePath);
	std::ifstream file(fullPath);
	if(!file.is_open())
	{
		spdlog::error("MapParser::parseSpawnsOnly: Could not open file {}", fullPath.string());
		return {};
	}

	json j;
	try
	{
		file >> j;
	}
	catch(json::parse_error const &e)
	{
		spdlog::error("MapParser::parseSpawnsOnly: JSON error in {} - {}", filePath, e.what());
		return {};
	}

	std::vector<sf::Vector2f> spawns;

	if(j.contains("layers"))
	{
		for(auto const &layerJson : j["layers"])
		{
			if(layerJson.value("type", "") == "objectgroup" && layerJson.contains("objects"))
			{
				for(auto const &objJson : layerJson["objects"])
				{
					std::string type = objJson.value("type", "");

					if(objJson.contains("properties"))
					{
						for(auto const &prop : objJson["properties"])
						{
							if(prop.contains("name") && prop["name"] == "type" && prop.contains("value") &&
							   prop["value"].is_string())
							{
								type = prop["value"].get<std::string>();
								break;
							}
						}
					}

					if(type == "player_spawn")
					{
						sf::Vector2f pos{objJson.value("x", 0.f), objJson.value("y", 0.f)};
						spawns.push_back(pos);
					}
				}
			}
		}
	}

	return spawns;
}
