#include "../Hooks.h"

#include "../../Features/Aimbot/AimbotProjectile/AimbotProjectile.h"

MAKE_HOOK(C_BaseViewModel_ShouldFlipViewModel, g_Pattern.Find(L"client.dll", L"8B 91 ? ? ? ? 83 FA FF 74 3B A1 ? ? ? ? 8B CA 81 E1"), bool, __fastcall,
		  void* ecx, void* edx)
{
	const bool flippy = F::AimbotProjectile.Flippy;

	return flippy ? !flippy : flippy;

	return false;
}