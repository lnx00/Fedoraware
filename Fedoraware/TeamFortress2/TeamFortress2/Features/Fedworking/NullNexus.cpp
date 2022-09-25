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

void CNullNexus::UpdateData()
{
	auto userName = g_SteamInterfaces.Friends002->GetPersonaName();

	NullNexus::UserSettings settings;
	settings.username = userName;

	nexus.changeData(settings);
}

void CNullNexus::Init()
{
	UpdateData();

	nexus.setHandlerChat(OnMessage);

	nexus.connect("nullnexus.cathook.club", "3000", "/api/v1/client", true);

	F::Commands.Register("nn_test", [](std::deque<std::string> args) {
		nexus.sendChat("Hello, World!");
	});
}

void CNullNexus::Disconnect()
{
	nexus.disconnect();
}
