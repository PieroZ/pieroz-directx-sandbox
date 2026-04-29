#pragma once
#include "prim.h"
#include <string>
#include <vector>


bool load_prim_object(const std::string& filePath, PrimObject& primObject, std::vector<PrimFace3>& faces3, std::vector<PrimFace4>& faces4);
