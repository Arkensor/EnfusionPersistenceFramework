# Utilities

## Loader utilities
To cut the process of getting the correct save-data from db and spawning it into the world as easy as possible two utility classes are provided. Check each inline documentation for how to use them.

### [`EPF_PersistentWorldEntityLoader`](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/Scripts/Game/EPF_PersistentWorldEntityLoader.c;1)
World entity utility

### [`EPF_PersistentScriptedStateLoader<T>`](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/Scripts/Game/EPF_PersistentScriptedStateLoader.c;1)
Scripted state utility - this one can not be used for `EPF_PersistentScriptedStateProxy`s.

## Persistence DB context and repository
> **Note**
> Before trying to access the persistence DB directly, double-check that there is no other intended way to handle the task at hand.
> Normally all features can be designed through the use of the existing `EPF_PersistenceComponent` and `EPF_PersistentScriptedState`.

To use the same `EDF_DbContext` as the persistence system, any custom logic can access it through [`EPF_PersistenceManager.GetDbContext()`](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/Scripts/Game/EPF_PersistenceManager.c;103).
There are also utility classes to get an `EL_DbRepository<T>` that is already connected to the persistence DB context:
- [`EPF_PersistenceEntityHelper<TEntityType>`](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/Scripts/Game/EPF_PersistenceRepository.c;1)
- [`EPF_PersistenceRepositoryHelper<TRepositoryType>`](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/Scripts/Game/EPF_PersistenceRepository.c;23)
