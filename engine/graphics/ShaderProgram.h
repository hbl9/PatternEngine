#pragma once

#include <fstream>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

struct UniformInfo
{
	GLint  location = -1;
	GLenum type = 0;
	GLint  size = 1;
};

class ShaderProgram
{
public:
	ShaderProgram() = default;
	ShaderProgram(const char* vertexPath, const char* fragmentPath);
	~ShaderProgram();

	ShaderProgram(const ShaderProgram&) = delete;
	ShaderProgram& operator=(const ShaderProgram&) = delete;

	ShaderProgram(ShaderProgram&& other)			noexcept;
	ShaderProgram& operator=(ShaderProgram&& other) noexcept;

	void LoadFromFiles(const char* vertexPath, const char* fragmentPath);

	void Bind() const;
	void Unbind() const;

	GLuint GetID() const;

	bool IsValid() const;

	void SendMat4(const char* name, const glm::mat4& value) const;
	void SendMat3(const char* name, const glm::mat3& value) const;
	void SendVec4(const char* name, const glm::vec4& value) const;
	void SendVec3(const char* name, const glm::vec3& value) const;
	void SendVec2(const char* name, const glm::vec2& value) const;
	void SendFloat(const char* name, float value) const;
	void SendInt(const char* name, int value) const;
	void SendBool(const char* name, bool value) const;

private:
	std::unordered_map<std::string, UniformInfo> _uniformCache;
	GLuint _id = 0;

	// Пути для отладки
	std::string _vertexPath;
	std::string _fragmentPath;

	bool FindUniform(const char* name, GLint& id) const;

	GLuint CompileShader(GLenum type, const char* src, const char* path);

	void BuildUniformReflection();

	GLuint LinkProgram(GLuint vertex, GLuint fragment);

	std::string ReadFile(const char* path);

	void Destroy();
};