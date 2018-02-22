#include "MessageHandlers.h"

// thread waiting for message event signaling
DWORD WINAPI WaitForNewMessagesThreadProc(LPVOID lpParam)
{
	UNREFERENCED_PARAMETER(lpParam);

	auto &plugin = Plugin::Get();
	HANDLE events[] = { plugin.handle_mre_cluster, plugin.handle_mre_thread };

	bool exit = false;
	while (!exit)
	{
		auto dwWaitResult = WaitForMultipleObjects(2, events, FALSE, INFINITE);
		switch (dwWaitResult)
		{
		case WAIT_OBJECT_0 + 0:
			if (ArkApi::GetApiUtils().GetWorld())
			{
				std::lock_guard<std::mutex> lock(plugin.new_message_available_mutex);
				plugin.new_message_available = true;
			}
			break;
		case WAIT_OBJECT_0 + 1:
			exit = true;
			break;
		case WAIT_TIMEOUT:
		default:
			Sleep(1000); //todo: ?
		}
	}

	CloseHandle(plugin.handle_mre_cluster);
	CloseHandle(plugin.handle_mre_thread);

	return 0;
}

// check for new messages on game tick 
void MessageTimer(float delta)
{
	auto &plugin = Plugin::Get();
	if (plugin.new_message_available)
	{
		std::lock_guard<std::mutex> lock(plugin.new_message_available_mutex);
		plugin.new_message_available = false;

		OnNewMessagesFromDatabase();
	}
}

// when the database has been updated with new messages
void OnNewMessagesFromDatabase()
{
	auto &plugin = Plugin::Get();
	try
	{
		*plugin.db << "SELECT Id,At,ServerKey,ServerTag,SteamId,PlayerName,CharacterName,TribeName,Message,Type,Rcon,Icon "
			"FROM Messages "
			"WHERE Id > ? "
			"ORDER BY Id ASC;" << plugin.lastRowId >>
			[&](long long id, 
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
				int icon)
		{
			HandleMessageFromDatabase(id, at, serverKey, serverTag, steamId, playerName, characterName, tribeName, message, type, rcon, icon);
			plugin.lastRowId = id;
		};
	}
	catch (sqlite::sqlite_exception& e)
	{
		Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, e.what());
	}
}

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
	int icon)
{
	auto &plugin = Plugin::Get();
	auto isLocal = serverKey.compare(plugin.serverKey) == 0;

	//send chat message to users
	if (rcon == 0)
	{
		auto chatIcon = static_cast<ChatIcon>(icon);
		UTexture2D *iconTexture = nullptr;

		// get chat icon
		if (chatIcon == ChatIcon::Admin)
		{
			auto engine = Globals::GEngine()();
			auto primalglobals = engine->GameSingletonField()();
			auto gamedata = primalglobals->PrimalGameDataOverrideField()();
			if (!gamedata) gamedata = primalglobals->PrimalGameDataField()();

			auto texture = gamedata->NameTagServerAdminField()();
			if (texture) iconTexture = gamedata->NameTagServerAdminField()();
		}

		auto name = FString(FromUTF16(characterName).c_str());
		if (!isLocal || !plugin.hideServerTagOnLocal)
		{
			// format name with server tag
			auto name_with_tag = FString(ArkApi::Tools::ConvertToWideStr(plugin.namePattern).c_str());
			name_with_tag.ReplaceInline(L"{Name}", *name);
			name_with_tag.ReplaceInline(L"{ServerTag}", ArkApi::Tools::ConvertToWideStr(serverTag).c_str());
			name = name_with_tag;
		}

		// get the sender id (to support own name showing in gray for the player)
		unsigned int linkedPlayerDataID;
		if (serverKey.compare(plugin.serverKey) == 0)
		{
			auto player_controller = ArkApi::GetApiUtils().FindPlayerFromSteamId(steamId);
			if (player_controller)
			{
				auto player_character = player_controller->GetPlayerCharacter();
				if (player_character) linkedPlayerDataID = player_character->LinkedPlayerDataIDField()();
			}
		}

		SendChatMessageToAll(
			linkedPlayerDataID,
			name,
			FromUTF8(playerName).c_str(),
			FromUTF8(tribeName).c_str(),
			FromUTF16(message).c_str(),
			iconTexture);
	}
	else if (!isLocal)
	{
		SendRconChatMessageToAll(FromUTF16(message));
	}
}