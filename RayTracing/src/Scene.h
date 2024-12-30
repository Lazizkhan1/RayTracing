#pragma once

#include <glm/glm.hpp>

#include <vector>
#include "stb_image.h"

struct Material
{
	glm::vec4 Albedo{ 1.0f };
	float Roughness = 1.0f;
	float Metallic = 0.0f;
	glm::vec4 EmissionColor{ 1.0f};
	float EmissionPower = 0.0f;
	glm::vec4 SpecularColor{ 1.0f };
	float SpecularPower = 0.0f;
	glm::vec4 GetEmission() const { return EmissionColor * EmissionPower; }
};

struct Sphere
{
	glm::vec3 Position{0.0f};
	float Radius = 0.5f;

	int MaterialIndex = 0;
};

struct SkyBox
{
	int Width = 0, Height = 0, Channels = 0;
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



