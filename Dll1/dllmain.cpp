// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <iostream>
#include <psapi.h>
#include <winnt.h>
#include <winternl.h>
#include <TlHelp32.h>
#include <Windows.h>
#include <string.h>
#include <vector>
#include <string>
#include <sstream>

typedef int (__fastcall *MouseDownWithPlant)(int clickCount, int sameAsPixelX, int boardPtr, int pixelX, int pixelY);
typedef int(__stdcall* PickRandomSeeds)(DWORD* seedChooserPtr);
typedef int(__thiscall* SeedChooserMouseDown)(DWORD* thisPtr, int mouseX, int mouseY, int clickCount);
typedef int(__thiscall* GameOverDialogButtonDepress)(DWORD* thisPtr, int theId);
typedef void(__stdcall* CutSceneUpdateZombiesWon)(DWORD* thisPtr);
typedef int(__thiscall* SeedChooserScreenDraw)(int thisPtr, int a2);

std::string helpMessage = R"(
Available commands:
help - Display this help message
exit - Exit the program
plant <rowNumber> <columnNumber> <seedBankIndex> - Add a plant with validation. Only works during a level.
plantbytype <rowNumber> <columnNumber> <seedType> - Same as plant but use seedType instead of seed bank index.
seedbank - Prints out all plant types in the seed bank currently.
randomseeds - Selects X random seeds for the seed bank during seed selection stage.
chooseseed <seedType> - Choose a specific plant during seed selection stage. If the plant is already added, it will remove the plant.
getseedbankcount - Gets the number of plants currently selected during seed selection stage.
start - Starts the game in seed selection stage. Do not use this when not in pre-level states (e.g seed selection)
gameui - Enum value for the current game state (2 for pre-level/seed selection, 3 for in game).
)";

std::vector<std::string> readArgument() {
    std::vector<std::string> v;
    std::string raw;
    std::string token;
    if (std::getline(std::cin, raw)) {
        std::istringstream iss(raw);
        std::string word;

        while (iss >> word) {
            v.push_back(word);
        }
    }
    return v;
}

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

/**
* This is a hacky way to call Board::MouseDownWithPlant but with converted column num and row num
* Board::AddPlant does not perform validations, but MouseDownWithPlant does, but it only accepts real cursor x y position.
**/
int addPlantColumnNumDetourValue;
DWORD addPlantColumnNumDetourReturnAddr;
void __declspec(naked) patchPlantGridXTranslation() {
    __asm {
        mov eax, [addPlantColumnNumDetourValue]
        push esi
        mov [esp+0x20], eax
        jmp [addPlantColumnNumDetourReturnAddr]
    }
}

int addPlantRowNumDetourValue;
DWORD addPlantRowNumDetourReturnAddr;
void __declspec(naked) patchPlantGridYTranslation() {
    __asm {
        mov eax, [addPlantRowNumDetourValue]
        mov ebx, [esp+0x1C]
        mov esi, eax
        jmp [addPlantRowNumDetourReturnAddr]
    }
}

int addPlantSeedTypeDetourValue;
DWORD addPlantSeedTypeReturnAddr;
void __declspec(naked) patchGetCursorSeedType() {
    __asm {
        mov eax, [addPlantSeedTypeDetourValue]
        mov edi, eax
        push ebx
        mov edx, esi
        jmp [addPlantSeedTypeReturnAddr]
    }
}

DWORD boardAddPlantSeedTypeReturnAddr;
void __declspec(naked) patchBoardAddPlantSeedType() {
    __asm {
        push [addPlantSeedTypeDetourValue]
        mov eax, [esp+0x20]
        jmp [boardAddPlantSeedTypeReturnAddr]
    }
}

DWORD plantGetCostReturnAddr;
void __declspec(naked) patchPlantGetCost() {
    __asm {
        mov eax, [addPlantSeedTypeDetourValue]
        mov ecx, dword ptr ds : 0x729670
        jmp[plantGetCostReturnAddr];
    }
}

int seedPacketIndex;
DWORD seedPacketIndexReturnAddr;
void __declspec(naked) patchSeedPacketIndex() {
    __asm {
        mov eax, [seedPacketIndex]
        mov ecx, [ebp+0x15C]
        jmp [seedPacketIndexReturnAddr]
    }
}

