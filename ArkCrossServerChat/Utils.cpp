#include <windows.h>
#include <fstream>
#include <chrono>
#include <random>
#include <codecvt>
#include "Utils.h"

void SendRconChatMessageToAll(std::wstring message)
{
	auto world = ArkApi::GetApiUtils().GetWorld();
	if (!world) return;

	FString msg(message.c_str());

	auto playerControllers = world->PlayerControllerListField()();
	for (auto playerController : playerControllers)
	{
		auto aShooterPC = static_cast<AShooterPlayerController*>(playerController.Get());

		aShooterPC->ClientServerChatDirectMessage(&msg, { 1.0f, 1.0f, 0.0f, 1.0f }, true);
	}
}

void SendChatMessageToAll(
	const unsigned int senderId,
	const FString &senderName, 
	const FString &senderSteamName, 
	const FString &senderTribeName, 
	const FString &message, 
	UTexture2D *senderIcon)
{
	FChatMessage chat_message = FChatMessage();
	chat_message.SenderId = senderId;
	chat_message.SenderName = senderName;
	chat_message.SenderSteamName = senderSteamName;
	chat_message.SenderTribeName = senderTribeName;
	chat_message.Message = message;
	chat_message.SenderIcon = senderIcon;

	auto world = ArkApi::GetApiUtils().GetWorld();
	if (!world) return;

	auto playerControllers = world->PlayerControllerListField()();
	for (auto playerController : playerControllers)
	{
		AShooterPlayerController* aShooterPC = static_cast<AShooterPlayerController*>(playerController.Get());
		aShooterPC->ClientChatMessage(chat_message);
	}
}

std::string GetPlayerName(AShooterPlayerController* playerController)
{
	std::string playerName;

	auto playerState = playerController->PlayerStateField()();
	if (playerState) playerName = ToUTF8(*playerState->PlayerNameField()());

	return playerName;
}

std::wstring GetPlayerCharacterName(AShooterPlayerController* playerController)
{
	auto characterName = ArkApi::GetApiUtils().GetCharacterName(playerController);
		
	return *characterName;
}

std::string GetTribeName(AShooterPlayerController* playerController)
{
	std::string tribeName;

	auto playerState = reinterpret_cast<AShooterPlayerState*>(playerController->PlayerStateField()());
	if (playerState)
	{
		auto tribeData = playerState->MyTribeDataField()();
		tribeName = tribeData.TribeName.ToString();
	}

	return tribeName;
}

std::wstring FromUTF8(const std::string &s)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> conv1;
	std::wstring wstr = conv1.from_bytes(s);
	return wstr;
}

std::string ToUTF8(const std::wstring &s)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> conv1;
	std::string u8str = conv1.to_bytes(s);
	return u8str;
}

std::wstring FromUTF16(const std::u16string &s) {
	return std::wstring(s.begin(), s.end());
}

std::u16string ToUTF16(const std::wstring &s) {
	return std::u16string(s.begin(), s.end());
}