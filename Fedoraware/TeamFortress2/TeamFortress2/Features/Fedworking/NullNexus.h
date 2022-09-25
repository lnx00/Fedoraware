#pragma once
#include "../../SDK/SDK.h"
#include "libNullNexus/libNullNexus.hpp"

class CNullNexus {
	void UpdateData();

public:
	void Init();
	void Disconnect();
};

ADD_FEATURE(CNullNexus, NullNexus)