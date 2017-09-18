#pragma once

#include "ArkCrossServerChat.h"

namespace Tools
{
	void SendRconChatMessageToAll(std::wstring message);
	void SendChatMessageToAll(std::wstring senderName, std::wstring senderSteamName, std::wstring senderTribeName, std::wstring message);

	template <typename... Args>
	void SendDirectMessage(AShooterPlayerController* playerController, const wchar_t* msg, Args&&... args)
	{
		size_t size = swprintf(nullptr, 0, msg, std::forward<Args>(args)...) + 1;

		wchar_t* buffer = new wchar_t[size];
		_snwprintf_s(buffer, size, _TRUNCATE, msg, std::forward<Args>(args)...);

		FString cmd(buffer);

		FLinearColor msgColor = {1,1,1,1};
		playerController->ClientServerChatDirectMessage(&cmd, msgColor, false);

		delete[] buffer;
	}

	__int64 GetSteamId(AShooterPlayerController* playerController);
	std::string GetPlayerName(AShooterPlayerController* playerController);
	std::wstring GetPlayerCharacterName(AShooterPlayerController* playerController);
	std::string GetTribeName(AShooterPlayerController* playerController);
	void Log(const std::string& text);
	std::string GetCurrentDir();
	wchar_t* ConvertToWideStr(const std::string& str);
	wchar_t* CopyWString(const std::wstring& str);
	std::string ToUTF8(const std::wstring &s);
	std::wstring FromUTF8(const std::string &s);
	std::wstring FromUTF16(const std::u16string &s);
	std::u16string ToUTF16(const std::wstring &s);
	std::wstring ToWString(const std::u16string &s);
	//std::u16string ToUtf16(const std::string &s);
	//std::string ToUtf16(const std::wstring &s);
}
