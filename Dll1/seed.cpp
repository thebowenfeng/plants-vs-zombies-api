#include "pch.h"
#include <Windows.h>
#include "memory.h"
#include "game.h"

typedef int(__stdcall* PickRandomSeeds)(DWORD* seedChooserPtr);
typedef int(__thiscall* SeedChooserMouseDown)(DWORD* thisPtr, int mouseX, int mouseY, int clickCount);

int getSeedBankSize() {
    int gameState = getGameUi();
    if (!(gameState == 2 || gameState == 3 || gameState == 4 || gameState == 5)) return -1;

    DWORD seedBankAddr = resolveMultiLevelPointer(std::vector<DWORD> { (DWORD)GetModuleHandle(NULL) + 0x329670, 0x868, 0x15C });
    return *(int*)(seedBankAddr + 0x24);
}

int getSeedPacketType(int index) {
    int gameState = getGameUi();
    if (!(gameState == 2 || gameState == 3 || gameState == 4 || gameState == 5)) return -2;

    DWORD seedBankAddr = resolveMultiLevelPointer(std::vector<DWORD> { (DWORD)GetModuleHandle(NULL) + 0x329670, 0x868, 0x15C });
    return *(int*)(seedBankAddr + index * 0x50 + 0x5C);
}

/*
Internally calls SeedChooserScreen::PickRandomSeeds() that randomly picks seeds and starts the game
*/
int chooseRandomSeeds() {
    if (getGameUi() != 2) return 1;

    DWORD seedChooserPtr = resolveMultiLevelPointer(std::vector<DWORD> { (DWORD)GetModuleHandle(NULL) + 0x329670, 0x874 });
    PickRandomSeeds PickRandomSeedsFunc = (PickRandomSeeds)((DWORD)GetModuleHandle(NULL) + 0x905B0);
    PickRandomSeedsFunc((DWORD*)seedChooserPtr);

    return 0;
}

/*
Force seedType during seed selection instead of going by mouse coords
*/
int chooseSeedType;
DWORD chooseSeedTypeReturnAddr;
void __declspec(naked) patchChooseSeedType() {
    __asm {
        mov eax, [chooseSeedType]
        mov edi, eax
        cmp edi, -01
        jmp[chooseSeedTypeReturnAddr];
    }
}

/*
Selects a seed based on seedType. If seed is already selected, this will deselect the seed.
Internally calls SeedChooserScreen::MouseDown() but coerces seed type without using mouse coords.

Arguments:
    - seedType: Enum representing the seed type that is to be selected
*/
int chooseSeed(int seedType) {
    if (getGameUi() != 2) return 1;

    DWORD chooseSeedTypeAddr = (DWORD)GetModuleHandle(NULL) + 0x91752;
    chooseSeedType = seedType;
    chooseSeedTypeReturnAddr = chooseSeedTypeAddr + 5;

    std::vector<BYTE> chooseSeedTypeOrig = detour((void*)chooseSeedTypeAddr, patchChooseSeedType, 5);

    DWORD seedChooserPtr = resolveMultiLevelPointer(std::vector<DWORD> { (DWORD)GetModuleHandle(NULL) + 0x329670, 0x874 });
    SeedChooserMouseDown SeedChooserMouseDownFunc = (SeedChooserMouseDown)((DWORD)GetModuleHandle(NULL) + 0x91350);

    SeedChooserMouseDownFunc((DWORD*)seedChooserPtr, 1, 1, 1);

    patchBytes((void*)chooseSeedTypeAddr, chooseSeedTypeOrig);

    return 0;
}

/*
Get number of seed selected (in bank) during seed selection
*/
int getSeedInBank() {
    int gameState = getGameUi();
    if (!(gameState == 2 || gameState == 3 || gameState == 4 || gameState == 5)) return -2;

    return (int)resolveMultiLevelPointer(std::vector<DWORD> { (DWORD)GetModuleHandle(NULL) + 0x329670, 0x874, 0xD3C });
}