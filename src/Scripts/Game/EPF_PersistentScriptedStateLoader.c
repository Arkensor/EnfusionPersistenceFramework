class EPF_PersistentScriptedStateLoader<Class TScriptedState>
{
	//------------------------------------------------------------------------------------------------
	//! Load and instantiate or create the scripted state singleton
	//! \return scripted state instance
	static TScriptedState LoadSingleton()
	{
		typename saveDataType;
		if (!TypeAndSettingsValidation(saveDataType))
			return null;

		EPF_PersistenceManager persistenceManager = EPF_PersistenceManager.GetInstance();
		array<ref EDF_DbEntity> findResults = persistenceManager
			.GetDbContext()
			.FindAll(saveDataType, limit: 1)
			.GetEntities();

		if (!findResults || findResults.IsEmpty())
		{
			typename spawnType = TScriptedState;
			return TScriptedState.Cast(spawnType.Spawn());
		}

		return TScriptedState.Cast(persistenceManager.SpawnScriptedState(EPF_ScriptedStateSaveData.Cast(findResults.Get(0))));
	}

	//------------------------------------------------------------------------------------------------
	//! s. LoadSingleton()
	static void LoadSingletonAsync(EDF_DataCallbackSingle<TScriptedState> callback = null)
	{
		typename saveDataType;
		if (!TypeAndSettingsValidation(saveDataType))
			return;

		EPF_ScriptedStateLoaderProcessorCallbackSingle<TScriptedState> processorCallback();
		processorCallback.Setup(callback, true, TScriptedState);

		EPF_PersistenceManager.GetInstance()
			.GetDbContext()
			.FindAllAsync(saveDataType, limit: 1, callback: processorCallback);
	}

	//------------------------------------------------------------------------------------------------
	//! Load and instantiate a scripted state by persistent id
	//! \param persistentId Persistent id
	//! \return instantiated scripted state instance or null on failure
	static TScriptedState Load(string persistentId)
	{
		typename saveDataType;
		if (!TypeAndSettingsValidation(saveDataType))
			return null;

		EPF_PersistenceManager persistenceManager = EPF_PersistenceManager.GetInstance();
		array<ref EDF_DbEntity> findResults = persistenceManager
			.GetDbContext()
			.FindAll(saveDataType, EDF_DbFind.Id().Equals(persistentId), limit: 1)
			.GetEntities();

		if (!findResults || findResults.Count() != 1)
			return null;

		return TScriptedState.Cast(persistenceManager.SpawnScriptedState(EPF_ScriptedStateSaveData.Cast(findResults.Get(0))));
	}

	//------------------------------------------------------------------------------------------------
	//! s. Load()
	static void LoadAsync(string persistentId, EDF_DataCallbackSingle<TScriptedState> callback = null)
	{
		typename saveDataType;
		if (!TypeAndSettingsValidation(saveDataType))
			return;

		EPF_ScriptedStateLoaderProcessorCallbackSingle<TScriptedState> processorCallback();
		processorCallback.Setup(callback, false, TScriptedState);

		EPF_PersistenceManager.GetInstance()
			.GetDbContext()
			.FindAllAsync(saveDataType, EDF_DbFind.Id().Equals(persistentId), limit: 1, callback: processorCallback);
	}

	//------------------------------------------------------------------------------------------------
	//! Load and instantiate multiple scripted states by their persistent id or all of this type if left empty/null
	//! \param persistentIds Persistent ids
	//! \return array of scripted state instances or emtpy
	static array<ref TScriptedState> Load(array<string> persistentIds = null)
	{
		typename saveDataType;
		if (!TypeAndSettingsValidation(saveDataType))
			return null;

		EDF_DbFindCondition condition;
		if (persistentIds && !persistentIds.IsEmpty())
			condition = EDF_DbFind.Id().EqualsAnyOf(persistentIds);

		EPF_PersistenceManager persistenceManager = EPF_PersistenceManager.GetInstance();
		array<ref EDF_DbEntity> findResults = persistenceManager
			.GetDbContext()
			.FindAll(saveDataType, condition)
			.GetEntities();

		array<ref TScriptedState> resultStates();
		if (findResults)
		{
			foreach (EDF_DbEntity findResult : findResults)
			{
				TScriptedState state = TScriptedState.Cast(persistenceManager.SpawnScriptedState(EPF_ScriptedStateSaveData.Cast(findResult)));
				if (state)
					resultStates.Insert(state);
			}
		}

		return resultStates;
	}

	//------------------------------------------------------------------------------------------------
	//! s. Load(array<string>?)
	static void LoadAsync(array<string> persistentIds = null, EDF_DataCallbackMultiple<TScriptedState> callback = null)
	{
		typename saveDataType;
		if (!TypeAndSettingsValidation(saveDataType))
			return;

		EPF_ScriptedStateLoaderProcessorCallbackMultiple<TScriptedState> processorCallback();
		processorCallback.Setup(callback);

		EDF_DbFindCondition condition;
		if (persistentIds && !persistentIds.IsEmpty())
			condition = EDF_DbFind.Id().EqualsAnyOf(persistentIds);

		EPF_PersistenceManager.GetInstance()
			.GetDbContext()
			.FindAllAsync(saveDataType, condition, callback: processorCallback);
	}

	//------------------------------------------------------------------------------------------------
	protected static bool TypeAndSettingsValidation(out typename saveDataType)
	{
		typename resultType = TScriptedState;
		if (!resultType.IsInherited(EPF_PersistentScriptedState))
			return false;

		auto settings = EPF_PersistentScriptedStateSettings.Get(TScriptedState);
		if (!settings || !settings.m_tSaveDataType)
		{
			Debug.Error(string.Format("Scripted state type '%1' needs to have no save-data configured to be loaded!", TScriptedState));
			return false;
		}

		saveDataType = settings.m_tSaveDataType;

		return true;
	}
}

