#pragma once

#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#endif

#include <cstdint>
#include <glm/vec3.hpp>
#include <string>
#include <vector>
#include <ws2def.h>

struct NetworkEvents
{
	std::vector<PlayerState> remoteStates;
	std::vector<uint32_t> disconnects;
};

class Network
{
public:
	static constexpr double CLIENT_TIMEOUT = 15.0;
	static constexpr uint16_t DEFAULT_PORT = 54545;
	static constexpr uint32_t MAGIC = 0x47504C59;
	static constexpr uint32_t MAGIC_DISCONNECT = 0x47504C60;
	static constexpr uint32_t INVALID_ID = 0xFFFFFFFF;

	Network();
	~Network();

	void Update(float deltaTime, const PlayerState& localState, NetworkEvents& outEvents);

	double Now();

	bool StartHost(uint16_t port = DEFAULT_PORT);
	bool StartClient(const std::string& hostIp, uint16_t port = DEFAULT_PORT);

	void SendLocalState(const PlayerState& local);

	void Poll(std::vector<PlayerState>& outStates, std::vector<uint32_t>& outDisconnects);
	void PollDisconnects(std::vector<uint32_t>& outleftIds);

	void BroadcastDisconnect(uint32_t leftId);

	bool IsHost() const;
	bool IsClient() const;
	bool IsRunning() const;

	uint32_t GetLocalId() const;

private:
	struct Peer
	{
		sockaddr_in addr{};
		uint32_t id = 0;
		double lastSeen = 0.0;
	};

	SOCKET _socket = INVALID_SOCKET;
	bool _isHost = false, _hostTimedOut = false;
	uint32_t _localId = 0;
	float _sendTimer = 0.0f;
	double _lastHostPacketTime = 0.0;

	sockaddr_in _serverAddr{};
	std::vector<Peer> _peers;
	uint32_t _nextClientId = 1;

	void CloseSocket();

	void SendRaw(const void* data, int size, const sockaddr_in& to);
	void SendStateTo(const PlayerState& st, const sockaddr_in& to);
};