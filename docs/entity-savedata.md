# Save entity information
This example shows what is needed to save information that is straight on an entity instead of a scripted component. Normally it is highly recommended to put scripted logic into reusable components.
```cs
class TAG_MyCustomEntityClass : GenericEntityClass 
{ 
};

class TAG_MyCustomEntity : GenericEntity 
{
    int m_iNumber = 1337;
};
```

## Creating the save-data class
The following save-data class helps as a basis for what to select in the persistence component as `Save Data`. The `[BaseContainerProps()]` attribute is added to make the class selectable in the workbench. The `[EDF_DbName.Automatic()]` is about the naming in the database, please [read this](https://github.com/Arkensor/EnfusionDatabaseFramework/blob/armareforger/docs/db-entity.md#creating-your-own-database-entity).
```cs
[BaseContainerProps()]
class EPF_MyCustomEntityDataClass : EPF_EntitySaveDataClass
{
};

[EDF_DbName.Automatic()]
class EPF_MyCustomEntitySaveData : EPF_EntitySaveData
{
    int m_iNumber;

    //------------------------------------------------------------------------------------------------
    override EPF_EReadResult ReadFrom(IEntity entity, EPF_EntitySaveDataClass attributes)
    {
        EPF_EReadResult readResult = super.ReadFrom(entity, attributes);

        // Option 1: Manually getting the data
        TAG_MyCustomEntity customEntity = TAG_MyCustomEntity.Cast(entity);
        m_iNumber = customEntity.m_iNumber;

        // Option 2: Automatically extract by matching property name
        EDF_DbEntityUtils.StructAutoCopy(entity, this);

        return readResult;
    }

    //------------------------------------------------------------------------------------------------
    override EPF_EApplyResult ApplyTo(IEntity entity, EPF_EntitySaveDataClass attributes)
    {
        EPF_EApplyResult applyResult = super.ApplyTo(entity, attributes);

        // Option 1: Manually setting the data
        TAG_MyCustomEntity customEntity = TAG_MyCustomEntity.Cast(entity);
        customEntity.m_iNumber = m_iNumber;

        // Option 2: Automatically applying on matching property name
        // !!! DANGER: The base class could contain huge amounts of component save data and other info.
        //             On auto-struct copy all of that would be serialized again. 
        //             For this situation consider going for manually applying it...
        // As an example of this see EPF_TimeAndWeatherSaveData
        EDF_DbEntityUtils.StructAutoCopy(this, entity);

        return applyResult;
    }

    //------------------------------------------------------------------------------------------------
    override protected bool SerializationSave(BaseSerializationSaveContext saveContext)
    {
        if (!super.SerializationSave(saveContext))
            return false;

        saveContext.WriteValue("m_iNumber", m_iNumber);

        return true;
    }

    //------------------------------------------------------------------------------------------------
    override protected bool SerializationLoad(BaseSerializationLoadContext loadContext)
    {
        if (!super.SerializationLoad(loadContext))
            return false;

        loadContext.ReadValue("m_iNumber", m_iNumber);
        
        return true;
    }
};
```
