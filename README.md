<div align="center">
<picture>
  <source media="(prefers-color-scheme: dark)" width="400" srcset="https://github.com/Arkensor/EnfusionPersistenceFramework/assets/8494013/f769fd10-1d40-4999-ae41-c1248d556d38">
  <img alt="Everon Life" width="400" src="https://github.com/Arkensor/EnfusionPersistenceFramework/assets/8494013/4875f7aa-e6af-4fb9-be20-4ed1c1790aa3">
</picture>

[![Releases](https://img.shields.io/github/v/release/Arkensor/EnfusionPersistenceFramework?style=flat-square)](https://github.com/Arkensor/EnfusionDatabaseFramework/releases)
[![Arma Reforger Workshop](https://img.shields.io/badge/Workshop-5D6EBC81EB1842EF-blue?style=flat-square)](https://reforger.armaplatform.com/workshop/5D6EBC81EB1842EF)
[![License MIT](https://img.shields.io/badge/License-MIT-green?style=flat-square)](https://opensource.org/licenses/MIT)
</div>

# Enfusion Persistence Framework

> **Warning**
> This framework is still in **BETA**. Until version 1.0.0 there is no backward compatibility guarantee! Expect some bugs/performance issues and function signature updates until then. Feedback via [issue](https://github.com/Arkensor/EnfusionPersistenceFramework/issues) or [discussion](https://github.com/Arkensor/EnfusionPersistenceFramework/discussions) is welcome.

**A framework to save and load the world state in Enfusion engine powered games.**  
This framework is built on top of the [Enfusion Database Framework](https://github.com/Arkensor/EnfusionDatabaseFramework). The persistence logic is responsible for collecting and translating the world state (relevant game entities and scripted instances) into DB entities which the DB framework can then store. On server restart all changes from the last session are requested from the DB and the save-data is applied back onto the world so players can continue to play.

### Why not use the vanilla save-game system?
The built-in save-game system in Arma Reforger has its purpose. Just dumping everything in e.g. a co-op mission, is a quick and easy way to continue it at a later point. But there are downsides: 
- Large file sizes, because for things like [`BaseSerializationSaveContext.WriteGameEntity()`](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/Scripts/Game/generated/Serialization/BaseSerializationSaveContext.c;22) EVERYTHING is written, even if the values are all default/hard-baked into the prefab. This becomes especially troublesome because there are limits to how much the BI backend servers will store. Afaik `512kb` for cloud saves and `1MB` for local saves. That is too little if a gamemode has data for maybe 10.000 players who connected at one point.
- No versioning/migration of save-data. If a component uses an int for a variable and a modder later decides they actually need a float or maybe array of ints there is no easy way to migrate this.
- No way to identify objects across restarts. [`IEntity.GetID()`](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/scripts/Core/generated/Entities/IEntity.c;157) / [`RplId`](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/scripts/Core/proto/EnNetwork.c;45) property is only reliable for the current session. Map entity id's can change too.
- The [`SCR_SaveLoadComponent`](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/Scripts/Game/GameMode/SaveLoad/SCR_SaveLoadComponent.c;12) while proving some nice reusable logic for save-games, unfortunately, uses the atrocious [`SCR_JsonApiStruct`](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/Scripts/Game/GameMode/SaveLoad/SCR_JsonApiStruct.c;5) base class which makes it rather labor intensive to setup save structs even if all you need is a simple copy of the component script variables.
- No apparent access to the save-data for administration purposes. Everything is stored on the BI servers, and I have yet to find any way - without some scripted downloader in workbench - to access and managed what it stored on them.

And probably some more ...  
They can adressed, but why solve these for every project? If you don't see yourself running into these issues, then maybe the vanilla systems will work out just fine for you - in which case: go for it - but anybody finding themselves not amused by their useability or future-proofing, this framework is what you are looking for instead!

## 🚀 Features
- 🚧 Many game mechanics persisted by default
  - ✅ Entity transform, hitzones, inventories and attachment slots
  - ✅ Vehicle passangers, engine status, fuel, lights and turrets
  - ✅ Weapon chambering status and magazine (with ammo count, also on UGL's)
  - ✅ Character stance, weapon and gadget raise status
  - ✅ Player quickbar
  - ✅ Date/Time and weather
  - ✅ Gargabe manager lifetime
  - 🚧 Factions
  - 🚧 Smoke granades
  - 🚧 Explosives (Mines)
  - 🚧 AI Grouping and mind state/tasks
  - 🚧 Scenario framework
- ✅ Auto- and shutdown save (HUGE benefit to game modes that often crash. People can continue to play!)
- ✅ Convert a gamemode into a persistent one as an afterthought without extensive re-writes.
- ✅ Easily extract and apply the essential information needed to restore the state instead of "dumb" full copies.
  - ✅ Entities
  - ✅ Entity components
  - ✅ Scripted class instances
- ✅ Persistent GUID's to identify anything across restarts
  - ✅ Even works with baked map objects. If they are named the map objects can also be moved and still corretly be identified back onloading. Includes a proxy utlity to name objects form locked parent scenes!
  - ✅ Multiple hives in the same DB possible without conflicts but cross player persistence.
- ✅ Built in versioning support for everything to allow for easy migrations.
- ✅ Many convenience functions to e.g. load and spawn back an entity or scripted instance by its GUID
- 🚧 Vanilla loadout and spawn screen handling on re-connect
- 🚧 Vanilla game-mode support
  - 🚧 Combat Ops
  - 🚧 Conflict
  - 🚧 Game Master

## 📖 Documentation
Detailed information on the individual classes and best practices can be found [here](docs/index.md).

## ⚡ Quick start
Depending on the use-case the effort to be put into the setup can vary. But for a simple custom game-mode where each player has one character they spawn back with the setup is quick. The following setup can be seen implemented in the unit test project as an example. Open the [`TestWorld`](https://enfusionengine.com/api/redirect?to=enfusion://WorldEditor/Worlds/TestWorld/TestWorld.ent) to have a look.

1. Add the [`EPF_PersistenceManagerComponent`](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/Scripts/Game/EPF_PersistenceManagerComponent.c;26) to your [`SCR_BaseGameMode`](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/Scripts/Game/GameMode/SCR_BaseGameMode.c;105) inherited game-mode entity.
2. Go into the component attributes and add a `Connection Info` instance to something like [`EDF_JsonFileDbConnectionInfo`](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/Scripts/Game/Drivers/LocalFile/EDF_JsonFileDbDriver.c;2) and enter a `Database Name` like `MyGamemodeDatabase`. For more info on the available database types please [read this](https://github.com/Arkensor/EnfusionDatabaseFramework/blob/armareforger/docs/drivers/index.md).
3. The persistence system is now up and running but player respawns need to be handled. In game modes where the group and loadout selection is used one needs to decide how they want things to behave - if on re-connect the player spawns automatically and only chooses the loadout on first join or death - or if they always have the option to choose a new one. This [integreation](docs/respawn-system-integration.md) into the respawn system that suits the need of the game mode is by far the most complicated aspect. For help consider opening a [discussion](https://github.com/Arkensor/EnfusionPersistenceFramework/discussions). In this simple example players spawn with exactly one loadout configured through the `Default Prefab` attribute on the [`EPF_BasicRespawnSystemComponent`](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/Scripts/Game/RespawnSystem/EPF_BasicRespawnSystemComponent.c;8) which has to be added to the game mode.
4. With this setup the play mode can be entered in the workbench for testing, the world can be changed, restarted and everything should be loaded back like it was before the restart.
