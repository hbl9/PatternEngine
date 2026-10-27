#pragma once

#include "..\graphics\ResourceManager.h"
#include "SceneObject.h"
#include <glm/vec3.hpp>
#include <glm/fwd.hpp>
#include <memory>
#include <string>
#include <vector>

class Scene
{
public:
	Scene() = default;

	SceneObject* CreateObject(const char* oName, const char* mName, const char* sName,
		const glm::vec3& pos = glm::vec3(0.0f), SceneObject* p = nullptr);

	SceneObject* CreateDot(const char* name, const glm::vec3& pos = glm::vec3(0.0f), SceneObject* p = nullptr);

	ResourceManager& GetResources();
	SceneObject* GetFirstOrNull(const std::string& name);

	void ClearObjects();
	void RemoveObject(SceneObject* obj);

	void CollectChildren(SceneObject* obj, std::vector<SceneObject*>& out) const;

	void Draw(const glm::mat4& view, const glm::mat4& projection);

private:
	std::unique_ptr<ResourceManager> _resources = std::make_unique<ResourceManager>();
	std::vector<std::unique_ptr<SceneObject>> _objects;
};