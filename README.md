# ARK Beyond API: Cross Server Chat (Plugin)

## Introduction

Adds cross server chat support to ARK Survival Evolved servers using ARK Beyond API. Currently only supports multiple servers on the same machine due to using the Windows PulseEvent Synchronization API (https://msdn.microsoft.com/en-us/library/windows/desktop/ms684914(v=vs.85).aspx).

## Configuration

If this plugin is used together with ArkBot (https://github.com/tsebring/ArkBot) configure each server with identical server- and cluster keys as in the ArkBot-configuration.

If not; each server should have a unique server key and the cluster key should be the same for all servers that are connected.

`DatabasePath` must be a valid path and the directory must exist. The plugin will not attempt to create the directory.

```json
{
	"ServerKey": "server1",
	"ClusterKey": "cluster1",
	"DatabasePath":  "C:\\ARK Servers\\ArkCrossServerChat.db"
}
```

## Acknowledgements

This plugin is based on Michidu's work on Ark-Server-Plugins and ARK Beyond API. The basic plumbing code is copied directly from those plugins.

## Links

My ARK Beyond API Fork (https://github.com/tsebring/ARK-Server-Beyond-API)

ARK Beyond API by Michidu (https://github.com/Michidu/ARK-Server-Beyond-API)

Ark-Server-Plugins (https://github.com/Michidu/Ark-Server-Plugins)