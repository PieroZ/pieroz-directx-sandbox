#pragma once
#include "PrimLightDef.h"
#include <unordered_map>
#include <string>

// Registry of emissive-light definision keyed by prim index. Persisted to JSON
// so map authors can mark a prim type as a light emitter once and 
// have every placed instance light the scene on load.
class PrimLightRegistry
{
public:
	// Returns true if the prim index has an entry (regardless of enabled state).
	bool Has(int primIndex) const noexcept;
	// Return the entry for the prim index, or a default-constructed one if absent.
	PrimLightDef Get(int primIndex) const noexcept;
	// Stored/updates the entry for the prim index.
	void Set(int primIndex, const PrimLightDef& def);
	// Removes the entry for the prim index (if any).
	void Remove(int primIndex);

	const std::unordered_map<int, PrimLightDef>& Entries() const noexcept { return entries; };

	// JSON persistence. Load is tolerant of missing file (leaves registry empty).
	void Load(const std::string& path);
	void Save(const std::string& path) const;

private:
	std::unordered_map<int, PrimLightDef> entries;
};