#pragma once

#include "Mesh.h"
#include "MeshFactory.h"
#include "ShaderProgram.h"
#include <string>
#include <unordered_map>

class ResourceManager
{
public:
	Mesh* GetMesh(const std::string& name) const;
	ShaderProgram* GetShader(const std::string& name) const;

	Mesh* CreateMesh(const std::string& name, const MeshData& data);
	ShaderProgram* CreateShader(const std::string& name, const char* vertexPath, const char* fragmentPath);

private:
	std::unordered_map<std::string, std::unique_ptr<Mesh>>			_meshes;
	std::unordered_map<std::string, std::unique_ptr<ShaderProgram>> _shaders;
};