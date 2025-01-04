#pragma once

#include <glm/glm.hpp>

#include <vector>
#include "stb_image.h"
#include <iostream>

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

	SkyBox(const char* filename) {
		if (filename == nullptr)
			return;

		unsigned char* data = stbi_load(filename, &Width, &Height, &Channels, 4);
		if (!data) {
			std::cout << "Failed to load skybox image" << std::endl;
		}
		else {
			std::vector<uint32_t> skyboxData(Width * Height);

			for (int i = 0; i < Width * Height; i++) {
				skyboxData[i] = data[i * 4 + 3] << 24 | (data[i * 4] << 16) | (data[i * 4 + 1] << 8) | data[i * 4 + 2];
			}
			stbi_image_free(data);
			SkyBoxData = skyboxData;
			IsLoaded = true;
		}
	}

};

struct Scene
{
	std::vector<Sphere> Spheres;
	std::vector<Material> Materials;
	SkyBox SkyBox = nullptr;
};



