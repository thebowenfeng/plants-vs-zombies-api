#pragma once
#include <string>

enum CALLBACK_KEY {
	CHOOSE_SEED = 0,
	GAME_OVER = 1
};

struct InvokeCallbackParam {
	CALLBACK_KEY callbackKey;
	std::string payload;
};

int insertCallback(CALLBACK_KEY callbackKey, std::string url);

int removeCallback(CALLBACK_KEY callbackKey, std::string url);

void invokeCallbacks(InvokeCallbackParam* param);
