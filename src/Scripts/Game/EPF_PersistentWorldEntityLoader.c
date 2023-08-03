class EPF_PersistentWorldEntityLoader
{
	protected static ref map<string, typename> s_mSaveDataTypeCache;

	//------------------------------------------------------------------------------------------------
	//! Load and spawn an entity by save-data type and persistent id
	//! \param saveDataType save-data type of the entity
	//! \param persistentId Persistent id
	//! \return Spawned entity or null on failure
	static IEntity Load(typename saveDataType, string persistentId)
	{
		EPF_PersistenceManager persistenceManager = EPF_PersistenceManager.GetInstance();
		array<ref EDF_DbEntity> findResults = persistenceManager
			.GetDbContext()
			.FindAll(saveDataType, EDF_DbFind.Id().Equals(persistentId), limit: 1)
			.GetEntities();

		if (!findResults || findResults.Count() != 1)
			return null;

		return persistenceManager.SpawnWorldEntity(EPF_EntitySaveData.Cast(findResults.Get(0)));
	}

	//------------------------------------------------------------------------------------------------
	//! Load and spawn an entity by prefab and persistent id
	//! \param prefab Prefab resource name
	//! \param persistentId Persistent id
	//! \return Spawned entity or null on failure
	static IEntity Load(string prefab, string persistentId)
	{
		typename type = GetSaveDataType(prefab); // Can not be inlined because of typename crash bug
		return Load(type, persistentId);
	}

	//------------------------------------------------------------------------------------------------
	//! see Load(typename, string)
	static void LoadAsync(typename saveDataType, string persistentId, EDF_DataCallbackSingle<IEntity> callback = null)
	{
		auto processorCallback = new EPF_WorldEntityLoaderProcessorCallbackSingle(context: callback);
		EPF_PersistenceManager.GetInstance()
			.GetDbContext()
			.FindAllAsync(saveDataType, EDF_DbFind.Id().Equals(persistentId), limit: 1, callback: processorCallback);
	}

	//------------------------------------------------------------------------------------------------
	//! see Load(string, string)
	static void LoadAsync(string prefab, string persistentId, EDF_DataCallbackSingle<IEntity> callback = null)
	{
		typename type = GetSaveDataType(prefab); // Can not be inlined because of typename crash bug
		LoadAsync(type, persistentId, callback);
	}

	//------------------------------------------------------------------------------------------------
	//! Load and spawn multiple entities by save-data type and persistent ids / or all if ids is null/empty
	//! \param saveDataType save-data type of the entities
	//! \param persistentId Array of persistent ids - or null/empty for load all.
	//! \return array of entities if spawned successfully, else empty
	static array<IEntity> Load(typename saveDataType, array<string> persistentIds = null)
	{
		array<IEntity> resultEntities();

		EDF_DbFindCondition condition;
		if (persistentIds && !persistentIds.IsEmpty())
			condition = EDF_DbFind.Id().EqualsAnyOf(persistentIds);

		EPF_PersistenceManager persistenceManager = EPF_PersistenceManager.GetInstance();
		array<ref EDF_DbEntity> findResults = persistenceManager
			.GetDbContext()
			.FindAll(saveDataType, condition)
			.GetEntities();

		if (findResults)
		{
			foreach (EDF_DbEntity findResult : findResults)
			{
				IEntity entity = persistenceManager.SpawnWorldEntity(EPF_EntitySaveData.Cast(findResult));
				if (entity)
					resultEntities.Insert(entity);
			}
		}

		return resultEntities;
	}

	//------------------------------------------------------------------------------------------------
	//! Load and spawn multiple entities by prefab and persistent ids / or all if ids is null/empty
	//! \param Prefab resource name of the entities
	//! \param persistentId Array of persistent ids - or null/empty for load all.
	//! \return array of entities if spawned successfully, else empty
	static array<IEntity> Load(string prefab, array<string> persistentIds = null)
	{
		typename type = GetSaveDataType(prefab); // Can not be inlined because of typename crash bug
		return Load(type, persistentIds);
	}

	//------------------------------------------------------------------------------------------------
	//! see Load(typename, array<string>)
	static void LoadAsync(typename saveDataType, array<string> persistentIds = null, EDF_DataCallbackMultiple<IEntity> callback = null)
	{
		auto processorCallback = new EPF_WorldEntityLoaderProcessorCallbackMultiple(context: callback);

		EDF_DbFindCondition condition;
		if (persistentIds && !persistentIds.IsEmpty())
			condition = EDF_DbFind.Id().EqualsAnyOf(persistentIds);

		EPF_PersistenceManager.GetInstance()
			.GetDbContext()
			.FindAllAsync(saveDataType, condition, callback: processorCallback);
	}

	//------------------------------------------------------------------------------------------------
	//! see Load(string, array<string>)
	static void LoadAsync(string prefab, array<string> persistentIds = null, EDF_DataCallbackMultiple<IEntity> callback = null)
	{
		typename type = GetSaveDataType(prefab); // Can not be inlined because of typename crash bug
		LoadAsync(type, persistentIds, callback);
	}

	//------------------------------------------------------------------------------------------------
	protected static typename GetSaveDataType(string prefab)
	{
		if (!s_mSaveDataTypeCache)
			s_mSaveDataTypeCache = new map<string, typename>();

		typename resultType = s_mSaveDataTypeCache.Get(prefab);

		if (!resultType)
		{
			Resource resource = Resource.Load(prefab);
			if (resource && resource.IsValid())
			{
				IEntitySource entitySource = resource.GetResource().ToEntitySource();
				for (int nComponentSource = 0, count = entitySource.GetComponentCount(); nComponentSource < count; nComponentSource++)
				{
					IEntityComponentSource componentSource = entitySource.GetComponent(nComponentSource);
					typename componentType = componentSource.GetClassName().ToType();
					if (componentType.IsInherited(EPF_PersistenceComponent))
					{
						BaseContainer saveDataContainer = componentSource.GetObject("m_pSaveData");
						if (saveDataContainer)
						{
							resultType = EPF_Utils.TrimEnd(saveDataContainer.GetClassName(), 5).ToType();
							break;
						}
					}
				}
			}
		}

		s_mSaveDataTypeCache.Set(prefab, resultType);

		return resultType;
	}
}

