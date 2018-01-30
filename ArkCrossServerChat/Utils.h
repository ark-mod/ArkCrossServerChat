#pragma once
#include "API/Ark/Ark.h"
#include <ArkPluginLibrary/ArkPluginLibrary.h>
#include <ArkPluginLibrary/json.hpp>
#include <ArkPluginLibrary/Dependencies/sqlite_modern_cpp/sqlite_modern_cpp.h>

enum class ChatIcon { None, Admin };

void SendRconChatMessageToAll(std::wstring message);
void SendChatMessageToAll(
	const unsigned int senderId,
	const FString &senderName, 
	const FString &senderSteamName, 
	const FString &senderTribeName, 
	const FString &message, 
	UTexture2D *senderIcon);

std::string GetPlayerName(AShooterPlayerController* playerController);
std::wstring GetPlayerCharacterName(AShooterPlayerController* playerController);
std::string GetTribeName(AShooterPlayerController* playerController);

std::string ToUTF8(const std::wstring &s);
std::wstring FromUTF8(const std::string &s);
std::wstring FromUTF16(const std::u16string &s);
std::u16string ToUTF16(const std::wstring &s);