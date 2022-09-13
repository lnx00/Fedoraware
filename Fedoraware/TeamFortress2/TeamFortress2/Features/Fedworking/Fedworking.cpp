#include "Fedworking.h"
#include "NullNexus/NullNexus.hpp"
#include "../Menu/Pong/Pong.h"
#include "../../Utils/Base64/Base64.hpp"

enum MessageType {
	None,
	Marker,	// [ Type, X-Pos, Y-Pos, Z-Pos, Player-IDX ]
	ESP,	// [ Type, X-Pos, Y-Pos, Z-Pos, Player-IDX ]
	Pong
};

void CFedworking::HandleMessage(const char* pMessage)
{
	std::string encMsg(pMessage);
	encMsg.erase(0, 4); // Remove FED@ prefix

	const std::string msg = Base64::Decode(encMsg);
	if (msg.empty()) { return; }

	const auto dataVector = Utils::SplitString(msg, "&");
	int msgType;
	try { msgType = std::stoi(dataVector[0]); }
	catch (...) { ConsoleLog("Failed to read message type!"); return; }

	switch (msgType) {
	case None:
		{
			// Undefined message type (Sent by party crasher)
			break;
		}

	case Marker:
		{
			if (dataVector.size() == 5) {
				try {
					const float xPos = std::stof(dataVector[1]);
					const float yPos = std::stof(dataVector[2]);
					const float zPos = std::stof(dataVector[3]);
					const int playerIndex = std::stoi(dataVector[4]);

					PlayerInfo_t playerInfo{};
					I::EngineClient->GetPlayerInfo(playerIndex, &playerInfo);

					if (playerInfo.userID != 0) {
						CGameEvent* markerEvent = I::GameEventManager->CreateNewEvent("show_annotation");
						if (markerEvent) {
							markerEvent->SetInt("id", playerIndex);
							markerEvent->SetFloat("worldPosX", xPos);
							markerEvent->SetFloat("worldPosY", yPos);
							markerEvent->SetFloat("worldPosZ", zPos);
							markerEvent->SetFloat("lifetime", 5.0f);

							markerEvent->SetBool("show_distance", true);
							markerEvent->SetString("text", playerInfo.name);
							markerEvent->SetString("play_sound", "coach/coach_go_here.wav");

							I::GameEventManager->FireEventClientSide(markerEvent);
						}
					}
				}
				catch (...) { ConsoleLog("Failed to read marker data!"); }
			}
			break;
		}

	case ESP:
		{
			if (dataVector.size() == 5) {
				try {
					const float xPos = std::stof(dataVector[1]);
					const float yPos = std::stof(dataVector[2]);
					const float zPos = std::stof(dataVector[3]);
					const int playerIndex = std::stoi(dataVector[4]);

					G::DormantPlayerESP[playerIndex].Location = { xPos, yPos, zPos };
					G::DormantPlayerESP[playerIndex].LastUpdate = I::EngineClient->Time();
				} catch (...) { ConsoleLog("Failed to read ESP data!"); }
			}
			break;
		}

	case Pong:
		{
			F::Pong.ReceiveData(dataVector);
			break;
		}

	default:
		{
			// Unknown message type
			ConsoleLog("Unknown message received!");
			break;
		}
	}
}

void CFedworking::SendMarker(const Vec3& pPos, int pPlayerIdx)
{
	std::stringstream msg;
	msg << Marker << "&" << pPos.x << "&" << pPos.y << "&" << pPos.z << "&" << pPlayerIdx;
	SendMessage(msg.str());
}

void CFedworking::SendESP(CBaseEntity* pPlayer)
{
	if (!pPlayer->GetDormant() && pPlayer->IsInValidTeam() && pPlayer->IsAlive()) {
		const float lastUpdate = G::DormantPlayerESP[pPlayer->GetIndex()].LastUpdate;
		if (lastUpdate == 0.f || I::EngineClient->Time() - lastUpdate >= 0.4f) {
			const Vec3 playerPos = pPlayer->GetVecOrigin();
			std::stringstream msg;
			msg << ESP << "&" << playerPos.x << "&" << playerPos.y << "&" << playerPos.z << "&" << pPlayer->GetIndex();
			SendMessage(msg.str());
		}
	}
}

void CFedworking::SendPong(const std::string& pData)
{
	std::stringstream msg;
	msg << Pong << "&" << pData;
	SendMessage(msg.str());
}

void CFedworking::SendMessage(const std::string& pData)
{
	const std::string encMsg = Base64::Encode(pData);
	if (encMsg.size() <= 253) {
		std::string cmd = "tf_party_chat \"FED@";
		cmd.append(encMsg);
		cmd.append("\"");
		I::EngineClient->ClientCmd_Unrestricted(cmd.c_str());
	} else {
		ConsoleLog("Failed to send message! The message was too long.");
	}
}

void CFedworking::Run()
{
	if (!Vars::Misc::PartyNetworking.Value || G::ShouldShift) { return; }

	if (const auto& pLocal = g_EntityCache.GetLocal()) {
		// Party marker
		static KeyHelper markerKey{ &Vars::Misc::PartyMarker.Value };
		if (Vars::Misc::PartyMarker.Value && markerKey.Pressed()) {
			const Vec3 viewAngles = I::EngineClient->GetViewAngles();
			Vec3 vForward;
			Math::AngleVectors(viewAngles, &vForward);

			CGameTrace trace;
			CTraceFilterHitscan traceFilter = {};
			Ray_t traceRay;
			vForward = pLocal->GetEyePosition() + vForward * MAX_TRACE_LENGTH;
			traceRay.Init(pLocal->GetEyePosition(), vForward);
			I::EngineTrace->TraceRay(traceRay, MASK_SOLID, &traceFilter, &trace);
			if (trace.DidHit()) {
#ifdef _DEBUG
				I::DebugOverlay->AddLineOverlay(trace.vStartPos, trace.vEndPos, 255, 0, 0, false, 1.0f);
#endif
				F::Fedworking.SendMarker(trace.vEndPos, pLocal->GetIndex());
			}
		}

		// Party ESP
		if (Vars::Misc::PartyESP.Value) {
			SendESP(pLocal);
			for (const auto& player : g_EntityCache.GetGroup(EGroupType::PLAYERS_ALL))
			{
				if (player->GetIndex() == pLocal->GetIndex()) { continue; }
				SendESP(player);
			}
		}
	}
}

void CFedworking::ConsoleLog(const std::string& pMessage)
{
	std::string consoleMsg = "[Fedworking] ";
	consoleMsg.append(pMessage);
	consoleMsg.append("\n");
	I::Cvar->ConsoleColorPrintf({ 225, 177, 44, 255 }, consoleMsg.c_str());
}
