#pragma once

#include <vector>
#include <Windows.h>

std::vector<BYTE> detour(void* toHook, void* ourFunc, int len);

void* trampolineHook(void* toHook, void* ourFunc, int len);

std::vector<BYTE> patchBytes(void* addrToPatch, std::vector<BYTE> bytes);

std::vector<BYTE> nopBytes(void* addrToPatch, int size);

DWORD resolveMultiLevelPointer(std::vector<DWORD> pointers);