#include "pch.h"
#include <Windows.h>
#include <string.h>
#include <vector>
#include "memory.h"
#include "seed.h"
#include "game.h"
#include <mutex>

typedef int(__fastcall* MouseDownWithPlant)(int clickCount, int sameAsPixelX, int boardPtr, int pixelX, int pixelY);

std::mutex addPlantMutex;

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
        mov[esp + 0x20], eax
        jmp[addPlantColumnNumDetourReturnAddr]
    }
}

int addPlantRowNumDetourValue;
DWORD addPlantRowNumDetourReturnAddr;
void __declspec(naked) patchPlantGridYTranslation() {
    __asm {
        mov eax, [addPlantRowNumDetourValue]
        mov ebx, [esp + 0x1C]
        mov esi, eax
        jmp[addPlantRowNumDetourReturnAddr]
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
        jmp[addPlantSeedTypeReturnAddr]
    }
}

DWORD boardAddPlantSeedTypeReturnAddr;
void __declspec(naked) patchBoardAddPlantSeedType() {
    __asm {
        push[addPlantSeedTypeDetourValue]
        mov eax, [esp + 0x20]
        jmp[boardAddPlantSeedTypeReturnAddr]
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
        mov ecx, [ebp + 0x15C]
        jmp[seedPacketIndexReturnAddr]
    }
}

DWORD mCursorTypeReturnAddr;
void __declspec(naked) patchMCursorType() {
    __asm {
        mov ecx, 1
        cmp ecx, 3
        jmp[mCursorTypeReturnAddr]
    }
}

void __declspec(naked) patchGlobalHasConveyorBeltSeedBank() {
    __asm {
        mov al, 0
        retn
    }
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
int addPlant(int row, int col, int seedType) {
    if (getGameUi() != 3) return 1;

    int seedIndex = -1;
    for (int i = 0; i < getSeedBankSize(); i++) {
        if (getSeedPacketType(i) == seedType) {
            seedIndex = i;
            break;
        }
    }
    if (seedIndex == -1) {
        return 2;
    }

    addPlantMutex.lock();

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

    addPlantMutex.unlock();
    return 0;
}

/*
Same as addPlant() except plant by seedIndex, which is a 0-indexed value of a seed in the seedbank.
*/
int addPlantBySeedIndex(int row, int col, int seedIndex) {
    if (seedIndex >= getSeedBankSize()) {
        return 2;
    }
    return addPlant(row, col, getSeedPacketType(seedIndex));
}