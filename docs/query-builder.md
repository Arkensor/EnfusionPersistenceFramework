# [`EPF_DbFind`](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/Scripts/Game/EPF_DbFind.c;1)

In addition to the [query builder](https://github.com/Arkensor/EnfusionDatabaseFramework/blob/armareforger/docs/query-builder.md) provided by the database framework there are some specializations for persistence data.

For example, the following db find condition would find all vehicles in the database that have their engine turned on.
```cs
EDF_DbFindCondition condition = EPF_DbFind.Component(EPF_VehicleControllerSaveData).Any().Field("m_bEngineOn").Equals(true);
```
