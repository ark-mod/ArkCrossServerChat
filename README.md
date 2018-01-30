# ARK Server API: Cross Server Chat (Plugin)

## Introduction

Adds cross server chat support to ARK Survival Evolved servers using ARK Server API.

Currently only supports multiple servers on the same machine due to using the Windows PulseEvent Synchronization API (https://msdn.microsoft.com/en-us/library/windows/desktop/ms684914(v=vs.85).aspx).


##  Installation

1. Install ARK Server API (http://arkserverapi.com).
2. Download and extract ImprovedCommands into `ShooterGame\Binaries\Win64\ArkApi\Plugins`.
3. Optionally configure the plugin in `config.json`.


## Configuration

If this plugin is used together with ArkBot (https://github.com/tsebring/ArkBot) configure each server with identical server- and cluster keys as in the ArkBot-configuration.

If not; each server should have a unique server key and the cluster key should be the same for all servers that are connected.

`DatabasePath` must be a valid path and the directory must exist. The plugin will not attempt to create the directory.

`NamePattern` controls the formatting of character names.

 * `{Name} [{ServerTag}]`: Use server tag as a suffix.
 * `[{ServerTag}] {Name}`: Use server tag as a prefix.
 * `{Name}`: Don't show server tags.

 `HideServerTagOnLocal` may be set to `false` if you want server tags to be shown for chat messages coming from the local server (i.e. the one you are playing on).

```json
{
	"ServerKey": "server1",
	"ClusterKey": "cluster1",
	"ServerTag": "MAPNAME",
	"NamePattern": "{Name} [{ServerTag}]",
	"HideServerTagOnLocal": true,
	"DatabasePath": "C:\\ARK Servers\\ArkCrossServerChat.db"
}
```


## Acknowledgements

This plugin is based on Michidu's work on Ark-Server-Plugins and ARK Server API. The basic plumbing code is copied directly from those plugins.


## Links

ARK Server API (http://arkserverapi.com)

ARK Server API [GitHub] (https://github.com/Michidu/ARK-Server-API)

Ark-Server-Plugins [GitHub] (https://github.com/Michidu/Ark-Server-Plugins)