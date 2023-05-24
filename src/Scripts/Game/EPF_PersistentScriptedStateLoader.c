class EPF_PersistentScriptedStateLoader<Class TScriptedState>
{
	//------------------------------------------------------------------------------------------------
	//! Load and instantiate or create the scripted state singleton
	//! \return scripted state instance
	static TScriptedState LoadSingleton()
	{
		typename saveDataType;
		if (!TypeAndSettingsValidation(saveDataType)) return null;

		EPF_PersistenceManager persistenceManager = EPF_PersistenceManager.GetInstance();
		array<ref EDF_DbEntity> findResults = persistenceManager.GetDbContext().FindAll(saveDataType, limit: 1).GetEntities();
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
		if (!TypeAndSettingsValidation(saveDataType)) return;

		EPF_ScriptedStateLoaderCallbackInvokerSingle<TScriptedState> callbackInvoker(callback);
		EPF_ScriptedStateLoaderProcessorCallbackSingle processorCallback();
		processorCallback.Setup(callbackInvoker, true, TScriptedState);
		EPF_PersistenceManager.GetInstance().GetDbContext().FindAllAsync(saveDataType, limit: 1, callback: processorCallback);
	}

	//------------------------------------------------------------------------------------------------
	//! Load and instantiate a scripted state by persistent id
	//! \param persistentId Persistent id
	//! \return instantiated scripted state instance or null on failure
	static TScriptedState Load(string persistentId)
	{
		typename saveDataType;
		if (!TypeAndSettingsValidation(saveDataType)) return null;

		EPF_PersistenceManager persistenceManager = EPF_PersistenceManager.GetInstance();
		array<ref EDF_DbEntity> findResults = persistenceManager.GetDbContext().FindAll(saveDataType, EDF_DbFind.Id().Equals(persistentId), limit: 1).GetEntities();
		if (!findResults || findResults.Count() != 1) return null;

		return TScriptedState.Cast(persistenceManager.SpawnScriptedState(EPF_ScriptedStateSaveData.Cast(findResults.Get(0))));
	}

	//------------------------------------------------------------------------------------------------
	//! s. Load()
	static void LoadAsync(string persistentId, EDF_DataCallbackSingle<TScriptedState> callback = null)
	{
		typename saveDataType;
		if (!TypeAndSettingsValidation(saveDataType)) return;

		EPF_ScriptedStateLoaderCallbackInvokerSingle<TScriptedState> callbackInvoker(callback);
		EPF_ScriptedStateLoaderProcessorCallbackSingle processorCallback();
		processorCallback.Setup(callbackInvoker, false, TScriptedState);
		EPF_PersistenceManager.GetInstance().GetDbContext().FindAllAsync(saveDataType, EDF_DbFind.Id().Equals(persistentId), limit: 1, callback: processorCallback);
	}

	//------------------------------------------------------------------------------------------------
	//! Load and instantiate multiple scripted states by their persistent id or all of this type if left empty/null
	//! \param persistentIds Persistent ids
	//! \return array of scripted state instances or emtpy
	static array<ref TScriptedState> Load(array<string> persistentIds = null)
	{
		typename saveDataType;
		if (!TypeAndSettingsValidation(saveDataType)) return null;

		array<ref TScriptedState> resultStates();

		EDF_DbFindCondition condition;
		if (persistentIds && !persistentIds.IsEmpty())
			condition = EDF_DbFind.Id().EqualsAnyOf(persistentIds);

		EPF_PersistenceManager persistenceManager = EPF_PersistenceManager.GetInstance();
		array<ref EDF_DbEntity> findResults = persistenceManager.GetDbContext().FindAll(saveDataType, condition).GetEntities();
		if (findResults)
		{
			foreach (EDF_DbEntity findResult : findResults)
			{
				TScriptedState state = TScriptedState.Cast(persistenceManager.SpawnScriptedState(EPF_ScriptedStateSaveData.Cast(findResult)));
				if (state) resultStates.Insert(state);
			}
		}

		return resultStates;
	}

	//------------------------------------------------------------------------------------------------
	//! s. Load(array<string>?)
	static void LoadAsync(array<string> persistentIds = null, EDF_DataCallbackMultiple<TScriptedState> callback = null)
	{
		typename saveDataType;
		if (!TypeAndSettingsValidation(saveDataType)) return;

		EPF_ScriptedStateLoaderCallbackInvokerMultiple<TScriptedState> callbackInvoker(callback);
		EPF_ScriptedStateLoaderProcessorCallbackMultiple processorCallback();
		processorCallback.Setup(callbackInvoker);

		EDF_DbFindCondition condition;
		if (persistentIds && !persistentIds.IsEmpty())
			condition = EDF_DbFind.Id().EqualsAnyOf(persistentIds);

		EPF_PersistenceManager.GetInstance().GetDbContext().FindAllAsync(saveDataType, condition, callback: processorCallback);
	}

	//------------------------------------------------------------------------------------------------
	protected static bool TypeAndSettingsValidation(out typename saveDataType)
	{
		typename resultType = TScriptedState;
		if (!resultType.IsInherited(EPF_PersistentScriptedState)) return false;

		EPF_PersistentScriptedStateSettings settings = EPF_PersistentScriptedStateSettings.Get(TScriptedState);
		if (!settings || !settings.m_tSaveDataType)
		{
			Debug.Error(string.Format("Scripted state type '%1' needs to have no save-data configured to be loaded!", TScriptedState));
			return false;
		}

		saveDataType = settings.m_tSaveDataType;

		return true;
	}
};


