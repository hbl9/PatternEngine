#include "ResourceManager.h"

Mesh* ResourceManager::GetMesh(const std::string& name) const
{
	auto mesh = _meshes.find(name);
	if (mesh == _meshes.end())
	{
		std::cout << "[ResourceManager] Mesh не найден: " << name << '\n';
		return nullptr;
	}
	return mesh->second.get();
}

ShaderProgram* ResourceManager::GetShader(const std::string& name) const
{
	auto shader = _shaders.find(name);
	if (shader == _shaders.end())
	{
		std::cout << "[ResourceManager] Shader не найден: " << name << '\n';
		return nullptr;
	}
	return shader->second.get();
}

Mesh* ResourceManager::CreateMesh(const std::string& name, const struct MeshData& data)
{
	auto temp = _meshes.find(name);
	if (temp != _meshes.end()) return temp->second.get();

	auto mesh = std::make_unique<Mesh>();
	mesh->Upload(data.vertices, data.layout, data.indices);
	_meshes[name] = std::move(mesh);
	return _meshes[name].get();
}

ShaderProgram* ResourceManager::CreateShader(const std::string& name, const char* vertexPath, const char* fragmentPath)
{
	auto temp = _shaders.find(name);
	if (temp != _shaders.end()) return temp->second.get();

	auto shader = std::make_unique<ShaderProgram>(vertexPath, fragmentPath);
	if (!shader->IsValid())
	{
		std::cout << "[ResourceManager] Ошибка шейдера: " << name << '\n';
		return nullptr;
	}
	_shaders[name] = std::move(shader);
	return _shaders[name].get();
}