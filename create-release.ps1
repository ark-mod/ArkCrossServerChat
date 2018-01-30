$path = "x64\Release\ArkCrossServerChat"

New-Item -ItemType Directory -Force -Path $path
Copy-Item x64\Release\ArkCrossServerChat.dll $path
Copy-Item x64\Release\ArkCrossServerChat.pdb $path
Copy-Item Configs\Config.json $path
Copy-Item Configs\PluginInfo.json $path
Copy-Item Configs\PdbConfig.json $path

Compress-Archive -Path $path -CompressionLevel Optimal -DestinationPath ArkCrossServerChat-2.00.zip

Remove-Item $path -Force -Recurse -ErrorAction SilentlyContinue