# Saving custom components
Below is an example script component the number should be persisted on
```cs
class TAG_MyCustomComponentClass : ScriptComponentClass 
{
};

class TAG_MyCustomComponent : ScriptComponent
{
	int m_iNumber = 1337; // We want this one!
    int m_fAnotherNumber = 42.42;
};
```
## Minimal auto-copy example
The component save-data can be very simple when automatically copying a subset of information by property name. Here only `m_iNumber` will be saved and loaded. This also works if the component's properties are `private` or `protected`.
```cs
[EPF_ComponentSaveDataType(TAG_MyCustomComponent), BaseContainerProps()]
class TAG_MyCustomComponentSaveDataClass : EPF_ComponentSaveDataClass
{
};

[EDF_DbName.Automatic()]
class TAG_MyCustomComponentSaveData : EPF_ComponentSaveData
{
    int m_iNumber;
};
```

## Manual read, apply and compare
With an updated save-data implementation from above the read and write process as well as the comparison of save-data for equality is done manually. This is needed when reading from engine classes where the properties themselves are C++ side and only script functions can be used to get and set them, or when e.g. a getter has special logic to return only processed data. When manually accessing the data the visiblity of members and methods can be a problem. In that case either refactor your own classes to expose the info or use the `modded` keyword to add getters and setters for the required data.
```cs
[EDF_DbName.Automatic()]
class TAG_MyCustomComponentSaveData : EPF_ComponentSaveData
{
    int m_iNumber;

    //------------------------------------------------------------------------------------------------
    override EPF_EReadResult ReadFrom(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
    {
        TAG_MyCustomComponent myCustom = TAG_MyCustomComponent.Cast(component);
        m_iNumber = myCustom.m_iNumber;
        return EPF_EReadResult.OK;
    }

    //------------------------------------------------------------------------------------------------
    override EPF_EApplyResult ApplyTo(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
    {
        TAG_MyCustomComponent myCustom = TAG_MyCustomComponent.Cast(component);
        myCustom.m_iNumber = m_iNumber;
        return EPF_EApplyResult.OK;
    }

    //------------------------------------------------------------------------------------------------
    override bool Equals(notnull EPF_ComponentSaveData other)
    {
        TAG_MyCustomComponentSaveData otherData = TAG_MyCustomComponentSaveData.Cast(other);
        return m_iNumber == otherData.m_iNumber;
    }
};

```

## Component save-data settings
Similarly to how the script components have a "meta classes", entity and component save-data have them too. They serve to be a shared instance amongst all identically configured prefab instances. The save-data "class" classes can be configured as part of the persistence component attributes in the world editor and can be accessed in script like below.
```cs
...
class TAG_MyCustomComponentSaveDataClass : EPF_ComponentSaveDataClass
{
    [Attribute("1")]
    bool m_bDoSomething;
};

...
class TAG_MyCustomComponentSaveData : EPF_ComponentSaveData
{
    ...
    //------------------------------------------------------------------------------------------------
    override EPF_EReadResult ReadFrom(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
    {
        ...
        TAG_MyCustomComponentSaveDataClass settings = TAG_MyCustomComponentSaveDataClass.Cast(attributes);
        Print(settings.m_bDoSomething)
        ...
    }
};
```

## Multiple component instances
It is possible to have multiple instances of a component type on an entity e.g. a `BaseInventoryStorageComponent`. To select which instance gets which save-data applied the `IsFor()` method can be implemented. It is called for each of the component instances to find the matching one. A usage example of this can be found [here](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/Scripts/Game/Components/EPF_BaseInventoryStorageComponentSaveData.c;108).

## Load order requirements
Should it be necessary for the component save-data to be applied after another component was loaded because the logic depends on it then this can be achieved by implementing the `Requires` function. Component save-data is applied after the entity save-data unless a modder changes the order.
```cs
class TAG_MyCustomComponentSaveDataClass : EPF_ComponentSaveDataClass
{
    //------------------------------------------------------------------------------------------------
    override array<typename> Requires()
    {
        return {TAG_MyOtherComponentSaveDataClass};
    }
};
```

## Default value trimming
The [`EPF_EReadResult`](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/Scripts/Game/EPF_EReadResult.c;2) offers an enum value `DEFAULT`. This is used together with the `Trim Defaults` attribute on the entity save-data class to avoid writing any default values into the save-data. A vehicle has its engine off by default, so usually the save-data only needs some info about it when it is on. Returning `DEFAULT` from `ReadFrom` helps the system to skip the data entirely if the settings are configured that way. Usage examples of this can be found here: [`EPF_VehicleControllerSaveData`](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/Scripts/Game/Components/EPF_VehicleControllerSaveData.c;18), [`EPF_BaseLightManagerComponentSaveData`](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/Scripts/Game/Components/EPF_BaseLightManagerComponentSaveData.c;36).

## Delayed apply completion
Unfortunately, there are some systems in the game that are designed in a way that their initialization does not happen during `OnPostInit` or `EOnInit` but later on the first frame tick or worse, multiple frames later. This can make it difficult to apply the save-data and wait for the completion because normally this process is blocking and thus instant to external code. With Arma 4 hopefully a lot less of this following "hack" is needed but at least there are ways to deal with it right now.  
The `ApplyTo` method can return [`EPF_EApplyResult.AWAIT_COMPLETION`](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/Scripts/Game/EPF_EApplyResult.c;5). This signals to the persistence component `Load` method to wait before firing the `OnAfterLoad` event. Pending processes can be added via
```cs
//------------------------------------------------------------------------------------------------
override EPF_EApplyResult ApplyTo(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
{
    ...
    EPF_DeferredApplyResult.AddPending(this, "SomeOperationIdentifier");
    return EPF_EApplyResult.AWAIT_COMPLETION;
}
```
and be completed with 
```cs
EPF_DeferredApplyResult.SetFinished(this, "SomeOperationIdentifier");
```
A full though not "simple" example usage can be found here [`EPF_CharacterControllerComponentSaveData`](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/Scripts/Game/Entities/Character/EPF_CharacterControllerComponentSaveData.c;99) to wait for the character weapon and gadget animations to finish before allowing the MP server to hand over network ownership. In other situations, it is used to wait 2 frames after a turret was created to apply the aiming angles because god knows why the system requires us to wait that long ([`EPF_TurretControllerComponentSaveData`](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/Scripts/Game/Components/EPF_TurretControllerComponentSaveData.c;57)).

On your own custom components, design them in a way that you do not need to wait for anything and you will never have to deal with this ... "interesting" concept.
