# [`EPF_PersistentScriptedState`](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/Scripts/Game/EPF_PersistentScriptedState.c;1)
Persistent scripted states are class instances that are unrelated to `IEntity` and `GenericComponent`. They are custom script classes that still need some save-data system. This could be a persistent scoreboard or some central manager that is done via scripted singleton instead of a manager entity on the map.

## State setup
There are two approaches to setting up the scripted state itself

### Persistence first
If the class was built with persistence in mind from the start the `EPF_PersistentScriptedState` base class can simply be inherited from. It takes care of all the required registration and offers the same script API that can be found on the [persistence component](persistence-component.md).
```cs
class TAG_MyScriptedState : EPF_PersistentScriptedState
{
    int m_iValue; // Some value we want to persist
}
```
The corresponding save-data for it is created like below. Different from the component setup this time the save-data also has some meta configuration via the [`EPF_PersistentScriptedStateSettings`](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/Scripts/Game/EPF_PersistentScriptedState.c;470). The settings can be set via the two optional parameters `saveType` and `options`. These replace the attributes normally configurable via the "class class" in the world editor. The first argument is for which scripted state typename the save-data is responsible. The example uses the auto copy by property name feature, which is almost always the easiest and also fastest for simple transfer of even deeply nested scripted info (like arrays of complex objects). Manual transfer is only required on polymorph arrays or when additional getter and setter logic should be invoked.
```cs
[
    EPF_PersistentScriptedStateSettings(TAG_MyScriptedState),
    EDF_DbName.Automatic()
]
class TAG_MyScriptedStateSaveData : EPF_ScriptedStateSaveData
{
    int m_iValue;
}
```

### Persistence as an afterthought
There will be situations where instances from vanilla or other mods need persistence added manually. But it is not possible to assign a new base class like in the example below.
```cs
/*external code from other mod*/
class TAG_MyExistingCounter
{
    int m_iCount;

    //------------------------------------------------------------------------------------------------
    void Increment()
    {
        Print(++m_iCount);
    }
};
```

For this situation there is the [`EPF_PersistentScriptedStateProxy`](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/Scripts/Game/EPF_PersistentScriptedState.c;304). It can be used to turn an existing instance into a scripted state and easily load save-data again. As a first step, the `Constructor` and `Destructor` of the class are hooked through a modded overload. Alternatively, this can of course be done in external code that controls the creation and removal of the target instances, but this way it's guaranteed not to miss anything. This example is using async callbacks and assumes that the counter is a singleton, but the API allows load by ID too.
```cs
modded class TAG_MyExistingCounter
{
    //------------------------------------------------------------------------------------------------
    void TAG_MyExistingCounter()
    {
        EDF_DataCallbackSingle<EPF_ScriptedStateSaveData> callback(this, "OnPersistenceLoaded");
        EPF_PersistentScriptedStateProxy.CreateAsync(this, callback: callback);
    }

    //------------------------------------------------------------------------------------------------
    private void OnPersistenceLoaded()
    {
        PrintFormat("Restored %1 with m_iCount = %2.", this, m_iCount);
    }

    //------------------------------------------------------------------------------------------------
    void ~TAG_MyExistingCounter()
    {
        EPF_PersistentScriptedStateProxy.Destory(this);
    }
};
```
With this proxy setup, it just needs the save-data for the class.
```cs
[
    EPF_PersistentScriptedStateSettings(TAG_MyExistingCounter),
    EDF_DbName.Automatic()
]
class TAG_MyExistingCounterSaveData : EPF_ScriptedStateSaveData
{
    int m_iCount;
};
```
And the setup is complete. To test this example the following action can be run.
```cs
class TAG_MyCounterAction : ScriptedUserAction
{
    ref TAG_MyExistingCounter m_pCounter;

    //------------------------------------------------------------------------------------------------
    override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
    {
        if (!m_pCounter)
            m_pCounter = new TAG_MyExistingCounter();

        if (m_pCounter)
            m_pCounter.Increment();

        if (m_pCounter.m_iCount > 10)
            m_pCounter = null;
    }
};
```
>SCRIPT: int m_iCount = 1  
>SCRIPT: int m_iCount = 2  
>SCRIPT: int m_iCount = 3  

After running it 3 times and exiting this is in the db.

`.db\EnfusionPersistenceFramework.Tests\MyExistingCounters\647399a8-0000-0000-0001-5487a9003c24.json`
```json
{
    "m_sId": "647399a8-0000-0000-0001-5487a9003c24",
    "m_iDataLayoutVersion": 1,
    "m_iLastSaved": 1685297576,
    "m_iCount": 3
}
```
Loading back in and running the action again produces
>SCRIPT: Restored TAG_MyExistingCounter<0x0000025A926A5050> with m_iCount = 3.  
>SCRIPT: int m_iCount = 4  
