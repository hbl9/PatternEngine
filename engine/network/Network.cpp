#include "network.h"
#include <WS2tcpip.h>
#include <WinSock2.h>
#include <Windows.h>
#include <chrono>
#include <cstdint>
#include <glm/fwd.hpp>
#include <iostream>
#include <string>
#include <string.h>
#include <vector>
#include <ws2def.h>

Network::Network()
{
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		std::cout << "[Net] Ошибка WSAStartup\n";
}

Network::~Network()
{
	CloseSocket();
	WSACleanup();
}

void Network::Update(float deltaTime, const PlayerState& localState, NetworkEvents& outEvents)
{
	if (IsRunning())
	{
		outEvents.remoteStates.clear();
		outEvents.disconnects.clear();
		_sendTimer += deltaTime;
		Poll(outEvents.remoteStates, outEvents.disconnects);

		if (_isHost) PollDisconnects(outEvents.disconnects);
		else if (!_hostTimedOut && Now() - _lastHostPacketTime > CLIENT_TIMEOUT)
		{
			std::cout << "[Net] Хост перестал отвечать на запросы\n";
			_hostTimedOut = true;
			outEvents.disconnects.push_back(0);
		}
		if (_sendTimer >= 1.0f / 30.0f)  // 30 раз в секунду
		{
			_sendTimer = 0.0f;
			SendLocalState(localState);
		}
	}
}