class EPF_WorldEntityLoaderProcessorCallbackSingle : EDF_DbFindCallbackSingle<EPF_EntitySaveData>
{
	//------------------------------------------------------------------------------------------------
	override void OnSuccess(EPF_EntitySaveData result, Managed context)
	{
		IEntity resultWorldEntity = EPF_PersistenceManager.GetInstance().SpawnWorldEntity(result);
		auto callback = EDF_DataCallbackSingle<IEntity>.Cast(context);
		if (callback)
			callback.Invoke(resultWorldEntity);
	}

	//------------------------------------------------------------------------------------------------
	override void OnFailure(EDF_EDbOperationStatusCode statusCode, Managed context)
	{
		auto callback = EDF_DataCallbackSingle<IEntity>.Cast(context);
		if (callback)
			callback.Invoke(null);
	}
}

class EPF_WorldEntityLoaderProcessorCallbackMultiple : EDF_DbFindCallbackMultiple<EPF_EntitySaveData>
{
	//------------------------------------------------------------------------------------------------
	override void OnSuccess(array<ref EPF_EntitySaveData> results, Managed context)
	{
		EPF_PersistenceManager persistenceManager = EPF_PersistenceManager.GetInstance();

		array<IEntity> resultEntities();
		foreach (EPF_EntitySaveData saveData : results)
		{
			IEntity entity = persistenceManager.SpawnWorldEntity(saveData);
			if (entity)
				resultEntities.Insert(entity);
		}

		auto callback = EDF_DataCallbackMultiple<IEntity>.Cast(context);
		if (callback)
			callback.Invoke(resultEntities);
	}

	//------------------------------------------------------------------------------------------------
	override void OnFailure(EDF_EDbOperationStatusCode statusCode, Managed context)
	{
		auto callback = EDF_DataCallbackMultiple<IEntity>.Cast(context);
		if (callback)
			callback.Invoke(new array<IEntity>());
	}
}
