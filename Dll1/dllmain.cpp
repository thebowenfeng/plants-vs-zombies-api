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
#include "memory.h"
#include "game.h"
#include "plant.h"
#include "seed.h"

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
result - Enum value for the result of a finished game
restart - Restart a finished game
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

/*
Cleans up any non-ephemeral hooks/detours/patches on DLL exit or unload
*/
void cleanup() {
    DWORD copyDialogAddr = (DWORD)GetModuleHandle(NULL) + 0x3FA1A;
    patchBytes((void*)copyDialogAddr, copyZombiesWonSurvivalDialogStolenBytes);

    DWORD cutSceneUpdateZombieWonAddr = (DWORD)GetModuleHandle(NULL) + 0x3F650;
    patchBytes((void*)cutSceneUpdateZombieWonAddr, std::vector<BYTE> { 0x6A, 0xFF, 0x68, 0xF7, 0xA7, 0x6C, 0x00 });

    DWORD seedChooserScreenDraw = (DWORD)GetModuleHandle(NULL) + 0x8F2F0;
    patchBytes((void*)seedChooserScreenDraw, std::vector<BYTE> { 0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF8 });
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

