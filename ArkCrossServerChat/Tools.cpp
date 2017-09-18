#include <windows.h>
#include <fstream>
#include <chrono>
#include <random>
#include <codecvt>
#include "Tools.h"

namespace Tools
{
	void SendRconChatMessageToAll(std::wstring message)
	{
		auto world = Ark::GetWorld();
		if (!world) return;

		wchar_t* msg = Tools::CopyWString(message);
		FLinearColor yellow = { 1.0f, 1.0f, 0.0f, 1.0f };

		auto playerControllers = world->GetPlayerControllerListField();
		for (uint32_t i = 0; i < playerControllers.Num(); ++i)
		{
			auto playerController = playerControllers[i];

			AShooterPlayerController* aShooterPC = static_cast<AShooterPlayerController*>(playerController.Get());

			aShooterPC->ClientServerChatDirectMessage(&FString(msg), yellow, true);
		}

		delete[] msg;
	}

	void SendChatMessageToAll(std::wstring senderName, std::wstring senderSteamName, std::wstring senderTribeName, std::wstring message)
	{
		FChatMessage* chatMessage = static_cast<FChatMessage*>(malloc(sizeof(FChatMessage)));
		if (chatMessage)
		{
			wchar_t* sn = Tools::CopyWString(senderName);
			wchar_t* ssn = Tools::CopyWString(senderSteamName);
			wchar_t* stn = Tools::CopyWString(senderTribeName);
			wchar_t* msg = Tools::CopyWString(message);

			chatMessage->SenderName = FString(sn);
			chatMessage->SenderSteamName = FString(ssn);
			chatMessage->SenderTribeName = FString(stn);
			chatMessage->SenderId = 0;
			chatMessage->Message = FString(msg);
			chatMessage->Receiver = L"";
			chatMessage->SenderTeamIndex = 0;
			chatMessage->ReceivedTime = -1;
			chatMessage->SendMode = EChatSendMode::GlobalChat;
			chatMessage->RadioFrequency = 0;
			chatMessage->ChatType = EChatType::GlobalChat;
			chatMessage->SenderIcon = 0;
			chatMessage->UserId = L"";

			void* mem = malloc(sizeof(FChatMessage));
			FChatMessage* chat = new(mem) FChatMessage(chatMessage);

			auto world = Ark::GetWorld();
			if (!world) return;

			auto playerControllers = world->GetPlayerControllerListField();
			for (uint32_t i = 0; i < playerControllers.Num(); ++i)
			{
				auto playerController = playerControllers[i];

				AShooterPlayerController* aShooterPC = static_cast<AShooterPlayerController*>(playerController.Get());

				aShooterPC->ClientChatMessage(chat);
			}

			chat->~FChatMessage();
			free(mem);

			free(chatMessage);

			delete[] sn;
			delete[] ssn;
			delete[] stn;
			delete[] msg;
		}
	}

	__int64 GetSteamId(AShooterPlayerController* playerController)
	{
		__int64 steamId = 0;

		APlayerState* playerState = playerController->GetPlayerStateField();
		if (playerState)
		{
			steamId = playerState->GetUniqueIdField()->UniqueNetId->GetUniqueNetIdField();
		}

		return steamId;
	}

	std::string GetPlayerName(AShooterPlayerController* playerController)
	{
		std::string playerName;

		APlayerState* playerState = playerController->GetPlayerStateField();
		if (playerState)
		{
			playerName = ToUTF8(*playerState->GetPlayerNameField());
		}

		return playerName;
	}

	std::wstring GetPlayerCharacterName(AShooterPlayerController* playerController)
	{
		FString characterName;
		playerController->GetPlayerCharacterName(&characterName);
		
		return *characterName;
	}

	std::string GetTribeName(AShooterPlayerController* playerController)
	{
		std::string tribeName;

		AShooterPlayerState* playerState = reinterpret_cast<AShooterPlayerState*>(playerController->GetPlayerStateField());
		if (playerState)
		{
			FTribeData* tribeData = playerState->GetMyTribeDataField();
			if (tribeData) {
				tribeName = ToUTF8(*tribeData->GetTribeNameField());
			}
		}

		return tribeName;
	}

	void Log(const std::string& text)
	{
		static std::ofstream file(GetCurrentDir() + "/BeyondApi/Plugins/ArkCrossServerChat/logs.txt", std::ios_base::app);

		auto time = std::chrono::system_clock::now();
		std::time_t tTime = std::chrono::system_clock::to_time_t(time);

		char buffer[256];
		ctime_s(buffer, sizeof(buffer), &tTime);

		buffer[strlen(buffer) - 1] = '\0';

		std::string timeStr = buffer;

		std::string finalText = timeStr + ": " + text + "\n";

		file << finalText;

		file.flush();
	}

	std::string GetCurrentDir()
	{
		char buffer[MAX_PATH];
		GetModuleFileNameA(nullptr, buffer, MAX_PATH);
		std::string::size_type pos = std::string(buffer).find_last_of("\\/");

		return std::string(buffer).substr(0, pos);
	}

	wchar_t* ConvertToWideStr(const std::string& str)
	{
		size_t newsize = str.size() + 1;

		wchar_t* wcstring = new wchar_t[newsize];

		size_t convertedChars = 0;
		mbstowcs_s(&convertedChars, wcstring, newsize, str.c_str(), _TRUNCATE);

		return wcstring;
	}

	wchar_t* CopyWString(const std::wstring& str) {
		wchar_t* buffer = new wchar_t[str.size() + 1]();
		str.copy(buffer, str.size());

		return buffer;
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

	std::wstring ToWString(const std::u16string &s) {
		return std::wstring(s.begin(), s.end());
	}

	/*std::string ToUtf16(const std::wstring &s) {
		std::wstring_convert<std::codecvt_utf16<wchar_t>> conv2;
		std::string u16str = conv2.to_bytes(s);
		return u16str;
	}*/

	/*std::u16string ToUtf16(const std::string &s)
	{
		std::wstring_convert<std::codecvt_utf8<int16_t>, int16_t> convert;
		auto asInt = convert.from_bytes(s);
		return std::u16string(reinterpret_cast<char16_t const *>(asInt.data()), asInt.length());
	}*/
}
