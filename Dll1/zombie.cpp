#include "pch.h"
#include "zombie.h"
#include "game.h"
#include "memory.h"

std::mutex addZombieMutex;

int addZombie(int row, int zombieType) {
	if (zombieType > 36 || zombieType == 12 || zombieType == 17 || zombieType == 22 || zombieType == 25) return 1; // Adding these zombies currently will crash the game
	if (getGameUi() != 3) return 2;

	addZombieMutex.lock();

	DWORD base = resolveMultiLevelPointer(std::vector<DWORD> { (DWORD)GetModuleHandle(NULL) + 0x329670, 0x868 });
	DWORD funcAddr = (DWORD)GetModuleHandle(NULL) + 0x10700;

	__asm {
		push [row]
		push [zombieType]
		mov eax, [base]
		call [funcAddr]
	}

	addZombieMutex.unlock();
	return 0;
}