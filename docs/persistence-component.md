# [`EPF_PersistenceComponent`](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/Scripts/Game/EPF_PersistenceComponent.c;32)
To connect an `IEntity` to the [`EPF_PersistenceManager`](persistence-manager.md) the `EPF_PersistenceComponent` is used. It is already present on many of the base prefabs used for the vanilla entities. Only items with this component will be persisted. If an entity holds persistent items in a storage but itself is not persistent, all entities will be lost (this might be on purpose for things like a trashcan). Prefabs that previously had an enabled persistence component but are now no longer configured as persistent are dropped from loading.
For the different options please consult the attribute descriptions.

## The persistent id
The id is assigned automatically by the [id genetator](id-generation.md) and only in rare cases one should manually set the id. 

To get the persistent id of an entity [`GetPersistentId`](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/Scripts/Game/EPF_PersistenceComponent.c;68) can be called on the `EPF_PersistenceComponent` or the static overload is used which uses the `IEntiy` as the argument.

## The root entity concept
In the documentation and scripts the term "root entity" is often used. This refers to entities that are not stored/attached inside/to other entities. Common examples of root entities are vehicles, players, and houses. Only root entities are saved in the database as their top-level "records". Any items inside e.g. inventory storages are saved as part of their parent hierarchy. The individual database backend might still save them as their own records and join them together later, but for something like the local JSON DB, this is not the case. Performing a search by ID in the database for nonroot entities is thus very inefficient.

## Manual save, load, delete
An entity can be saved at any time by calling [`Save()`](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/Scripts/Game/EPF_PersistenceComponent.c;157). This does not reset the auto-save timer for the entity.

Similarly, loading can be done with a save-data instance via [`Load()`](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/Scripts/Game/EPF_PersistenceComponent.c;225). This will also update the entity registration at the persistence manager if the save-data has a different id than the persistence component currently holds.

The persistence data of an entity can be deleted via [`Delete()`](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/Scripts/Game/EPF_PersistenceComponent.c;278) this does not delete the entity. Unless the tracking is paused however the next auto- or shutdown-save might create the record again and assign a new different id. 

## Pausing persistence tracking
Sometimes it can be necessary to pause all the automated processes of persistence to e.g. manually manage the removal of a vehicle while putting it into a virtual garage. For this [`PauseTracking`](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/Scripts/Game/EPF_PersistenceComponent.c;142) and [`ResumeTracking`](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/Scripts/Game/EPF_PersistenceComponent.c;149). While not tracked auto- and shutdown-save will not be called, nor will the entity self delete the records.

## Events
There are a few events exposed that can be used as information sources or to inject/manipulate save-data into the process for more advanced use cases.
- [`GetOnAfterSaveEvent`](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/Scripts/Game/EPF_PersistenceComponent.c;105)
- [`GetOnAfterPersistEvent`](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/Scripts/Game/EPF_PersistenceComponent.c;115)
- [`GetOnBeforeLoadEvent`](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/Scripts/Game/EPF_PersistenceComponent.c;124)
- [`GetOnAfterLoadEvent`](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/Scripts/Game/EPF_PersistenceComponent.c;133)
