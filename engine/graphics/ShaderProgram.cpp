#include "ShaderProgram.h"
#include <assert.h>

ShaderProgram::ShaderProgram(const char* vertexPath, const char* fragmentPath)
{
	LoadFromFiles(vertexPath, fragmentPath);
}

ShaderProgram::~ShaderProgram() { Destroy(); }

ShaderProgram::ShaderProgram(ShaderProgram&& other) noexcept : _id(other._id), _uniformCache(std::move(other._uniformCache))
{
	other._id = 0;
}

ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) noexcept
{
	if (this != &other)
	{
		Destroy();
		_uniformCache = std::move(other._uniformCache);
		_id = other._id;
		other._id = 0;
	}
	return *this;
}

void ShaderProgram::LoadFromFiles(const char* vertexPath, const char* fragmentPath)
{
	std::string vsSrc = ReadFile(vertexPath);
	std::string fsSrc = ReadFile(fragmentPath);

	if (vsSrc.empty())
	{
		std::cout << "[SP] Ошибка чтения файла: " << vertexPath << '\n';
		return;
	}
	if (fsSrc.empty())
	{
		std::cout << "[SP] Ошибка чтения файла: " << fragmentPath << '\n';
		return;
	}

	_vertexPath = vertexPath;
	_fragmentPath = fragmentPath;

	GLuint vs = CompileShader(GL_VERTEX_SHADER, vsSrc.c_str(), vertexPath);
	GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fsSrc.c_str(), fragmentPath);
	if (!vs || !fs)
	{
		glDeleteShader(vs);
		glDeleteShader(fs);
		std::cout << "[SP] Ошибка компиляции шейдеров\n";
		return;
	}

	GLuint program = LinkProgram(vs, fs);

	glDeleteShader(vs);
	glDeleteShader(fs);

	if (!program)
	{
		std::cout << "[SP] Ошибка линковки шейдеров\n";
		return;
	}

	Destroy();
	_id = program;
	BuildUniformReflection();
}

void ShaderProgram::Bind() const { glUseProgram(_id); }
void ShaderProgram::Unbind() const { glUseProgram(0); }

GLuint ShaderProgram::GetID() const { return _id; }

bool ShaderProgram::IsValid() const { return _id; }

GLuint ShaderProgram::CompileShader(GLenum type, const char* src, const char* path)
{
	GLuint shader = glCreateShader(type);

	glShaderSource(shader, 1, &src, nullptr);
	glCompileShader(shader);

	int success = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

	if (!success)
	{
		char infoLog[512];
		glGetShaderInfoLog(shader, 512, nullptr, infoLog);
		std::cerr << "[SP] Ошибка компиляции шейдера: " << path << '\n' << infoLog << "\n";

		glDeleteShader(shader);
		return 0;
	}

	return shader;
}

void ShaderProgram::BuildUniformReflection()
{
	_uniformCache.clear();

	GLint count = 0;
	glGetProgramiv(_id, GL_ACTIVE_UNIFORMS, &count);

	for (GLint i = 0; i < count; i++)
	{
		UniformInfo info;
		char name[128];
		GLsizei len = 0;
		GLint size = 0;
		GLenum type = 0;

		glGetActiveUniform(_id, i, sizeof(name), &len, &size, &type, name);

		std::string fullName(name, len);
		std::string baseName = fullName;
		if (auto bracket = baseName.find('['); bracket != std::string::npos)
			baseName.erase(bracket);

		info.location = glGetUniformLocation(_id, fullName.c_str());
		info.type = type;
		info.size = size;

		_uniformCache[baseName] = info;
	}
}

GLuint ShaderProgram::LinkProgram(GLuint vertex, GLuint fragment)
{
	GLuint program = glCreateProgram();
	glAttachShader(program, vertex);
	glAttachShader(program, fragment);
	glLinkProgram(program);

	GLint success = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success)
	{
		char infoLog[512];
		glGetProgramInfoLog(program, 512, nullptr, infoLog);
		std::cerr << "[SP] Ошибка линковки программы:\n" << infoLog << "\n";

		glDeleteProgram(program);
		return 0;
	}
	return program;
}

std::string ShaderProgram::ReadFile(const char* path)
{
	std::ifstream file(path);
	std::stringstream ss;

	if (!file.is_open())
	{
		std::cerr << "[SP] Не удалось открыть файл: " << path << '\n';
		return {};
	}

	ss << file.rdbuf();

	return std::string(ss.str());
}

void ShaderProgram::SendMat4(const char* name, const glm::mat4& v) const
{
	GLint id = 0;
	if (FindUniform(name, id))
		glUniformMatrix4fv(id, 1, GL_FALSE, glm::value_ptr(v));
}

void ShaderProgram::SendMat3(const char* name, const glm::mat3& v) const
{
	GLint id = 0;
	if (FindUniform(name, id))
		glUniformMatrix3fv(id, 1, GL_FALSE, glm::value_ptr(v));
}

void ShaderProgram::SendVec4(const char* name, const glm::vec4& v) const
{
	GLint id = 0;
	if (FindUniform(name, id))
		glUniform4fv(id, 1, glm::value_ptr(v));
}

void ShaderProgram::SendVec3(const char* name, const glm::vec3& v) const
{
	GLint id = 0;
	if (FindUniform(name, id))
		glUniform3fv(id, 1, glm::value_ptr(v));
}

void ShaderProgram::SendVec2(const char* name, const glm::vec2& v) const
{
	GLint id = 0;
	if (FindUniform(name, id))
		glUniform2fv(id, 1, glm::value_ptr(v));
}

void ShaderProgram::SendFloat(const char* name, const float v) const
{
	GLint id = 0;
	if (FindUniform(name, id))
		glUniform1f(id, v);
}

void ShaderProgram::SendInt(const char* name, const int v) const
{
	GLint id = 0;
	if (FindUniform(name, id))
		glUniform1i(id, v);
}

void ShaderProgram::SendBool(const char* name, const bool v) const { SendInt(name, static_cast<int>(v)); }

bool ShaderProgram::FindUniform(const char* name, GLint& id) const
{
	auto u = _uniformCache.find(name);
	if (u == _uniformCache.end())
	{
		std::cerr << "[SP] Не найден uniform: " << name << " для шейдеров - " << _vertexPath << " " << _fragmentPath << "\n";
		id = 0;
		return false;
	}
	id = u->second.location;
	return true;
}

void ShaderProgram::Destroy()
{
	if (_id)
	{
		glDeleteProgram(_id);
		_uniformCache.clear();
		_id = 0;
	}
}