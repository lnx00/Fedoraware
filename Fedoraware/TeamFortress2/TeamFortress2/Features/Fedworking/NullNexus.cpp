#include "NullNexus.h"
#include "../Commands/Commands.h"

static NullNexus nexus;
static std::string serverSteamID = "";

void OnMessage(const std::string& user, const std::string& msg, int color)
{
	if (msg.size() > 128 || user.size() > 32)
	{
		I::Cvar->ConsolePrintf("[NullNexus] Message or Username too long.");
		return;
	}

	if (I::EngineClient->IsInGame())
	{
		const std::string chatMsg = tfm::format("\x07%06X[\x07%06XNullnexus\x07%06X] \x07%06X%s\x01: %s", 0x5e3252, 0xba3d9a, 0x5e3252, color, user, msg);
		I::ClientModeShared->m_pChatElement->ChatPrintf(0, chatMsg.c_str());
	}
	else
	{
		I::Cvar->ConsoleColorPrintf(Vars::Menu::Colors::MenuAccent, "[Nullnexus] %s: %s\n", user.c_str(), msg.c_str());
	}
}

void OnAuthedPlayers(std::vector<std::string> steamIDs)
{
	if (!I::EngineClient->IsInGame()) { return; }

	for (int i = 0; i < I::EngineClient->GetMaxClients(); i++)
	{
		PlayerInfo_t playerInfo{ };
		if (I::EngineClient->GetPlayerInfo(i, &playerInfo))
		{
			if (playerInfo.friendsID == 0) { continue; }

			MD5Value_t result;
			std::string steamHash = std::to_string(playerInfo.friendsID) + playerInfo.name;
			MD5_ProcessSingleBuffer(steamHash.c_str(), strlen(steamHash.c_str()), result);

			std::stringstream hashStream;
			hashStream << std::hex;
			for (auto i : result.bits)
			{
				hashStream << std::setw(2) << std::setfill('0') << static_cast<int>(i);
			}
			
			steamHash = hashStream.str();
			std::remove_if(steamIDs.begin(), steamIDs.end(), [&](std::string& steamID) {
				if (steamID == steamHash)
				{
					// Use actual steamid to set cat status
					return true;
				}

				return false;
			});
		}
	}
}

void CNullNexus::UpdateServer(NullNexus::UserSettings& settings)
{
	const auto netChannel = I::EngineClient->GetNetChannelInfo();
	static int gHostSpawnCount = 1;

	if (netChannel && !serverSteamID.empty())
	{
		PlayerInfo_t playerInfo{ };
		if (I::EngineClient->GetPlayerInfo(I::EngineClient->GetLocalPlayer(), &playerInfo))
		{
			MD5Value_t result{ };
			std::string steamHash = std::to_string(playerInfo.friendsID) + playerInfo.name;

			std::stringstream hashStream;
			hashStream << std::hex;
			for (const auto i : result.bits)
			{
				hashStream << std::setw(2) << std::setfill('0') << static_cast<int>(i);
			}

			steamHash = hashStream.str();
			settings.tf2server = NullNexus::TF2Server(true, serverSteamID, steamHash, gHostSpawnCount);
			return;
		}
	}

	// Not connected
	settings.tf2server = NullNexus::TF2Server(false);
}

void CNullNexus::UpdateData()
{
	auto userName = g_SteamInterfaces.Friends002->GetPersonaName();

	NullNexus::UserSettings settings;
	settings.username = userName;

	// Tell nullnexus about the current server we are connected to.
	UpdateServer(settings);
	nexus.changeData(settings);
}

void CNullNexus::Init()
{
	UpdateData();

	nexus.setHandlerChat(OnMessage);
	nexus.setHandlerAuthedplayers(OnAuthedPlayers);

	nexus.connect("nullnexus.cathook.club", "3000", "/api/v1/client", true);

	F::Commands.Register("nn_test", [](std::deque<std::string> args) {
		nexus.sendChat("Hello, World!");
	});
}

void CNullNexus::Disconnect()
{
	nexus.disconnect();
}
