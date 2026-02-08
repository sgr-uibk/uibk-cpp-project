#include "MapParser.h"
#include "ResourceManager.h"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

using json = nlohmann::json;

static std::optional<json> loadJsonFile(std::string const &filePath, std::string const &caller)
{
	auto fullPath = g_assetPathResolver.resolveRelative(filePath);
	std::ifstream file(fullPath);
	if(!file.is_open())
	{
		spdlog::error("{}: Could not open file {}", caller, fullPath.string());
		return std::nullopt;
	}

	json j;
	try
	{
		file >> j;
	}
	catch(json::parse_error const &e)
	{
		spdlog::error("{}: JSON error in {} - {}", caller, filePath, e.what());
		return std::nullopt;
	}
	return j;
}

static std::string extractObjectType(nlohmann::json const &objJson)
{
	std::string type = objJson.value("type", "");

	if(objJson.contains("properties"))
	{
		for(auto const &prop : objJson["properties"])
		{
			if(prop.contains("name") && prop["name"] == "type" && prop.contains("value") && prop["value"].is_string())
			{
				type = prop["value"].get<std::string>();
				break;
			}
		}
	}
	return type;
}

std::optional<MapBlueprint> MapParser::parse(std::string const &filePath)
{
	auto jOpt = loadJsonFile(filePath, "MapParser::parse");
	if(!jOpt.has_value())
		return std::nullopt;

	auto const &j = *jOpt;

	auto const tileDim = sf::Vector2i{j.value("tilewidth", 0), j.value("tileheight", 0)};

	std::optional<RawTileset> tilesets = std::nullopt;
	if(j.contains("tilesets") && !j["tilesets"].empty())
	{
		auto const &tsJson = j["tilesets"][0];
		tilesets = RawTileset{.imagePath = tsJson.value("image", ""),
		                      .tileDim = {tsJson.value("tilewidth", 0), tsJson.value("tileheight", 0)},
		                      .mapTileDim = tileDim,
		                      .columns = tsJson.value("columns", 0),
		                      .spacing = tsJson.value("spacing", 0),
		                      .margin = tsJson.value("margin", 0)};
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
				if(!layerJson.contains("objects"))
					continue;

				for(auto const &objJson : layerJson["objects"])
				{
					RawObject obj;
					obj.id = objJson.value("id", -1);
					obj.name = objJson.value("name", "");
					obj.type = extractObjectType(objJson);
					obj.position = {objJson.value("x", 0.f), objJson.value("y", 0.f)};
					obj.size = {objJson.value("width", 0.f), objJson.value("height", 0.f)};
					obj.rotation = objJson.value("rotation", 0.f);

					if(objJson.contains("properties"))
					{
						for(auto const &prop : objJson["properties"])
						{
							if(!prop.contains("name") || !prop.contains("value"))
								continue;

							std::string propName = prop["name"];
							auto val = prop["value"];

							if(val.is_string())
								obj.properties[propName] = val.get<std::string>();
							else
								obj.properties[propName] = val.dump();
						}
					}
					objects.push_back(obj);
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
	auto jOpt = loadJsonFile(filePath, "MapParser::parseSpawnsOnly");
	if(!jOpt.has_value())
		return {};

	auto const &j = *jOpt;

	std::vector<sf::Vector2f> spawns;

	if(!j.contains("layers"))
		return spawns;

	for(auto const &layerJson : j["layers"])
	{
		if(layerJson.value("type", "") != "objectgroup" || !layerJson.contains("objects"))
			continue;

		for(auto const &objJson : layerJson["objects"])
		{
			std::string type = extractObjectType(objJson);

			if(type == ObjectType::PLAYER_SPAWN)
			{
				spawns.push_back({objJson.value("x", 0.f), objJson.value("y", 0.f)});
			}
		}
	}

	return spawns;
}
