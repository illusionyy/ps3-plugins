# ps3-plugins

Usermode plugins for PS3.

- `game_plugin_bootloader`
  - Usermode plugin to be loaded by Cobra stage2 [(To be merged)](https://github.com/Evilnat/Cobra-PS3/pull/65).
  - Loads plugins from `/dev_hdd0/game_plugins.yml`
- `game_patch`
  - Loaded by bootloader plugin, applies game patches before `start()` in CRT startup.
  - Patches are managed by [Artemis App (WIP)](https://github.com/bucanero/ArtemisPS3/pull/105).
- `game_patch_vsh_data`
  - Writes data about started game from XMB.
  - Includes data such as Title ID and its app version.
  - Shows notification when patches are applied.

# Credits

- [RouLetteVshMenu](https://github.com/TheRouletteBoi/RouLetteVshMenu/tree/e8fb6520e4c1c144e38023186339a2113c29307a) for Memory, Syscalls and Detour functions
- [VirtualShell](https://github.com/TheRouletteBoi/VirtualShell) for VSH exports ([fork](https://github.com/illusionyy/VirtualShell/tree/4e90529daeff471a5f38c321f29f50bbb7539913) used)
- [ps3-ckit](https://github.com/tge-was-taken/ps3-ckit) for `file.c/file.h`