class EPF_ScriptedStateLoaderProcessorCallbackSingle : EDF_DbFindCallbackSingle<EPF_ScriptedStateSaveData>
{
	ref EPF_ScriptedStateLoaderCallbackInvokerBase m_pCallbackInvoker;
	bool m_bCreateSingleton;
	typename m_tCreateType;

	//------------------------------------------------------------------------------------------------
	override void OnSuccess(EPF_ScriptedStateSaveData result, Managed context)
	{
		EPF_PersistenceManager persistenceManager = EPF_PersistenceManager.GetInstance();

		EPF_PersistentScriptedState resultScriptedState;

		if (result)
		{
			resultScriptedState = EPF_PersistentScriptedState.Cast(persistenceManager.SpawnScriptedState(result))
		}
		else if (m_bCreateSingleton)
		{
			resultScriptedState = EPF_PersistentScriptedState.Cast(m_tCreateType.Spawn());
		}

		GetGame().GetScriptModule().Call(m_pCallbackInvoker, "Invoke", false, null, resultScriptedState);
	}

	//------------------------------------------------------------------------------------------------
	override void OnFailure(EDF_EDbOperationStatusCode statusCode, Managed context)
	{
		GetGame().GetScriptModule().Call(m_pCallbackInvoker, "Invoke", false, null, null);
	}

	//------------------------------------------------------------------------------------------------
	void Setup(EPF_ScriptedStateLoaderCallbackInvokerBase callbackInvoker, bool createSingleton, typename createType)
	{
		m_pCallbackInvoker = callbackInvoker;
		m_bCreateSingleton = createSingleton;
		m_tCreateType = createType;
	}
};

class EPF_ScriptedStateLoaderProcessorCallbackMultiple : EDF_DbFindCallbackMultiple<EPF_ScriptedStateSaveData>
{
	ref EPF_ScriptedStateLoaderCallbackInvokerBase m_pCallbackInvoker;

	//------------------------------------------------------------------------------------------------
	override void OnSuccess(array<ref EPF_ScriptedStateSaveData> results, Managed context)
	{
		EPF_PersistenceManager persistenceManager = EPF_PersistenceManager.GetInstance();

		array<ref EPF_PersistentScriptedState> resultReferences();
		foreach (EPF_ScriptedStateSaveData saveData : results)
		{
			EPF_PersistentScriptedState resultScriptedState = EPF_PersistentScriptedState.Cast(persistenceManager.SpawnScriptedState(saveData));
			if (resultScriptedState)
			{
				resultReferences.Insert(resultScriptedState); // Keep the instance alive until we invoked the callback
				GetGame().GetScriptModule().Call(m_pCallbackInvoker, "AddResult", false, null, resultScriptedState);
			}
		}

		GetGame().GetScriptModule().Call(m_pCallbackInvoker, "Invoke", false, null);
	}

	//------------------------------------------------------------------------------------------------
	override void OnFailure(EDF_EDbOperationStatusCode statusCode, Managed context)
	{
		GetGame().GetScriptModule().Call(m_pCallbackInvoker, "Invoke", false, null);
	}

	//------------------------------------------------------------------------------------------------
	void Setup(EPF_ScriptedStateLoaderCallbackInvokerBase callbackInvoker)
	{
		m_pCallbackInvoker = callbackInvoker;
	}
};

// Seperate invoker because we can't strong type the EDF_DataCallback via template directly until https://feedback.bistudio.com/T167295 is fixed.
class EPF_ScriptedStateLoaderCallbackInvokerBase
{
	ref EDF_Callback m_pCallback;

	//------------------------------------------------------------------------------------------------
	void EPF_ScriptedStateLoaderCallbackInvokerBase(EDF_Callback callback)
	{
		m_pCallback = callback;
	}
};

class EPF_ScriptedStateLoaderCallbackInvokerSingle<Class T> : EPF_ScriptedStateLoaderCallbackInvokerBase
{
	//------------------------------------------------------------------------------------------------
	void Invoke(EPF_PersistentScriptedState resultScriptedState)
	{
		T typedResult = T.Cast(resultScriptedState);
		auto typedCallback = EDF_DataCallbackSingle<T>.Cast(m_pCallback);
		if (typedCallback)
			typedCallback.Invoke(typedResult);
	}
};

class EPF_ScriptedStateLoaderCallbackInvokerMultiple<Class T> : EPF_ScriptedStateLoaderCallbackInvokerBase
{
	ref array<T> m_aResultBuffer = {};

	//------------------------------------------------------------------------------------------------
	void AddResult(EPF_PersistentScriptedState resultScriptedState)
	{
		T resultTyped = T.Cast(resultScriptedState);
		if (resultTyped)
			m_aResultBuffer.Insert(resultTyped);
	}

	//------------------------------------------------------------------------------------------------
	void Invoke()
	{
		auto typedCallback = EDF_DataCallbackMultiple<T>.Cast(m_pCallback);
		if (typedCallback)
			typedCallback.Invoke(m_aResultBuffer);
	}
};
