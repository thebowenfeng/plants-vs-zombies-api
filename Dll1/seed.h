#pragma once

int getSeedBankSize();

int getSeedPacketType(int index);

/*
Internally calls SeedChooserScreen::PickRandomSeeds() that randomly picks seeds and starts the game
*/
void chooseRandomSeeds();

/*
Selects a seed based on seedType. If seed is already selected, this will deselect the seed.
Internally calls SeedChooserScreen::MouseDown() but coerces seed type without using mouse coords.

Arguments:
    - seedType: Enum representing the seed type that is to be selected
*/
void chooseSeed(int seedType);

/*
Get number of seed selected (in bank) during seed selection
*/
int getSeedInBank();