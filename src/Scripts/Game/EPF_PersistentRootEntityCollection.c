[EDF_DbName("RootEntityCollection")]
class EPF_PersistentRootEntityCollection : EPF_MetaDataDbEntity
{
	[NonSerialized()]
	ref set<string> m_aPossibleBackedRootEntities = new set<string>();

	ref set<string> m_aRemovedBackedRootEntities = new set<string>();
	ref map<typename, ref array<string>> m_mSelfSpawnDynamicEntities = new map<typename, ref array<string>>();

	[NonSerialized()]
	protected bool m_bHasData;

	//------------------------------------------------------------------------------------------------
	void Add(EPF_PersistenceComponent persistenceComponent, string persistentId, EPF_EPersistenceManagerState state)
	{
		if (EPF_BitFlags.CheckFlags(persistenceComponent.GetFlags(), EPF_EPersistenceFlags.BAKED))
		{
			if (m_aRemovedBackedRootEntities.RemoveItem(persistentId))
				return;
		}

		EPF_PersistenceComponentClass settings = EPF_ComponentData<EPF_PersistenceComponentClass>.Get(persistenceComponent);
		if (!settings.m_bSelfSpawn)
			return;

		ForceSelfSpawn(persistenceComponent, persistentId, settings);
	}

	//------------------------------------------------------------------------------------------------
	void ForceSelfSpawn(EPF_PersistenceComponent persistenceComponent, string persistentId, EPF_PersistenceComponentClass settings)
	{
		array<string> ids = m_mSelfSpawnDynamicEntities.Get(settings.m_tSaveDataType);
		if (!ids)
		{
			ids = {persistentId};
			m_mSelfSpawnDynamicEntities.Set(settings.m_tSaveDataType, ids);
			return;
		}
		
		if (!ids.Contains(persistentId))
			ids.Insert(persistentId);
	}

	//------------------------------------------------------------------------------------------------
	void Remove(EPF_PersistenceComponent persistenceComponent, string persistentId, EPF_EPersistenceManagerState state)
	{
		if (EPF_BitFlags.CheckFlags(persistenceComponent.GetFlags(), EPF_EPersistenceFlags.BAKED) &&
			m_aPossibleBackedRootEntities.Contains(persistentId))
		{
			m_aRemovedBackedRootEntities.Insert(persistentId);
			return;
		}

		EPF_PersistenceComponentClass settings = EPF_ComponentData<EPF_PersistenceComponentClass>.Get(persistenceComponent);
		array<string> ids = m_mSelfSpawnDynamicEntities.Get(settings.m_tSaveDataType);
		if (!ids)
			return;

		ids.RemoveItem(persistentId);

		if (ids.IsEmpty())
			m_mSelfSpawnDynamicEntities.Remove(settings.m_tSaveDataType);
	}

	//------------------------------------------------------------------------------------------------
	void Save(EDF_DbContext dbContext)
	{
		m_iLastSaved = System.GetUnixTime();

		bool hasData = !m_aRemovedBackedRootEntities.IsEmpty() || !m_mSelfSpawnDynamicEntities.IsEmpty();
		if (hasData)
		{
			dbContext.AddOrUpdateAsync(this);
		}
		else
		{
			// Remove collection if we no longer have any data but there were some in the db previously
			if (m_bHasData)
				dbContext.RemoveAsync(this);
		}

		m_bHasData = hasData;
	}

	//------------------------------------------------------------------------------------------------
	protected bool SerializationSave(BaseSerializationSaveContext saveContext)
	{
		if (!saveContext.IsValid())
			return false;

		SerializeMetaData(saveContext);

		saveContext.WriteValue("m_aRemovedBackedRootEntities", m_aRemovedBackedRootEntities);

		array<ref EPF_SelfSpawnDynamicEntity> selfSpawnDynamicEntities();
		selfSpawnDynamicEntities.Reserve(m_mSelfSpawnDynamicEntities.Count());

		foreach (typename saveDataType, array<string> ids : m_mSelfSpawnDynamicEntities)
		{
			EPF_SelfSpawnDynamicEntity entry();
			entry.m_sSaveDataType = EDF_DbName.Get(saveDataType);
			entry.m_aIds = ids;
			selfSpawnDynamicEntities.Insert(entry);
		}

		saveContext.WriteValue("m_aSelfSpawnDynamicEntities", selfSpawnDynamicEntities);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected bool SerializationLoad(BaseSerializationLoadContext loadContext)
	{
		if (!loadContext.IsValid())
			return false;

		DeserializeMetaData(loadContext);

		loadContext.ReadValue("m_aRemovedBackedRootEntities", m_aRemovedBackedRootEntities);

		array<ref EPF_SelfSpawnDynamicEntity> selfSpawnDynamicEntities();
		loadContext.ReadValue("m_aSelfSpawnDynamicEntities", selfSpawnDynamicEntities);

		foreach (EPF_SelfSpawnDynamicEntity entry : selfSpawnDynamicEntities)
		{
			m_mSelfSpawnDynamicEntities.Set(EDF_DbName.GetTypeByName(entry.m_sSaveDataType), entry.m_aIds);
		}

		m_bHasData = !m_aRemovedBackedRootEntities.IsEmpty() || !m_mSelfSpawnDynamicEntities.IsEmpty();

		return true;
	}
}

class EPF_SelfSpawnDynamicEntity
{
	string m_sSaveDataType;
	ref array<string> m_aIds;
}
