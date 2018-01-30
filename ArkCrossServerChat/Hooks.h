#pragma once
#include "Plugin.h"

DECLARE_HOOK_INLINE(UShooterCheatManager_ServerChat, void, UShooterCheatManager*, FString*);

bool ChatMessageCallback(AShooterPlayerController* _AShooterPlayerController, FString* Message, EChatSendMode::Type Mode, bool spam_check, bool command_executed);