class EPF_ScriptedStateLoaderProcessorCallbackSingle<Class TScriptedState> : EDF_DbFindCallbackSingle<EPF_ScriptedStateSaveData>
{
	ref EDF_DataCallbackSingle<TScriptedState> m_pCallback;
	bool m_bCreateSingleton;
	typename m_tCreateType;

	//------------------------------------------------------------------------------------------------
	override void OnSuccess(EPF_ScriptedStateSaveData result, Managed context)
	{
		EPF_PersistenceManager persistenceManager = EPF_PersistenceManager.GetInstance();

		TScriptedState resultScriptedState;

		if (result)
		{
			resultScriptedState = TScriptedState.Cast(persistenceManager.SpawnScriptedState(result))
		}
		else if (m_bCreateSingleton)
		{
			resultScriptedState = TScriptedState.Cast(m_tCreateType.Spawn());
		}

		m_pCallback.Invoke(resultScriptedState);
	}

	//------------------------------------------------------------------------------------------------
	override void OnFailure(EDF_EDbOperationStatusCode statusCode, Managed context)
	{
		m_pCallback.Invoke(null);
	}

	//------------------------------------------------------------------------------------------------
	void Setup(EDF_DataCallbackSingle<TScriptedState> callback, bool createSingleton, typename createType)
	{
		m_pCallback = callback;
		m_bCreateSingleton = createSingleton;
		m_tCreateType = createType;
	}
}

class EPF_ScriptedStateLoaderProcessorCallbackMultiple<Class TScriptedState> : EDF_DbFindCallbackMultiple<EPF_ScriptedStateSaveData>
{
	ref EDF_DataCallbackMultiple<TScriptedState> m_pCallback;

	//------------------------------------------------------------------------------------------------
	override void OnSuccess(array<ref EPF_ScriptedStateSaveData> results, Managed context)
	{
		EPF_PersistenceManager persistenceManager = EPF_PersistenceManager.GetInstance();

		array<TScriptedState> resultStates();
		array<ref TScriptedState> resultRefs();
		resultStates.Reserve(results.Count());
		resultRefs.Reserve(results.Count());

		foreach (EPF_ScriptedStateSaveData saveData : results)
		{
			TScriptedState resultScriptedState = TScriptedState.Cast(persistenceManager.SpawnScriptedState(saveData));
			if (resultScriptedState)
			{
				resultStates.Insert(resultScriptedState);
				resultRefs.Insert(resultScriptedState);
			}
		}

		m_pCallback.Invoke(resultStates);
	}

	//------------------------------------------------------------------------------------------------
	override void OnFailure(EDF_EDbOperationStatusCode statusCode, Managed context)
	{
		m_pCallback.Invoke(null);
	}

	//------------------------------------------------------------------------------------------------
	void Setup(EDF_DataCallbackMultiple<TScriptedState> callback)
	{
		m_pCallback = callback;
	}
}
