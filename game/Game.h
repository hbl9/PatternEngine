#pragma once

#include "..\engine\camera\ICameraController.h"
#include "..\engine\game\IGame.h"
#include "..\engine\network\Network.h"
#include "..\engine\scene\SceneObject.h"

struct PlayerState
{
	uint32_t id = 0;
	glm::vec3 pos = glm::vec3(0.0f);
	float yaw = 0.0f;
	float pitch = 0.0f;
};

class Game : public IGame
{
public:
	Game(bool isHost = false, const char* connectIp = nullptr);

	void OnInit(Scene& scene) override;
	void OnUpdate(float deltaTime, Scene& scene, InputManager& im) override;
	void OnRender() override;

private:
	std::unordered_map<uint32_t, SceneObject*> _remotePlayers;

	SceneObject* _player = nullptr;
	SceneObject* _dick = nullptr;

	bool _isHost = false;
	std::string _connectIp;
};