double Network::Now()
{
	return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

bool Network::StartHost(uint16_t port)
{
	CloseSocket();

	_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (!IsRunning())
	{
		std::cout << "[Net] Ошибка socket()\n";
		return false;
	}

	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);

	if (bind(_socket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		std::cout << "[Net] Ошибка bind(), возможно целевой порт занят\n";
		CloseSocket();
		return false;
	}

	u_long mode = 1;
	ioctlsocket(_socket, FIONBIO, &mode);

	_isHost = true;
	_localId = 0;

	std::cout << "[Net] Хост включил порт " << port << '\n';
	return true;
}

bool Network::StartClient(const std::string& hostIp, uint16_t port)
{
	CloseSocket();

	_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (!IsRunning())
	{
		std::cout << "[Net] Ошибка socket()\n";
		return false;
	}

	_serverAddr.sin_family = AF_INET;
	_serverAddr.sin_port = htons(port);
	if (inet_pton(AF_INET, hostIp.c_str(), &_serverAddr.sin_addr) != 1)
	{
		std::cout << "[Net] Неверный IP: " << hostIp << '\n';
		CloseSocket();
		return false;
	}

	u_long mode = 1;
	ioctlsocket(_socket, FIONBIO, &mode);

	_isHost = false;
	_localId = INVALID_ID;
	_hostTimedOut = false;
	_lastHostPacketTime = Now();
	_sendTimer = 0.0f;

	std::cout << "[Net] Подключение -> " << hostIp << ':' << port << '\n';

	PlayerState hello;
	hello.id = INVALID_ID;
	SendLocalState(hello);

	return true;
}

void Network::SendLocalState(const PlayerState& local)
{
	if (!IsRunning()) return;

	if (_isHost)
		for (auto& p : _peers)
			SendStateTo(local, p.addr);
	else
		SendStateTo(local, _serverAddr);
}

void Network::Poll(std::vector<PlayerState>& outStates, std::vector<uint32_t>& outDisconnects)
{
	if (!IsRunning()) return;

	uint8_t buf[64];

	while (true)
	{
		sockaddr_in from{};
		int fromLen = sizeof(from);
		int n = recvfrom(_socket, (char*)buf, sizeof(buf), 0, (sockaddr*)&from, &fromLen);
		if (n <= 0) break;
		if (n < 8) continue;

		uint32_t magic = 0, id = 0;
		float x, y, z, yaw, pitch;
		int off = 0;
		memcpy(&magic, buf + off, 4); off += 4;

		if (magic == MAGIC_DISCONNECT)
		{
			uint32_t leftId = 0;
			memcpy(&leftId, buf + 4, 4);
			outDisconnects.push_back(leftId);

			if (_isHost)
				for (auto peer = _peers.begin(); peer != _peers.end(); peer++)
					if (peer->id == leftId)
					{
						for (auto& p : _peers)
						{
							if (p.id == leftId) continue;

							uint8_t b[8];
							int o = 0;
							memcpy(b + o, &MAGIC_DISCONNECT, 4); o += 4;
							memcpy(b + o, &leftId, 4); o += 4;
							SendRaw(b, o, p.addr);
						}
						_peers.erase(peer);
						break;
					}

			continue;
		}

		memcpy(&id, buf + off, 4); off += 4;
		memcpy(&x, buf + off, 4); off += 4;
		memcpy(&y, buf + off, 4); off += 4;
		memcpy(&z, buf + off, 4); off += 4;
		memcpy(&yaw, buf + off, 4); off += 4;
		memcpy(&pitch, buf + off, 4); off += 4;

		if (magic != MAGIC || n < 28) continue;

		if (_isHost)
		{
			Peer* peer = nullptr;
			for (auto& p : _peers)
				if (p.addr.sin_addr.s_addr == from.sin_addr.s_addr && p.addr.sin_port == from.sin_port)
				{
					peer = &p;
					break;
				}

			if (!peer)
			{
				Peer pr;
				pr.addr = from;
				pr.id = _nextClientId++;
				pr.lastSeen = Now();
				_peers.push_back(pr);
				peer = &_peers.back();
				std::cout << "[Net] Новый пользователь, id=" << peer->id << '\n';
			}
			else
				peer->lastSeen = Now();

			PlayerState reply;
			reply.id = peer->id;
			reply.pos = glm::vec3(x, y, z);
			reply.yaw = yaw;
			reply.pitch = pitch;
			SendStateTo(reply, from);

			PlayerState st;
			st.id = peer->id;
			st.pos = glm::vec3(x, y, z);
			st.yaw = yaw;
			st.pitch = pitch;
			outStates.push_back(st);

			for (auto& p : _peers)
			{
				if (p.addr.sin_addr.s_addr == from.sin_addr.s_addr && p.addr.sin_port == from.sin_port)
					continue;

				PlayerState fwd;
				fwd.id = peer->id;
				fwd.pos = glm::vec3(x, y, z);
				fwd.yaw = yaw;
				fwd.pitch = pitch;
				SendStateTo(fwd, p.addr);
			}
		}
		else
		{
			if (_localId == INVALID_ID && id != INVALID_ID)
			{
				_localId = id;
				std::cout << "[Net] Присвоен id=" << _localId << '\n';
			}

			_lastHostPacketTime = Now();

			PlayerState st;
			st.id = id;
			st.pos = glm::vec3(x, y, z);
			st.yaw = yaw;
			st.pitch = pitch;
			outStates.push_back(st);
		}
	}
}

void Network::PollDisconnects(std::vector<uint32_t>& outleftIds)
{
	if (!_isHost || !IsRunning()) return;

	double now = Now();
	auto p = _peers.begin();
	while (p != _peers.end())
	{
		if (now - p->lastSeen > CLIENT_TIMEOUT)
		{
			std::cout << "[Net] Клиент id=" << p->id << " превышено время ожидания\n";
			outleftIds.push_back(p->id);
			BroadcastDisconnect(p->id);
			p = _peers.erase(p);
		}
		else
			p++;
	}
}

void Network::BroadcastDisconnect(uint32_t leftIds)
{
	if (!IsRunning()) return;

	uint8_t buf[8];
	int o = 0;
	memcpy(buf + o, &MAGIC_DISCONNECT, 4); o += 4;
	memcpy(buf + o, &leftIds, 4); o += 4;

	if (_isHost)
		for (auto& p : _peers)
			SendRaw(buf, o, p.addr);
	else
		SendRaw(buf, o, _serverAddr);
}

bool Network::IsHost() const { return _isHost; }
bool Network::IsClient() const { return !_isHost; }
bool Network::IsRunning() const { return _socket != INVALID_SOCKET; }

uint32_t Network::GetLocalId() const { return _localId; }

void Network::CloseSocket()
{
	if (IsRunning())
	{
		closesocket(_socket);
		_socket = INVALID_SOCKET;
	}
	_peers.clear();
}

void Network::SendRaw(const void* data, int size, const sockaddr_in& to)
{
	if (IsRunning()) sendto(_socket, (const char*)data, size, 0, (const sockaddr*)&to, sizeof(to));
}

void Network::SendStateTo(const PlayerState& st, const sockaddr_in& to)
{
	uint8_t buf[28];
	int off = 0;

	memcpy(buf + off, &MAGIC, 4); off += 4;
	memcpy(buf + off, &st.id, 4); off += 4;
	memcpy(buf + off, &st.pos.x, 4); off += 4;
	memcpy(buf + off, &st.pos.y, 4); off += 4;
	memcpy(buf + off, &st.pos.z, 4); off += 4;
	memcpy(buf + off, &st.yaw, 4); off += 4;
	memcpy(buf + off, &st.pitch, 4); off += 4;

	SendRaw(buf, off, to);
}