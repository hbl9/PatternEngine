#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <iostream>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

enum class FrameRateMode
{
	VSync,
	Unlimited
};

class Window
{
public:
	Window() = default;
	Window(int width = 600, int height = 600, const char* title = "Window",
		GLFWmonitor* monitor = nullptr, GLFWwindow* share = nullptr, FrameRateMode frameRateMode = FrameRateMode::VSync);
	~Window();

	bool IsCursorCaptured() const;
	bool ShouldClose();

	void SetCursorCaptured(bool captured);
	void SetVSync(FrameRateMode frameRateMode);
	void SetGLClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
	float GetAspectRatio() const;

	void MakeCurrent();

	GLFWwindow* GetWindow() const;
	glm::ivec2 GetSizeWindow();

	void UpdateFrameBufferSize();
	void SwapBuffers();
	void PollEvents();

	void Destroy();

private:
	GLFWwindow* _window = nullptr;
	int _width = 0, _height = 0;

	bool _cursorCaptured = false;
};