#include "PrimLightRegistry.h"
#include "json.hpp"
#include <fstream>

using json = nlohmann::json;

bool PrimLightRegistry::Has(int primIndex) const noexcept
{
	return entries.find(primIndex) != entries.end();
}

PrimLightDef PrimLightRegistry::Get(int primIndex) const noexcept
{
	const auto it = entries.find(primIndex);
	if (it != entries.end())
	{
		return it->second;
	}
	return PrimLightDef{};
}

void PrimLightRegistry::Set(int primIndex, const PrimLightDef& def)
{
	entries[primIndex] = def;
}

void PrimLightRegistry::Remove(int primIndex)
{
	entries.erase(primIndex);
}

void PrimLightRegistry::Load(const std::string& path)
{
	std::ifstream file(path);
	if (!file.is_open())
	{
		// No file yet: start with an empty registry.
		return;
	}

	json j;
	file >> j;
	entries.clear();
	if (!j.contains("primLights"))
	{
		return;
	}

	for (const auto& e : j.at("primLights"))
	{
		const int idx = e.at("primIndex").get<int>();
		PrimLightDef def;
		def.enabled = e.value("enabled", false);
		def.offset.x = e.value("offsetX", 0.0f);
		def.offset.y = e.value("offsetY", 1.0f);
		def.offset.z = e.value("offsetZ", 0.0f);
		def.color.x = e.value("colorR", 1.0f);
		def.color.y = e.value("colorG", 0.85f);
		def.color.z = e.value("colorB", 0.6f);
		def.intensity = e.value("intensity", 2.0f);
		def.attConst = e.value("attConst", 1.0f);
		def.attLin = e.value("attLin", 0.0045f);
		def.attQuad  = e.value("attQuad", 0.0075f);
		def.spotlight  = e.value("spotlight", true);
		def.direction.x  = e.value("dirX", 0.0f);
		def.direction.y  = e.value("dirY", -1.0f);
		def.direction.z  = e.value("dirZ", 0.0f);
		def.innerConeDeg  = e.value("innerConeDeg", 25.0f);
		def.outerConeDeg  = e.value("outerConeDeg", 40.0f);
		entries[idx] = def;
	}
}

void PrimLightRegistry::Save(const std::string& path) const
{
	json j;
	j["primLights"] = json::array();
	for (const auto& [idx, def] : entries)
	{
		json e;
		e["primIndex"] = idx;
		e["enabled"] = def.enabled;
		e["offsetX"] = def.offset.x;
		e["offsetY"] = def.offset.y;
		e["offsetZ"] = def.offset.z;
		e["colorR"] = def.color.x;
		e["colorG"] = def.color.y;
		e["colorB"] = def.color.z;
		e["intensity"] = def.intensity;
		e["attConst"] = def.attConst;
		e["attLin"] = def.attLin;
		e["attQuad"] = def.attQuad;
		e["spotlight"] = def.spotlight;
		e["dirX"] = def.direction.x;
		e["dirY"] = def.direction.y;
		e["dirZ"] = def.direction.z;
		e["innerConeDeg"] = def.innerConeDeg;
		e["outerConeDeg"] = def.outerConeDeg;
		j["primLights"].push_back(std::move(e));;
	}

	std::ofstream file(path);
	if (!file.is_open())
	{
		throw std::runtime_error("PrimLightRegistry: cannot write file: " + path);
	}
	file << j.dump(2);
}