DWORD mCursorTypeReturnAddr;
void __declspec(naked) patchMCursorType() {
    __asm {
        mov ecx, 1
        cmp ecx, 3
        jmp [mCursorTypeReturnAddr]
    }
}

void __declspec(naked) patchGlobalHasConveyorBeltSeedBank() {
    __asm {
        mov al, 0
        retn
    }
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

int getSeedBankSize() {
    DWORD seedBankAddr = resolveMultiLevelPointer(std::vector<DWORD> { (DWORD)GetModuleHandle(NULL) + 0x329670, 0x868, 0x15C });
    return *(int*)(seedBankAddr + 0x24);
}

int getSeedPacketType(int index) {
    DWORD seedBankAddr = resolveMultiLevelPointer(std::vector<DWORD> { (DWORD)GetModuleHandle(NULL) + 0x329670, 0x868, 0x15C });
    return *(int*)(seedBankAddr + index * 0x50 + 0x5C);
}

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
Add plant to garden given row number, column number and seed type.
Arguments:
    row - Integer denoting 0 indexed row number
    col - Integer denoting 0 indexed column number
    seedType - Integer representing the SeedType enum

Caveats:
    - Does not perform seed cooldown validation, meaning there is effectively no 
    cooldown. This is due to the fact that cooldown validation is performed
    when player selects a seed and is controlled by the SeedPacket, not Board.
    When MouseDownWithPlant is invoked, it is assumed cooldown is validated.

Internally calls Board::MouseDownWithPlant with a few modification, mainly to coerce
mouse coord to row col translation and skips several checks requiring mCursor.

int __fastcall Board::MouseDownWithPlant(int clickCount, int sameAsPixelX, int boardPtr, int pixelX, int pixelY) {
...
v9 = Board::GetSeedTypeInCursor();
v9 = seedType
...
columnNum = Board::PlantingPixelToGridX(pixelX);
rowNum = Board::PlantingPixelToGridY(v9, boardPtr, pixelX, pixelY);
columnNum = col
rowNum = row
...
if ( !*(_BYTE *)(*(_DWORD *)(boardPtr + 164) + 2356)
    && (*(_DWORD *)(*(_DWORD *)(boardPtr + 336) + 48) == 1 || true)
    && !(unsigned __int8)Board::HasConveyorBeltSeedBank() )
  {
...
}
char __usercall Board::HasConveyorBeltSeedBank@<al>(int a1@<eax>) { return false }
Plant::GetCost() { internalSeedType = seedType ... }
*/
void addPlant(int row, int col, int seedType) {
    if (getGameUi() != 3) return;

    int seedIndex = -1;
    for (int i = 0; i < getSeedBankSize(); i++) {
        if (getSeedPacketType(i) == seedType) {
            seedIndex = i;
            break;
        }
    }
    if (seedIndex == -1) {
        return;
    }

    DWORD colDetourAddr = (DWORD)GetModuleHandle(NULL) + 0x127BA;
    DWORD rowDetourAddr = (DWORD)GetModuleHandle(NULL) + 0x127C9;
    DWORD seedCursorDetourAddr = (DWORD)GetModuleHandle(NULL) + 0x127AA;
    DWORD mCursorTypeDetourAddr = (DWORD)GetModuleHandle(NULL) + 0x1339B;
    DWORD boardAddPlantTypeDetourAddr = (DWORD)GetModuleHandle(NULL) + 0x13456;
    DWORD hasConveyorBeltSeedBankAddr = (DWORD)GetModuleHandle(NULL) + 0x1EC10;
    DWORD mCursorConditionalWithBeltSeedBankCheckAddr = (DWORD)GetModuleHandle(NULL) + 0x1320C;
    DWORD seedWasPlantedAddr = (DWORD)GetModuleHandle(NULL) + 0x1347C;
    DWORD plantGetCostAddr = (DWORD)GetModuleHandle(NULL) + 0x6B5C0;
    DWORD seedPacketIndexAddr = (DWORD)GetModuleHandle(NULL) + 0x13482;

    addPlantColumnNumDetourValue = col;
    addPlantRowNumDetourValue = row;
    addPlantSeedTypeDetourValue = seedType;
    seedPacketIndex = seedIndex;

    addPlantColumnNumDetourReturnAddr = colDetourAddr + 5;
    addPlantRowNumDetourReturnAddr = rowDetourAddr + 6;
    addPlantSeedTypeReturnAddr = seedCursorDetourAddr + 5;
    mCursorTypeReturnAddr = mCursorTypeDetourAddr + 6;
    boardAddPlantSeedTypeReturnAddr = boardAddPlantTypeDetourAddr + 5;
    plantGetCostReturnAddr = plantGetCostAddr + 6;
    seedPacketIndexReturnAddr = seedPacketIndexAddr + 9;
    std::vector<BYTE> patchPlantGridXOrig = detour((void*)colDetourAddr, patchPlantGridXTranslation, 5);
    std::vector<BYTE> patchPlantGridYOrig = detour((void*)rowDetourAddr, patchPlantGridYTranslation, 6);
    std::vector<BYTE> patchGetCursorSeedTypeOrig = detour((void*)seedCursorDetourAddr, patchGetCursorSeedType, 5);
    std::vector<BYTE> patchMCursorTypeOrig = detour((void*)mCursorTypeDetourAddr, patchMCursorType, 6);
    std::vector<BYTE> patchBoardAddPlantSeedTypeOrig = detour((void*)boardAddPlantTypeDetourAddr, patchBoardAddPlantSeedType, 5);
    std::vector<BYTE> patchGlobalConveyorBeltSeedOrig = detour((void*)hasConveyorBeltSeedBankAddr, patchGlobalHasConveyorBeltSeedBank, 6);
    std::vector<BYTE> patchPlantGetCostOrig = detour((void*)plantGetCostAddr, patchPlantGetCost, 6);
    std::vector<BYTE> patchSeedPacketIndexOrig = detour((void*)seedPacketIndexAddr, patchSeedPacketIndex, 9);
    std::vector<BYTE> nopCursorConditionalBeltSeedBankOrig = nopBytes((void*)mCursorConditionalWithBeltSeedBankCheckAddr, 12);

    MouseDownWithPlant MouseDownWithPlantFunc = (MouseDownWithPlant)((DWORD)GetModuleHandle(NULL) + 0x126F0);
    DWORD boardPtrAddr = resolveMultiLevelPointer(std::vector<DWORD> { (DWORD)GetModuleHandle(NULL) + 0x329670, 0x868 });

    MouseDownWithPlantFunc(1, 1, boardPtrAddr, 1, 1);

    patchBytes((void*)colDetourAddr, patchPlantGridXOrig);
    patchBytes((void*)rowDetourAddr, patchPlantGridYOrig);
    patchBytes((void*)seedCursorDetourAddr, patchGetCursorSeedTypeOrig);
    patchBytes((void*)mCursorTypeDetourAddr, patchMCursorTypeOrig);
    patchBytes((void*)boardAddPlantTypeDetourAddr, patchBoardAddPlantSeedTypeOrig);
    patchBytes((void*)hasConveyorBeltSeedBankAddr, patchGlobalConveyorBeltSeedOrig);
    patchBytes((void*)plantGetCostAddr, patchPlantGetCostOrig);
    patchBytes((void*)mCursorConditionalWithBeltSeedBankCheckAddr, nopCursorConditionalBeltSeedBankOrig);
    patchBytes((void*)seedPacketIndexAddr, patchSeedPacketIndexOrig);

    //Maybe proper seed packet handling (i.e wait for seed packet cooldown) is too hard because need to call a mouse pos based function for seedpacket.
}

/*
Internally calls SeedChooserScreen::PickRandomSeeds() that randomly picks seeds and starts the game
*/
void chooseRandomSeeds() {
    if (getGameUi() != 2) return;

    DWORD seedChooserPtr = resolveMultiLevelPointer(std::vector<DWORD> { (DWORD)GetModuleHandle(NULL) + 0x329670, 0x874 });
    PickRandomSeeds PickRandomSeedsFunc = (PickRandomSeeds)((DWORD)GetModuleHandle(NULL) + 0x905B0);
    PickRandomSeedsFunc((DWORD*)seedChooserPtr);
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
        jmp [chooseSeedTypeReturnAddr];
    }
}

/*
Selects a seed based on seedType. If seed is already selected, this will deselect the seed.
Internally calls SeedChooserScreen::MouseDown() but coerces seed type without using mouse coords.

Arguments:
    - seedType: Enum representing the seed type that is to be selected
*/
void chooseSeed(int seedType) {
    if (getGameUi() != 2) return;

    DWORD chooseSeedTypeAddr = (DWORD)GetModuleHandle(NULL) + 0x91752;
    chooseSeedType = seedType;
    chooseSeedTypeReturnAddr = chooseSeedTypeAddr + 5;

    std::vector<BYTE> chooseSeedTypeOrig = detour((void*)chooseSeedTypeAddr, patchChooseSeedType, 5);

    DWORD seedChooserPtr = resolveMultiLevelPointer(std::vector<DWORD> { (DWORD)GetModuleHandle(NULL) + 0x329670, 0x874 });
    SeedChooserMouseDown SeedChooserMouseDownFunc = (SeedChooserMouseDown)((DWORD)GetModuleHandle(NULL) + 0x91350);

    SeedChooserMouseDownFunc((DWORD*)seedChooserPtr, 1, 1, 1);

    patchBytes((void*)chooseSeedTypeAddr, chooseSeedTypeOrig);
}

/*
Get number of seed selected (in bank) during seed selection
*/
int getSeedInBank() {
    return (int)resolveMultiLevelPointer(std::vector<DWORD> { (DWORD)GetModuleHandle(NULL) + 0x329670, 0x874, 0xD3C });
}

/*
Start the game when in seed selection. Internally calls SeedChooserScreen::OnStartButton()
Note: Will crash the game if invoked improperly (i.e game isn't meant to start)
*/
void startGame() {
    if (getGameUi() != 2 || getSeedInBank() != getSeedBankSize()) return;

    DWORD seedChooserOnStartAddr = (DWORD)GetModuleHandle(NULL) + 0x90200;
    DWORD seedChooserPtr = resolveMultiLevelPointer(std::vector<DWORD> { (DWORD)GetModuleHandle(NULL) + 0x329670, 0x874 });

    __asm {
        push esi
        mov esi, [seedChooserPtr]
        call [seedChooserOnStartAddr]
        pop esi
    }
}

/*
Same as addPlant() except plant by seedIndex, which is a 0-indexed value of a seed in the seedbank.
*/
void addPlantBySeedIndex(int row, int col, int seedIndex) {
    if (seedIndex >= getSeedBankSize()) {
        return;
    }
    return addPlant(row, col, getSeedPacketType(seedIndex));
}

DWORD copySurvivalDialogReturnAddr = NULL;
DWORD survivalDialogAddr = NULL;
std::vector<BYTE> copySurvivalDialogStolenBytes;
void __declspec(naked) copySurvivalDialogAddrPatch() {
    __asm {
        add esp, 0x4
        mov DWORD PTR [esp + 0x78], eax
        mov survivalDialogAddr, eax
        jmp [copySurvivalDialogReturnAddr]
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
    copySurvivalDialogStolenBytes = detour((void*)copyDialogAddr, copySurvivalDialogAddrPatch, 7);
}

/*
Restart a survival level
*/
void restartSurvivalLevel() {
    if (getGameResult() != 2) return;

    GameOverDialogButtonDepress GameOverDialogButtonDepressFunc = (GameOverDialogButtonDepress)((DWORD)GetModuleHandle(NULL) + 0x5B720);
    GameOverDialogButtonDepressFunc((DWORD*)(survivalDialogAddr + 0xA0), 1000);
}

/*
Cleans up any non-ephemeral hooks/detours/patches on DLL exit or unload
*/
void cleanup() {
    DWORD copyDialogAddr = (DWORD)GetModuleHandle(NULL) + 0x3FA1A;
    patchBytes((void*)copyDialogAddr, copySurvivalDialogStolenBytes);

    DWORD cutSceneUpdateZombieWonAddr = (DWORD)GetModuleHandle(NULL) + 0x3F650;
    patchBytes((void*)cutSceneUpdateZombieWonAddr, std::vector<BYTE> { 0x6A, 0xFF, 0x68, 0xF7, 0xA7, 0x6C, 0x00 });

    DWORD seedChooserScreenDraw = (DWORD)GetModuleHandle(NULL) + 0x8F2F0;
    patchBytes((void*)seedChooserScreenDraw, std::vector<BYTE> { 0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF8 });
}

/*
Callback that fires when zombies win (only when cutscene is over)
*/
CutSceneUpdateZombiesWon CutSceneUpdateZombiesWonOrigFunc;
void __stdcall hookCutSceneUpdateZombiesWon(DWORD* thisPtr) {
    std::cout << *(int*)((DWORD)thisPtr + 0x8) << std::endl; // print cutscene time
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
SeedChooserScreenDraw SeedChooserScreenDrawOrigFunc;
int __fastcall hookSeedChooserScreenDraw(int thisPtr, int unused, int a2) {
    std::cout << resolveMultiLevelPointer(std::vector<DWORD> { (DWORD)GetModuleHandle(NULL) + 0x329670, 0x868, 0x174, 0x8 }) << std::endl; // print mCutSceneTime
    return SeedChooserScreenDrawOrigFunc(thisPtr, a2);
}

/*
Hooks into SeedChooserScreen::Draw
*/
void trampHookCutSceneAnimateBoard() {
    DWORD seedChooserScreenDraw = (DWORD)GetModuleHandle(NULL) + 0x8F2F0;
    SeedChooserScreenDrawOrigFunc = (SeedChooserScreenDraw)trampolineHook((void*)seedChooserScreenDraw, hookSeedChooserScreenDraw, 6);
}

int parseCommand(std::vector<std::string> command) {
    if (command.size() < 1) {
        std::cout << "Invalid command" << std::endl;
        return 0;
    }

    try {
        if (command[0] == "help") {
            std::cout << helpMessage << std::endl;
        }
        else if (command[0] == "exit") {
            cleanup();
            std::cout << "Exiting..." << std::endl;
            Sleep(1000);
            return 1;
        }
        else if (command[0] == "sleep" && command.size() >= 2) {
            Sleep(std::stoi(command[1]));
            return parseCommand(std::vector<std::string>(command.begin() + 2, command.end()));
        }
        else if (command[0] == "plant" && command.size() >= 4) {
            addPlantBySeedIndex(std::stoi(command[1]), std::stoi(command[2]), std::stoi(command[3]));
        }
        else if (command[0] == "plantbytype" && command.size() >= 4) {
            addPlant(std::stoi(command[1]), std::stoi(command[2]), std::stoi(command[3]));
        }
        else if (command[0] == "seedbank") {
            int numSeeds = getSeedBankSize();
            std::cout << "Number of seeds: " << numSeeds << std::endl;
            std::cout << "Seed type: ";
            for (int i = 0; i < numSeeds; i++) {
                std::cout << getSeedPacketType(i) << " ";
            }
            std::cout << std::endl;
        }
        else if (command[0] == "randomseeds") {
            chooseRandomSeeds();
        }
        else if (command[0] == "chooseseed" && command.size() >= 2) {
            chooseSeed(std::stoi(command[1]));
        }
        else if (command[0] == "getseedbankcount") {
            std::cout << getSeedInBank() << std::endl;
        }
        else if (command[0] == "start") {
            startGame();
        }
        else if (command[0] == "gameui") {
            std::cout << getGameUi() << std::endl;
        }
        else if (command[0] == "result") {
            std::cout << getGameResult() << std::endl;
        }
        else if (command[0] == "restart") {
            restartSurvivalLevel();
        }
        else {
            std::cout << "Unknown command or missing arguments" << std::endl;
        }
    }
    catch (std::exception& e) {
        std::cout << "Exception occured: " << e.what() << std::endl;
    }
    return 0;
}

DWORD WINAPI main(HMODULE hModule) {
    AllocConsole();
    FILE* f;
    FILE* f2;
    freopen_s(&f, "CONOUT$", "w", stdout);
    freopen_s(&f2, "CONIN$", "r", stdin);

    std::cout << "Debugger. Enter \"help\" for more" << std::endl;

    copyZombiesWonSurvivalDialogAddr();
    trampHookCutSceneUpdateZombieWon();
    trampHookCutSceneAnimateBoard();

    while (true) {
        std::cout << ">";
        std::vector<std::string> command = readArgument();

        if (parseCommand(command) == 1) {
            break;
        }
    }

    fclose(f);
    fclose(f2);
    FreeConsole();
    FreeLibraryAndExitThread(hModule, 0);
    return 0;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        CloseHandle(CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)main, hModule, 0, nullptr));
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        cleanup();
        break;
    }
    return TRUE;
}

