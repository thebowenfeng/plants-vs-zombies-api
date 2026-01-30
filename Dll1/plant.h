#pragma once

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
void addPlant(int row, int col, int seedType);

/*
Same as addPlant() except plant by seedIndex, which is a 0-indexed value of a seed in the seedbank.
*/
void addPlantBySeedIndex(int row, int col, int seedIndex);