#include <fstream>
#include <chrono>
#include "ArkCrossServerChat.h"
#include "Tools.h"

#pragma comment(lib, "ArkApi.lib")

nlohmann::json json;
sqlite::database* db;
HANDLE ghManualResetEvent;
HANDLE ghThread;
bool bExit = false;
long long lastRowId = 0;
wchar_t* manualResetEventName;
std::chrono::time_point<std::chrono::system_clock> NextCleanupTime;

namespace
{
	DECLARE_HOOK(AShooterPlayerController_ServerSendChatMessage_Impl, void, AShooterPlayerController*, FString*, EChatSendMode::Type);
	DECLARE_HOOK(UShooterCheatManager_ServerChat, void, UShooterCheatManager*, FString*);

	void CleanupTimer();
	void LoadConfig();
	void Update();
	DWORD WINAPI ThreadProc(LPVOID lpParam);

	void Init()
	{
		if (sqlite3_threadsafe() == 0) {
			Tools::Log("sqlite is not threadsafe");
			throw;
		}

		LoadConfig();
		std::string serverKey = json["ServerKey"];
		if (serverKey.empty()) {
			Tools::Log("ServerKey must be set in config.json");
			throw;
		}

		std::string clusterKey = json["ClusterKey"];
		if (clusterKey.empty()) {
			Tools::Log("ClusterKey must be set in config.json");
			throw;
		}
		std::wstring tmp = std::wstring(L"ArkCrossServerChat_").append(Tools::FromUTF8(clusterKey));
		manualResetEventName = Tools::CopyWString(tmp);

		std::string databasePath = json["DatabasePath"];
		if (databasePath.empty()) {
			Tools::Log("DatabasePath must be set in config.json");
			throw;
		}
		//todo: mkdir here
		db = new sqlite::database(databasePath);

		*db << u"create table if not exists Messages ("
			"Id integer primary key autoincrement not null,"
			"At integer default 0,"
			"ServerKey text default '',"
			"SteamId integer default 0,"
			"PlayerName text default '',"
			"CharacterName text default '',"
			"TribeName text default '',"
			"Message text default '',"
			"Type integer default 0,"
			"Rcon integer default 0"
			");";

		//todo: would be better to do this once the server is started and world is up and running
		*db << "SELECT Id FROM Messages ORDER BY Id DESC LIMIT 1;" >> [&](long long id) {
			lastRowId = id;
		};

		ghManualResetEvent = CreateEvent(NULL, TRUE, FALSE, manualResetEventName);

		if (ghManualResetEvent == NULL) {
			Tools::Log("failed to create sync event");
			throw;
		}

		DWORD dwThreadID;
		ghThread = CreateThread(NULL, 0, ThreadProc, NULL, 0, &dwThreadID);

		if (ghThread == NULL)
		{
			Tools::Log("failed to create thread");
			throw;
		}

		NextCleanupTime = std::chrono::system_clock::now() + std::chrono::seconds(300); //init with cleanup in 5 min

		Ark::SetHook("AShooterPlayerController", "ServerSendChatMessage_Implementation", &Hook_AShooterPlayerController_ServerSendChatMessage_Impl, reinterpret_cast<LPVOID*>(&AShooterPlayerController_ServerSendChatMessage_Impl_original));
		Ark::SetHook("UShooterCheatManager", "ServerChat", &Hook_UShooterCheatManager_ServerChat, reinterpret_cast<LPVOID*>(&UShooterCheatManager_ServerChat_original));

		Ark::AddOnTimerCallback(&CleanupTimer);
	}

	void CleanupTimer()
	{
		auto now = std::chrono::system_clock::now();
		auto diff = std::chrono::duration_cast<std::chrono::seconds>(NextCleanupTime - now);

		if (diff.count() <= 0)
		{
			NextCleanupTime = now + std::chrono::seconds(3600);

			try
			{
				*db << u"DELETE FROM Messages WHERE At <= ?;" << (std::chrono::system_clock::now() - std::chrono::seconds(3600 * 24 * 7)).time_since_epoch().count();
			}
			catch (sqlite::sqlite_exception& e)
			{
				std::cout << "Unexpected DB error " << e.what() << std::endl;
			}
		}
	}

	void LoadConfig()
	{
		std::ifstream file(Tools::GetCurrentDir() + "/BeyondApi/Plugins/ArkCrossServerChat/config.json");
		if (!file.is_open())
		{
			Tools::Log("Could not open file config.json");
			throw;
		}

		file >> json;
		file.close();
	}

	DWORD WINAPI ThreadProc(LPVOID lpParam)
	{
		UNREFERENCED_PARAMETER(lpParam);

		while (!bExit) {
			//todo: we could use a timeout here
			DWORD dwWaitResult = WaitForSingleObject(ghManualResetEvent, INFINITE);

			if (bExit) break;

			
			switch (dwWaitResult)
			{
			case WAIT_OBJECT_0:
			case WAIT_TIMEOUT:
				if (Ark::GetWorld()) {
					Update();
				}
				break;
			default:
				Sleep(1000); //todo: ?
			}
		}

		return 0;
	}

