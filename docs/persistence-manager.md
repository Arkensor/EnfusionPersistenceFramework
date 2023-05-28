# [`EPF_PersistenceManager`](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/Scripts/Game/EPF_PersistenceManager.c;10)
The persistence manager is the central coordinator for the entire persistence system. It keeps track of all persistent entities and scripted states to handle the `Autosave`, `Shutdownsave`, `Root entity collection`, and `find instances` by their persistent id. The manager is a singleton class that gets configured through the [`EPF_PersistenceManagerComponent`](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/Scripts/Game/EPF_PersistenceManagerComponent.c;2) on the `SCR_BaseGameMode`.

To get the singleton instance call [`EPF_PersistenceManager.GetInstance()`](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/Scripts/Game/EPF_PersistenceManager.c;62). Consider also using the static [`EPF_PersistenceManager.IsPersistenceMaster()`](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/Scripts/Game/EPF_PersistenceManager.c;49) utility function check if the current machine is the one responsible for dealing with persistence.

## Settings
- The `Update Rate` accumulates the game ticks to reduce the performance wasted on checking for current tasks. Only change the tick rate of the persistence manager if you have a good understanding of what you are doing. Too high or too low values can cause performance degradation.
- The `Connection Info` attribute is used for selecting which database is being connected to for loading and saving. For information on the different selectable types please [consult this](https://github.com/Arkensor/EnfusionDatabaseFramework/blob/armareforger/docs/drivers/index.md). Can be overridden via CLI argument using `-ConnectionString=...` so you do not hard code production connection string into the mod.

## Autosave
The manager automatically saves all tracked instances regardless of how recently they might have been manually saved. This is to increase the consistency of the database after an autosave is completed. A server that crashes shortly after and auto-save should ideally let players pick up their gameplay again with only a few minutes lost.
- `Enable Autosave` controls if the auto-save is active or not
- `Autosave Interval` is the time between auto-saves in seconds. Default is 10 minutes = `600` seconds.
- `Autosave Iterations` controls how many entities are processed per manager tick. This helps to smooth out the CPU-heavy tasks of creating all the save-data and sending them to the database. Reduce this value if you notice lag spikes during the auto-save period. The total amount of instances processed per second is `(1 / <Update Rate>) * <Autosave Iterations>`

### Triggering the auto-save
The auto-save can be triggered at any time manually by calling [`EPF_PersistenceManager.AutoSave()`](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/Scripts/Game/EPF_PersistenceManager.c;207). This resets the countdown until the next regular auto-save. If the auto-save is already ongoing this has no effect. For testing the [`EPF_TriggerSaveAction`](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/Scripts/Game/EPF_TriggerSaveAction.c;1) can be used to trigger saves from a user action.

## Shutdownsave
If the server does a **controlled** shuts down by e.g. CTRL+C or close signal to process or from within script via [`GetGame().RequestClose()`](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/scripts/GameLib/generated/Game.c;66) the server automatically saves everything and blocks the termination until the changes could be sent to the DB. The operating system *might* kill the server if this takes too long. For the workbench play mode hitting escape will count as a controlled shutdown.

## Find by persistent id
The manager offers [`FindEntityByPersistentId()`](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/Scripts/Game/EPF_PersistenceManager.c;174), [`FindPersistenceComponentById()`](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/Scripts/Game/EPF_PersistenceManager.c;184) and  [`FindScriptedStateByPersistentId()`](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/Scripts/Game/EPF_PersistenceManager.c;196). These can be used at any time after the entity or scripted state has completed their `EOnInit` or `Constructor`. While the lookup is using a hashmap consider not calling this every frame. Cache results whenever they are needed frequently.

## Root entity collection
To be able to spawn back any dynamic world entities and remove and deleted map objects after restart the persistence manager maintains a [`EPF_PersistentRootEntityCollection`](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/Scripts/Game/EPF_PersistentRootEntityCollection.c;2). Only entities which have `Self Spawn` enabled in their persistence component will be accounted for.
Normally there is never a need to interact with this collection manually. If an entity like e.g. a dead body should be spawned back automatically even though the character prefab has `Self Spawn`, one can call [`EPF_PersistenceComponent.ForceSelfSpawn()`](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/Scripts/Game/EPF_PersistenceComponent.c;636) which registers the id to be included in the dynamic spawns.

## Awaiting the world load
To wait in script for the persistence data to be loaded and applied to the world the [`GetOnStateChangeEvent()`](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/Scripts/Game/EPF_PersistenceManager.c;94) can be subscribed. Alternatively if `OnWorldPostProcess` is used in the code already it can be awaited to finish as the persistence manager will block the return of that function until everything is loaded.
