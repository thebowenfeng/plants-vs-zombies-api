#include "pch.h"
#include <Windows.h>
#include <string.h>
#include <vector>

std::vector<BYTE> detour(void* toHook, void* ourFunc, int len) {
    std::vector<BYTE> stolenBytes;
    DWORD currProt;
    VirtualProtect(toHook, len, PAGE_EXECUTE_READWRITE, &currProt);

    for (int i = 0; i < len; i++) {
        stolenBytes.push_back(((BYTE*)toHook)[i]);
    }
    memset(toHook, 0x90, len);

    DWORD relAddr = ((DWORD)ourFunc - (DWORD)toHook) - 5;
    *(BYTE*)toHook = 0xE9;
    *(DWORD*)((DWORD)toHook + 1) = relAddr;

    DWORD temp;
    VirtualProtect(toHook, len, currProt, &temp);
    return stolenBytes;
}

void* trampolineHook(void* toHook, void* ourFunc, int len) {
    void* orig = VirtualAlloc(NULL, len + 5, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    // Return null virtualAlloc somehow fail
    if (orig == NULL) {
        return NULL;
    }
    else {
        memcpy(orig, toHook, len);
        ((BYTE*)orig)[len] = 0xE9;
        *(DWORD*)((DWORD)orig + len + 1) = ((DWORD)toHook - (DWORD)orig) - 5;
        detour(toHook, ourFunc, len);
        return orig;
    }
}

std::vector<BYTE> patchBytes(void* addrToPatch, std::vector<BYTE> bytes) {
    std::vector<BYTE> stolenBytes;
    DWORD currProt;
    VirtualProtect(addrToPatch, bytes.size(), PAGE_EXECUTE_READWRITE, &currProt);

    for (int i = 0; i < bytes.size(); i++) {
        stolenBytes.push_back(((BYTE*)addrToPatch)[i]);
    }
    for (int i = 0; i < bytes.size(); i++) {
        ((BYTE*)addrToPatch)[i] = bytes[i];
    }

    DWORD temp;
    VirtualProtect(addrToPatch, bytes.size(), currProt, &temp);
    return stolenBytes;
}

std::vector<BYTE> nopBytes(void* addrToPatch, int size) {
    std::vector<BYTE> stolenBytes;
    DWORD currProt;
    VirtualProtect(addrToPatch, size, PAGE_EXECUTE_READWRITE, &currProt);

    for (int i = 0; i < size; i++) {
        stolenBytes.push_back(((BYTE*)addrToPatch)[i]);
    }
    memset(addrToPatch, 0x90, size);

    DWORD temp;
    VirtualProtect(addrToPatch, size, currProt, &temp);
    return stolenBytes;
}

DWORD resolveMultiLevelPointer(std::vector<DWORD> pointers) {
    DWORD* currPtr = 0;
    for (int i = 0; i < pointers.size(); i++) {
        if (currPtr == 0) {
            currPtr = (DWORD*)pointers[i];
            currPtr = (DWORD*)*currPtr;
        }
        else {
            DWORD* newPtr = (DWORD*)((DWORD)currPtr + pointers[i]);
            currPtr = (DWORD*)*newPtr;
        }
    }
    return (DWORD)currPtr;
}