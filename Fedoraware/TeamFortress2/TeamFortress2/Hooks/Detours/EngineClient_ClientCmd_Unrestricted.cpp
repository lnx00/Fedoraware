#include "../Hooks.h"

#include "../../Features/Menu/MaterialEditor/MaterialEditor.h"
#include "../../Features/Commands/Commands.h"
#include "../../Features/Chams/DMEChams.h"

class split_q
{
public:
	split_q() : in_q(false) {}

	bool operator()(char ch) const
	{
		if (ch == '\"')
		{
			in_q = !in_q;
		}
		return !in_q && ch == ' ';
	}

private:
	mutable bool in_q;

};

MAKE_HOOK(EngineClient_ClientCmd_Unrestricted, Utils::GetVFuncPtr(I::EngineClient, 106), void, __fastcall,
		  void* ecx, void* edx, const char* szCmdString)
{
	std::string cmdString(szCmdString);
	std::deque<std::string> cmdArgs;

	// Yes I will use boost for this
	//boost::split(cmdArgs, cmdString, split_q());
	// TODO: Return a deque in SplitString
	auto cmdArgsVec = Utils::SplitString(cmdString, " ");
	std::ranges::move(cmdArgsVec, std::back_inserter(cmdArgs));

	if (!cmdArgs.empty())
	{
		const std::string cmdName = cmdArgs.front();
		cmdArgs.pop_front();

		if (F::Commands.Run(cmdName, cmdArgs))
		{
			return;
		}
	}

	if (cmdString == "disconnect")
	{
		F::DMEChams.DeleteMaterials();	//	schizoid
	}

	Hook.Original<FN>()(ecx, edx, cmdString.c_str());
}