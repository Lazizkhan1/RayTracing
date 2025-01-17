#include "Renderer.h"
#include <iostream>

#include "Walnut/Random.h"

#include <execution>

namespace Utils {

	static uint32_t ConvertToRGBA(const glm::vec4& color)
	{
		uint8_t r = (uint8_t)(color.r * 255.0f);
		uint8_t g = (uint8_t)(color.g * 255.0f);
		uint8_t b = (uint8_t)(color.b * 255.0f);
		uint8_t a = (uint8_t)(color.a * 255.0f);

		uint32_t result = (a << 24) | (b << 16) | (g << 8) | r;
		return result;
	}

	static glm::vec4 ConvertToVec4(uint32_t color)
	{
		uint8_t r = (color >> 16) & 0xFF;
		uint8_t g = (color >> 8) & 0xFF;
		uint8_t b = color & 0xFF;
		uint8_t a = (color >> 24) & 0xFF;

		return glm::vec4(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
	}

	static glm::vec3 ConvertToVec3(uint32_t color)
	{
		uint8_t r = (color >> 16) & 0xFF;
		uint8_t g = (color >> 8) & 0xFF;
		uint8_t b = (color >> 0) & 0xFF;

		return glm::vec3(r / 255.0f, g / 255.0f, b / 255.0f);
	}

	static uint32_t PCG_Hash(uint32_t input) {
		uint32_t state = input * 747796405U + 2891336453U;
		uint32_t word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
		return (word >> 22u) ^ word;
	}

	static float RandomFloat(uint32_t& seed)
	{
		seed = PCG_Hash(seed);
		return (float)seed / (float)UINT32_MAX;
	}

	static float RandomFloat(float min, float max, uint32_t& seed)
	{
		return RandomFloat(seed) * (max - min) + min;
	}

	static glm::vec3 InUnitSphere(uint32_t& seed)
	{
		return glm::normalize(glm::vec3(
			RandomFloat(seed) * 2.0f - 1.0f,
			RandomFloat(seed) * 2.0f - 1.0f,
			RandomFloat(seed) * 2.0f - 1.0f
			)
		);
	}

	static glm::vec3 Vec3(float min, float max, uint32_t& seed)
	{
		return glm::vec3(RandomFloat(seed) * (max - min) + min, RandomFloat(seed) * (max - min) + min, RandomFloat(seed) * (max - min) + min);
	}

	static glm::vec4 GetSkyColor(const SkyBox& skybox, const glm::vec3& direction) {
		glm::vec4 defaultSkyColor(0.6f, 0.7f, 0.9f, 1.0f);
		if (!skybox.IsLoaded)
			return defaultSkyColor;
		glm::vec3 normalDirection = glm::normalize(direction);
		const float PI = 3.14159265358979323846264338327950288f;
		float u = 0.5f + atan2(direction.z, direction.x) / (2.0f * PI);
		float v = 0.5f - asin(direction.y) / PI;

		int x = (int)(u * skybox.Width);
		int y = (int)(v * skybox.Height);

		if (x < 0) x = 0;
		if (x >= skybox.Width) x = skybox.Width - 1;
		if (y < 0) y = 0;
		if (y >= skybox.Height) y = skybox.Height - 1;

		return Utils::ConvertToVec4(skybox.SkyBoxData[x + y * skybox.Width]);
	}
}

void Renderer::OnResize(uint32_t width, uint32_t height)
{
	if (m_FinalImage)
	{
		// No resize necessary
		if (m_FinalImage->GetWidth() == width && m_FinalImage->GetHeight() == height)
			return;

		m_FinalImage->Resize(width, height);
	}
	else
	{
		m_FinalImage = std::make_shared<Walnut::Image>(width, height, Walnut::ImageFormat::RGBA);
	}

	delete[] m_ImageData;
	m_ImageData = new uint32_t[width * height];

	delete[] m_AccumulationData;
	m_AccumulationData = new glm::vec4[width * height];

	m_ImageHorizontalIter.resize(width);
	m_ImageVerticalIter.resize(height);
	for (uint32_t i = 0; i < width; i++)
		m_ImageHorizontalIter[i] = i;
	for (uint32_t i = 0; i < height; i++)
		m_ImageVerticalIter[i] = i;
}

void Renderer::Render(const Scene& scene, const Camera& camera)
{
	m_ActiveScene = &scene;
	m_ActiveCamera = &camera;
	
	if (m_FrameIndex == 1)
		memset(m_AccumulationData, 0, m_FinalImage->GetWidth() * m_FinalImage->GetHeight() * sizeof(glm::vec4));

#define MT 1
#if MT
	std::for_each(std::execution::par, m_ImageVerticalIter.begin(), m_ImageVerticalIter.end(),
		[this](uint32_t y)
		{
			std::for_each(std::execution::par, m_ImageHorizontalIter.begin(), m_ImageHorizontalIter.end(),
				[this, y](uint32_t x)
				{
					glm::vec4 color = PerPixel(x, y);
					m_AccumulationData[x + y * m_FinalImage->GetWidth()] += color;

					glm::vec4 accumulatedColor = m_AccumulationData[x + y * m_FinalImage->GetWidth()];
					accumulatedColor /= (float)m_FrameIndex;

					accumulatedColor = glm::clamp(accumulatedColor, glm::vec4(0.0f), glm::vec4(1.0f));
					m_ImageData[x + y * m_FinalImage->GetWidth()] = Utils::ConvertToRGBA(accumulatedColor);
				});
		});

#else

	for (uint32_t y = 0; y < m_FinalImage->GetHeight(); y++)
	{
		for (uint32_t x = 0; x < m_FinalImage->GetWidth(); x++)
		{
			glm::vec4 color = PerPixel(x, y);
			m_AccumulationData[x + y * m_FinalImage->GetWidth()] += color;

			glm::vec4 accumulatedColor = m_AccumulationData[x + y * m_FinalImage->GetWidth()];
			accumulatedColor /= (float)m_FrameIndex;

			accumulatedColor = glm::clamp(accumulatedColor, glm::vec4(0.0f), glm::vec4(1.0f));
			m_ImageData[x + y * m_FinalImage->GetWidth()] = Utils::ConvertToRGBA(accumulatedColor);
		}
	}
#endif

	m_FinalImage->SetData(m_ImageData);

	if (m_Settings.Accumulate)
		m_FrameIndex++;
	else
		m_FrameIndex = 1;
}

glm::vec4 Renderer::PerPixel(uint32_t x, uint32_t y)
{
	Ray ray;
	ray.Origin = m_ActiveCamera->GetPosition();
	ray.Direction = m_ActiveCamera->GetRayDirections()[x + y * m_FinalImage->GetWidth()];
	
	glm::vec4 light(0.0f, 0.0f, 0.0f, 1.0f);
	glm::vec4 contribution(1.0f);

	uint32_t seed = x + y * m_FinalImage->GetWidth();
	seed *= m_FrameIndex;

	int bounces = 5;
	for (int i = 0; i < bounces; i++)
	{
		seed += i;
		Renderer::HitPayload payload = TraceRay(ray);

		const Sphere& sphere = m_ActiveScene->Spheres[payload.ObjectIndex];
		const Material& material = m_ActiveScene->Materials[sphere.MaterialIndex];
		const SkyBox& skybox = m_ActiveScene->SkyBox;

		if (payload.HitDistance < 0.0f)
		{
			glm::vec4 skyColor = Utils::GetSkyColor(skybox, ray.Direction);
			light += skyColor * contribution;
			break;
		}


		//contribution *= material.Albedo;
		light += material.GetEmission();

		ray.Origin = payload.WorldPosition + payload.WorldNormal * 0.0001f;

		glm::vec3 specularDir = ray.Direction = glm::reflect(ray.Direction,
			payload.WorldNormal + material.Roughness * Utils::Vec3(-0.5f, 0.5f, seed));
		glm::vec3 diffuse = ray.Direction = glm::normalize(payload.WorldNormal +  Utils::InUnitSphere(seed));
		
		bool isSpecularBounce = material.SpecularPower >= Utils::RandomFloat(0.0f, 1.0f, seed);


		ray.Direction = glm::normalize(glm::mix(diffuse, specularDir, material.Metallic * isSpecularBounce));
		contribution *= glm::mix(material.Albedo, material.SpecularColor, isSpecularBounce);
		//std::cout << "contribution: " << contribution.x << " " << contribution.y << " " << contribution.z << std::endl;
		
	}

	return glm::vec4(light);
}

Renderer::HitPayload Renderer::TraceRay(const Ray& ray)
{
	// (bx^2 + by^2)t^2 + (2(axbx + ayby))t + (ax^2 + ay^2 - r^2) = 0
	// where
	// a = ray origin
	// b = ray direction
	// r = radius
	// t = hit distance

	int closestSphere = -1;
	float hitDistance = std::numeric_limits<float>::max();
	for (size_t i = 0; i < m_ActiveScene->Spheres.size(); i++)
	{
		const Sphere& sphere = m_ActiveScene->Spheres[i];
		glm::vec3 origin = ray.Origin - sphere.Position;

		float a = glm::dot(ray.Direction, ray.Direction);
		float b = 2.0f * glm::dot(origin, ray.Direction);
		float c = glm::dot(origin, origin) - sphere.Radius * sphere.Radius;

		// Quadratic forumula discriminant:
		// b^2 - 4ac

		float discriminant = b * b - 4.0f * a * c;
		if (discriminant < 0.0f)
			continue;

		// Quadratic formula:
		// (-b +- sqrt(discriminant)) / 2a

		// float t0 = (-b + glm::sqrt(discriminant)) / (2.0f * a); // Second hit distance (currently unused)
		float closestT = (-b - glm::sqrt(discriminant)) / (2.0f * a);
		if (closestT > 0.0f && closestT < hitDistance)
		{
			hitDistance = closestT;
			closestSphere = (int)i;
		}
	}

	if (closestSphere < 0)
		return Miss(ray);

	return ClosestHit(ray, hitDistance, closestSphere);
}

Renderer::HitPayload Renderer::ClosestHit(const Ray& ray, float hitDistance, int objectIndex)
{
	Renderer::HitPayload payload;
	payload.HitDistance = hitDistance;
	payload.ObjectIndex = objectIndex;

	const Sphere& closestSphere = m_ActiveScene->Spheres[objectIndex];

	glm::vec3 origin = ray.Origin - closestSphere.Position;
	payload.WorldPosition = origin + ray.Direction * hitDistance;
	payload.WorldNormal = glm::normalize(payload.WorldPosition);

	payload.WorldPosition += closestSphere.Position;

	return payload;
}

Renderer::HitPayload Renderer::Miss(const Ray& ray)
{
	Renderer::HitPayload payload;
	payload.HitDistance = -1.0f;
	return payload;
}
