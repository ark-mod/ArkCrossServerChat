#include "Hooks.h"

// intercept player chat messages
bool ChatMessageCallback(AShooterPlayerController* _AShooterPlayerController, FString* Message, EChatSendMode::Type Mode, bool spam_check, bool command_executed)
{
	if (spam_check || command_executed) return false;

	auto& plugin = Plugin::Get();
	auto playerControllerEx = reinterpret_cast<ArkExtensions::AShooterPlayerController*>(_AShooterPlayerController);

	// process the message and relay
	long double lastChatMessageTime = 0;
	std::wstring msg;
	if (_AShooterPlayerController && Message && Mode == EChatSendMode::Type::GlobalChat)
	{
		lastChatMessageTime = _AShooterPlayerController->LastChatMessageTimeField();
		msg = std::wstring(ArkApi::Tools::ConvertToWideStr(Message->ToString()));

		auto steamId = static_cast<int64>(ArkApi::GetApiUtils().GetSteamIdFromController(_AShooterPlayerController));
		auto playerName = GetPlayerName(_AShooterPlayerController);
		auto characterName = GetPlayerCharacterName(_AShooterPlayerController);
		auto tribeName = GetTribeName(_AShooterPlayerController);

		auto icon = playerControllerEx->bIsAdminField()()
			&& !_AShooterPlayerController->bSuppressAdminIconField()
			? ChatIcon::Admin
			: ChatIcon::None;

		//todo: maybe support other badges in the future (but when are they used?)
		//auto badgeGroups = uPrimalGameData->BadgeGroupsNameTagField()();
		//if (badgeGroups.Num() > icon) icon = reinterpret_cast<DWORD64>(badgeGroups[icon]);

		try
		{
			*plugin.db << u"INSERT INTO Messages (At,ServerKey,ServerTag,SteamId,PlayerName,CharacterName,TribeName,Message,Type,Icon) "
				"VALUES (?,?,?,?,?,?,?,?,?,?);"
				<< std::chrono::system_clock::now().time_since_epoch().count()
				<< plugin.serverKey
				<< plugin.serverTag
				<< steamId
				<< playerName
				<< ToUTF16(characterName)
				<< tribeName
				<< ToUTF16(msg)
				<< (int)Mode
				<< (int)icon;

			//notify other servers
			PulseEvent(plugin.handle_mre_cluster);
		}
		catch (sqlite::sqlite_exception& e)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, e.what());
		}

		// To prevent duplicate chat messages when other chat-based plugins are in use, return false if we don't intent to modify locally-sent global chat.
		return !plugin.hideServerTagOnLocal;
	}

	return false;
}

// intercept rcon chat messages
void Hook_UShooterCheatManager_ServerChat(UShooterCheatManager* _UShooterCheatManager, FString* MessageText)
{
	auto& plugin = Plugin::Get();
	if (MessageText)
	{
		try
		{
			std::string serverKey = plugin.serverKey;
			*plugin.db << u"INSERT INTO Messages (At,ServerKey,Message,Type,Rcon) VALUES (?,?,?,?,?);"
				<< std::chrono::system_clock::now().time_since_epoch().count()
				<< serverKey
				<< ToUTF16(**MessageText)
				<< -1 << 1;

			//notify other servers
			PulseEvent(plugin.handle_mre_cluster);
		}
		catch (sqlite::sqlite_exception& e)
		{
			Log::GetLog()->error("({} {}) Unexpected DB error {}", __FILE__, __FUNCTION__, e.what());
		}
	}

	UShooterCheatManager_ServerChat_original(_UShooterCheatManager, MessageText);
}