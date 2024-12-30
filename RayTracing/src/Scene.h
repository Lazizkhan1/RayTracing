#pragma once

#include <glm/glm.hpp>

#include <vector>
#include "stb_image.h"

struct Material
{
	glm::vec3 Albedo{ 1.0f };
	float Roughness = 1.0f;
	float Metallic = 0.0f;
	glm::vec3 EmissionColor{ 0.0f };
	float EmissionPower = 0.0f;
	glm::vec3 SpecularColor{ 1.0f };
	float SpecularPower = 0.0f;
	glm::vec3 GetEmission() const { return EmissionColor * EmissionPower; }
};

struct Sphere
{
	glm::vec3 Position{0.0f};
	float Radius = 0.5f;

	int MaterialIndex = 0;
};

struct SkyBox
{
	std::vector<uint32_t> SkyBoxData;
	bool IsHDR = false;
	bool IsLoaded = false;

};

struct Scene
{
	std::vector<Sphere> Spheres;
	std::vector<Material> Materials;
	SkyBox SkyBox;
};



