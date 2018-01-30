#pragma once
#include "Plugin.h"

DWORD WINAPI WaitForNewMessagesThreadProc(LPVOID lpParam);
void OnNewMessagesFromDatabase();
void HandleMessageFromDatabase(
	long long id, 
	long long at, 
	std::string serverKey, 
	std::string serverTag,
	long long steamId, 
	std::string playerName, 
	std::u16string characterName, 
	std::string tribeName, 
	std::u16string message, 
	int type, 
	int rcon, 
	int icon);