	void Update()
	{
		try
		{
			std::string serverKey = json["ServerKey"];
			*db << "SELECT Id,At,ServerKey,SteamId,PlayerName,CharacterName,TribeName,Message,Type,Rcon FROM Messages WHERE Id > ? ORDER BY Id ASC;" << lastRowId >>
				[&](long long id, long long at, std::string msgServerKey, long long steamId, std::string playerName, std::u16string characterName, std::string tribeName, std::u16string message, int type, int rcon) {
				if (msgServerKey.compare(serverKey) != 0) {
					//send chat message to users
					if (rcon == 0) {
						Tools::SendChatMessageToAll(Tools::FromUTF16(characterName), Tools::FromUTF8(playerName), Tools::FromUTF8(tribeName), Tools::FromUTF16(message));
					}
					else {
						Tools::SendRconChatMessageToAll(Tools::FromUTF16(message));
					}
				}

				lastRowId = id;
			};
		}
		catch (sqlite::sqlite_exception& e)
		{
			std::cout << "Unexpected DB error " << e.what() << std::endl;
		}
	}

	void _cdecl Hook_AShooterPlayerController_ServerSendChatMessage_Impl(AShooterPlayerController* _AShooterPlayerController, FString* Message, EChatSendMode::Type Mode)
	{
		long double lastChatMessageTime = 0;
		wchar_t* msg = NULL;
		if (_AShooterPlayerController && Message && Mode == EChatSendMode::Type::GlobalChat) { //todo: for now only relay global chat
			lastChatMessageTime = _AShooterPlayerController->GetLastChatMessageTimeField();
			msg = Tools::CopyWString(std::wstring(**Message));
		}

		AShooterPlayerController_ServerSendChatMessage_Impl_original(_AShooterPlayerController, Message, Mode);

		if (_AShooterPlayerController && Message && Mode == EChatSendMode::Type::GlobalChat) { //todo: for now only relay global chat
			if (_AShooterPlayerController->GetLastChatMessageTimeField() <= lastChatMessageTime) return; //skip if the message was not sent (due to spam filter etc.)

			__int64 steamId = Tools::GetSteamId(_AShooterPlayerController);
			std::string playerName = Tools::GetPlayerName(_AShooterPlayerController);
			std::wstring characterName = Tools::GetPlayerCharacterName(_AShooterPlayerController);
			std::string tribeName = Tools::GetTribeName(_AShooterPlayerController);

			//checking for admin badge is tricky (going by manual offset means no automatic updates)
			//[admin] ([_AShooterPlayerController+0x0930] & 4 != 0)
			//bSuppressAdminIcon (0x0CD6)
			//Another way: _AShooterPlayerController->GetPlayerCharacter, AShooterCharacter->bIsServerAdmin

			//markup in the message is replaced if the player is not an admin
			/*while (FString::ReplaceInline((FString *)&v106, L"  ", L" ", IgnoreCase))
			while (FString::ReplaceInline((FString *)&v106, L"<img", &word_142316594, IgnoreCase))
			while (FString::ReplaceInline((FString *)&v106, L"< img", &word_142316484, IgnoreCase))
			while (FString::ReplaceInline((FString *)&v106, L"Chat.Image", &word_1423164CC, IgnoreCase))
			while (FString::ReplaceInline((FString *)&v106, L"src=", &word_14231661C, IgnoreCase))
			while (FString::ReplaceInline((FString *)&v106, L"src =", &word_142316884, IgnoreCase))
			while (FString::ReplaceInline((FString *)&v106, L"path=", &word_1423165BC, IgnoreCase))
			while (FString::ReplaceInline((FString *)&v106, L"\n", &word_1423165FC, IgnoreCase))
			while (FString::ReplaceInline((FString *)&v106, L"\\n", &word_1423169BC, IgnoreCase))
			while (FString::ReplaceInline((FString *)&v106, L"\\n", &word_1423169F4, IgnoreCase))
			while (FString::ReplaceInline((FString *)&v106, L"<RichColor", &word_1423168CC, IgnoreCase))
			while (FString::ReplaceInline((FString *)&v106, L"< RichColor", &word_142316934, IgnoreCase))*/

			try
			{
				std::string serverKey = json["ServerKey"];
				*db << u"INSERT INTO Messages (At,ServerKey,SteamId,PlayerName,CharacterName,TribeName,Message,Type) VALUES (?,?,?,?,?,?,?,?);"
					<< std::chrono::system_clock::now().time_since_epoch().count() << serverKey << steamId << playerName << Tools::ToUTF16(characterName) << tribeName << Tools::ToUTF16(msg) << (int)Mode;

				//notify other servers
				PulseEvent(ghManualResetEvent);
			}
			catch (sqlite::sqlite_exception& e)
			{
				std::cout << "ServerSendChatMessage_Impl() Unexpected DB error " << e.what() << std::endl;
			}
		}

		if (msg) {
			delete[] msg;
		}
	}

	void _cdecl Hook_UShooterCheatManager_ServerChat(UShooterCheatManager* _UShooterCheatManager, FString* MessageText)
	{
		if (MessageText) {
			try
			{
				std::string serverKey = json["ServerKey"];
				*db << u"INSERT INTO Messages (At,ServerKey,Message,Type,Rcon) VALUES (?,?,?,?,?);"
					<< std::chrono::system_clock::now().time_since_epoch().count() << serverKey << Tools::ToUTF16(**MessageText) << -1 << 1;

				//notify other servers
				PulseEvent(ghManualResetEvent);
			}
			catch (sqlite::sqlite_exception& e)
			{
				std::cout << "UShooterCheatManager_ServerChat() Unexpected DB error " << e.what() << std::endl;
			}
		}

		UShooterCheatManager_ServerChat_original(_UShooterCheatManager, MessageText);
	}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		Init();
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		bExit = true;

		CloseHandle(ghManualResetEvent);
		//CloseHandle(ghThread);

		if (db) {
			delete db;
			db = NULL;
		}
		if (manualResetEventName) {
			delete[] manualResetEventName;
		}
		break;
	}
	return TRUE;
}
