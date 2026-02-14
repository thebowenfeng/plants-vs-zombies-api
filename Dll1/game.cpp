#include "pch.h"
#include "memory.h"
#include "seed.h"
#include "game.h"
#include <iostream>

typedef int(__thiscall* GameOverDialogButtonDepress)(DWORD* thisPtr, int theId);
typedef void(__stdcall* CutSceneUpdateZombiesWon)(DWORD* thisPtr);
typedef int(__thiscall* SeedChooserScreenDraw)(int thisPtr, int a2);

/*
Gets the current game UI state (i.e which screen is the game currently on)

2 - Seed selection/waiting to start
3 - In game
*/
int getGameUi() {
    return (int)resolveMultiLevelPointer(std::vector<DWORD> { (DWORD)GetModuleHandle(NULL) + 0x329670, 0x91C });
}

/*
Gets the current game result

0 - none (in progress)
1 - won
2 - lost
*/
int getGameResult() {
    return (int)resolveMultiLevelPointer(std::vector<DWORD> { (DWORD)GetModuleHandle(NULL) + 0x329670, 0x9A8 });
}

/*
Start the game when in seed selection. Internally calls SeedChooserScreen::OnStartButton()
Note: Will crash the game if invoked improperly (i.e game isn't meant to start)
*/
int startGame() {
    if (getGameUi() != 2) return 1;
    if (getSeedInBank() != getSeedBankSize()) return 2;

    DWORD seedChooserOnStartAddr = (DWORD)GetModuleHandle(NULL) + 0x90200;
    DWORD seedChooserPtr = resolveMultiLevelPointer(std::vector<DWORD> { (DWORD)GetModuleHandle(NULL) + 0x329670, 0x874 });

    __asm {
        push esi
        mov esi, [seedChooserPtr]
        call[seedChooserOnStartAddr]
        pop esi
    }

    return 0;
}

DWORD copySurvivalDialogReturnAddr = NULL;
DWORD survivalDialogAddr = NULL;
std::vector<BYTE> copyZombiesWonSurvivalDialogStolenBytes;
void __declspec(naked) copySurvivalDialogAddrPatch() {
    __asm {
        add esp, 0x4
        mov DWORD PTR[esp + 0x78], eax
        mov survivalDialogAddr, eax
        jmp[copySurvivalDialogReturnAddr]
    }
}

/*
Sets a detour on DLL load to copy zombie won survival mode dialog's address that
can later be used to invoke level restart
*/
void copyZombiesWonSurvivalDialogAddr() {
    if (copySurvivalDialogReturnAddr != NULL) return;

    DWORD copyDialogAddr = (DWORD)GetModuleHandle(NULL) + 0x3FA1A;
    copySurvivalDialogReturnAddr = copyDialogAddr + 7;
    copyZombiesWonSurvivalDialogStolenBytes = detour((void*)copyDialogAddr, copySurvivalDialogAddrPatch, 7);
}

void cleanupZombieSurvivalDialogHook() {
    DWORD copyDialogAddr = (DWORD)GetModuleHandle(NULL) + 0x3FA1A;
    patchBytes((void*)copyDialogAddr, copyZombiesWonSurvivalDialogStolenBytes);
}

/*
Restart a survival level
*/
int restartSurvivalLevel() {
    if (getGameResult() != 2) return 1;

    GameOverDialogButtonDepress GameOverDialogButtonDepressFunc = (GameOverDialogButtonDepress)((DWORD)GetModuleHandle(NULL) + 0x5B720);
    GameOverDialogButtonDepressFunc((DWORD*)(survivalDialogAddr + 0xA0), 1000);

    return 0;
}

/*
Callback that fires when zombies win (only when cutscene is over)
*/
CutSceneUpdateZombiesWon CutSceneUpdateZombiesWonOrigFunc;
void __stdcall hookCutSceneUpdateZombiesWon(DWORD* thisPtr) {
    if (*(int*)((DWORD)thisPtr + 0x8) == 11000) {
        std::cout << "Game over" << std::endl;
    }
    return CutSceneUpdateZombiesWonOrigFunc(thisPtr);
}

/*
Hooks into CutScene::UpdateZombiesWon
*/
void trampHookCutSceneUpdateZombieWon() {
    DWORD cutSceneUpdateZombieWonAddr = (DWORD)GetModuleHandle(NULL) + 0x3F650;
    CutSceneUpdateZombiesWonOrigFunc = (CutSceneUpdateZombiesWon)trampolineHook((void*)cutSceneUpdateZombieWonAddr, hookCutSceneUpdateZombiesWon, 7);
}

/*
Callback that fires when seed chooser screen draws
*/
int prevCutsceneTime = 0;
SeedChooserScreenDraw SeedChooserScreenDrawOrigFunc;
int __fastcall hookSeedChooserScreenDraw(int thisPtr, int unused, int a2) {
    int currCutsceneTime = resolveMultiLevelPointer(std::vector<DWORD> { (DWORD)GetModuleHandle(NULL) + 0x329670, 0x868, 0x174, 0x8 });
    if (currCutsceneTime == 4250 && prevCutsceneTime < 4250) {
        std::cout << "Choose seeds" << std::endl;
    }
    prevCutsceneTime = currCutsceneTime;
    return SeedChooserScreenDrawOrigFunc(thisPtr, a2);
}

/*
Hooks into SeedChooserScreen::Draw
*/
void trampHookCutSceneAnimateBoard() {
    DWORD seedChooserScreenDraw = (DWORD)GetModuleHandle(NULL) + 0x8F2F0;
    SeedChooserScreenDrawOrigFunc = (SeedChooserScreenDraw)trampolineHook((void*)seedChooserScreenDraw, hookSeedChooserScreenDraw, 6);
}