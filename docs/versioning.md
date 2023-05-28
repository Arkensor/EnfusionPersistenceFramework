# Versioning
One key feature advertised for the framework is built-in versioning. For this, a simple variable [`m_iDataLayoutVersion`](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/Scripts/Game/EPF_MetaDataDbEntity.c;3) is added to all save-data classes. By default, the version of the layout is 1. In this example, (based on the [TAG_MyExistingCounter](scripted-states.md#persistence-as-an-afterthought) from scripted state docs) the datatype in the save-data for a version two will change from int to string and needs to be translated on loading or else the save-data would be treated as corrupted.

The existing save-data looks like this, it still has integer data in it.
```json
{
    "m_sId": "647399a8-0000-0000-0001-5487a9003c24",
    "m_iDataLayoutVersion": 1,
    "m_iLastSaved": 1685297576,
    "m_iCount": 3
}
```

To migrate a version 1 layout on load all that needs to be done is implement the serialization methods and handle the change.
```cs
[
    EPF_PersistentScriptedStateSettings(TAG_MyExistingCounter),
    EDF_DbName.Automatic()
]
class TAG_MyExistingCounterSaveData : EPF_ScriptedStateSaveData
{
    string m_sCount;

    //------------------------------------------------------------------------------------------------
    protected bool SerializationLoad(BaseSerializationLoadContext loadContext)
    {
        DeserializeMetaData(loadContext);

        // Migtate V1 data
        if (m_iDataLayoutVersion == 1)
        {
            int migrateCount;
            loadContext.ReadValue("m_iCount", migrateCount);
            m_sCount = migrateCount.ToString();
            return true;
        }

        loadContext.ReadValue("m_sCount", m_sCount);

        return true;
    }

    //------------------------------------------------------------------------------------------------
    void TAG_MyExistingCounterSaveData()
    {
        // We set the version to two via ctor, but could also be done via SerializationSave
        m_iDataLayoutVersion = 2;
    }
};
```
> SCRIPT: Restored TAG_MyExistingCounter<0x0000025A919BD5F0> with m_sCount = 4.
