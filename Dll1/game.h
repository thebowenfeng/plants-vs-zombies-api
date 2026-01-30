#pragma once

extern std::vector<BYTE> copyZombiesWonSurvivalDialogStolenBytes;

/*
Gets the current game UI state (i.e which screen is the game currently on)

2 - Seed selection/waiting to start
3 - In game
*/
int getGameUi();

/*
Gets the current game result

0 - none (in progress)
1 - won
2 - lost
*/
int getGameResult();

/*
Start the game when in seed selection. Internally calls SeedChooserScreen::OnStartButton()
Note: Will crash the game if invoked improperly (i.e game isn't meant to start)
*/
void startGame();

/*
Sets a detour on DLL load to copy zombie won survival mode dialog's address that
can later be used to invoke level restart
*/
void copyZombiesWonSurvivalDialogAddr();

/*
Restart a survival level
*/
void restartSurvivalLevel();

/*
Hooks into CutScene::UpdateZombiesWon
*/
void trampHookCutSceneUpdateZombieWon();

/*
Hooks into SeedChooserScreen::Draw
*/
void trampHookCutSceneAnimateBoard();