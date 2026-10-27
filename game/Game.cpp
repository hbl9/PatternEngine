#include "..\engine\graphics\MeshFactory.h"
#include "..\engine\scene\Scene.h"
#include "Game.h"

Game::Game(bool isHost, const char* connectIp) :
	_isHost(isHost), _connectIp(connectIp ? connectIp : "") {
}

void Game::OnInit(Scene& scene)
{
	if (_network)
	{
		if (_isHost) _network->StartHost(Network::DEFAULT_PORT);
		else if (!_connectIp.empty()) _network->StartClient(_connectIp, Network::DEFAULT_PORT);
	}

	scene.GetResources().CreateMesh("cube", MeshFactory::CreateCube());
	scene.GetResources().CreateMesh("plane", MeshFactory::CreatePlane(10.0f, 10.0f));
	scene.GetResources().CreateShader("basic", "assets/shaders/basic.vert", "assets/shaders/basic.frag");
	scene.GetResources().CreateShader("plane", "assets/shaders/plane.vert", "assets/shaders/plane.frag");
	scene.GetResources().CreateShader("player", "assets/shaders/player.vert", "assets/shaders/player.frag");

	SceneObject* plane = scene.CreateObject("Plane", "plane", "plane");
	plane->SetColor(0.55f, 0.55f, 0.55f);
	plane->SetUniform("uColor", plane->GetColor());

	_player = scene.CreateObject("player", "cube", "player", _cameraController->GetPos());
	_player->SetColor(glm::vec3(0.8f, 0.5f, 0.2f));
	_player->SetUniform("uColor", _player->GetColor());

	_dick = scene.CreateDot("Dick");
	scene.CreateObject("cube", "cube", "basic", glm::vec3(-0.5f, 0.5f, 0.0f), _dick);
	scene.CreateObject("cube", "cube", "basic", glm::vec3(0.5f, 0.5f, 0.0f), _dick);
	scene.CreateObject("cube", "cube", "basic", glm::vec3(0.0f, 1.5f, 0.0f), _dick);
	scene.CreateObject("cube", "cube", "basic", glm::vec3(0.0f, 2.5f, 0.0f), _dick);
}

void Game::OnUpdate(float deltaTime, Scene& scene, InputManager& im)
{
	_dick->Rotate(deltaTime, glm::vec3(0.0f, 1.0f, 0.0f));
	_player->SetLocalPosition(_cameraController->GetPos());

	if (_network->IsRunning())
	{
		PlayerState me;
		me.id = _network->GetLocalId();
		me.pos = _cameraController->GetPos();
		me.yaw = _cameraController->GetYaw();
		me.pitch = _cameraController->GetPitch();

		NetworkEvents ne;
		_network->Update(deltaTime, me, ne);

		for (uint32_t id : ne.disconnects)
		{
			auto rp = _remotePlayers.find(id);
			if (rp != _remotePlayers.end())
			{
				std::cout << "[Net] Пользователь с id=" << id << " покидает нас\n";
				scene.RemoveObject(_remotePlayers[id]);
				_remotePlayers.erase(rp);
			}
		}

		for (const auto& st : ne.remoteStates)
		{
			if (st.id == _network->GetLocalId()) continue;

			auto rp = _remotePlayers.find(st.id);
			if (rp == _remotePlayers.end())
			{
				SceneObject* obj = scene.CreateObject("remotePlayer", "cube", "player", glm::vec3(0.0f));
				obj->SetColor(glm::vec3(0.2f, 0.5f, 0.8f) * glm::vec3(st.id));
				obj->SetUniform("uColor", obj->GetColor());
				obj->SetLocalScale(glm::vec3(0.2f, 0.2f, 0.2f));
				obj->SetLocalPosition(st.pos);
				obj->SetRotationFromYawPitch(st.yaw, st.pitch);
				_remotePlayers[st.id] = obj;
			}
			else
			{
				rp->second->SetLocalPosition(st.pos);
				rp->second->SetRotationFromYawPitch(st.yaw, st.pitch);
			}
		}
	}
}

void Game::OnRender()
{

}