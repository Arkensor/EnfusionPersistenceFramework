[
	EPF_PersistentScriptedStateSettings(EPF_PersistentDoorStateManager),
	EDF_DbName.Automatic()
]
class EPF_PersistentDoorStateManagerSaveData : EPF_ScriptedStateSaveData
{
	ref map<string, ref EPF_PersistentDoorStateStruct> m_mStates;

	//------------------------------------------------------------------------------------------------
	override EPF_EReadResult ReadFrom(notnull Managed scriptedState)
	{
		auto doorStateManager = EPF_PersistentDoorStateManager.Cast(scriptedState);
		ReadMetaData(doorStateManager);

		m_mStates = new map<string, ref EPF_PersistentDoorStateStruct>();
		foreach (Tuple2<IEntity, DoorComponent> dataTuple : doorStateManager.m_aTrackedDoors)
		{
			if (!dataTuple.param1 || !dataTuple.param2)
				continue;

			EPF_PersistentDoorStateStruct state();
			if (state.ReadFrom(dataTuple.param1, dataTuple.param2) == EPF_EReadResult.DEFAULT)
				continue;

			string mapKey = EPF_Utils.GetPrefabGUID(EPF_Utils.GetPrefabName(dataTuple.param1)) + "@" + dataTuple.param1.GetOrigin().ToString(false);
			m_mStates.Set(mapKey, state);
		}

		if (m_mStates.IsEmpty())
			return EPF_EReadResult.DEFAULT;

		return EPF_EReadResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override EPF_EApplyResult ApplyTo(notnull Managed scriptedState)
	{
		auto doorStateManager = EPF_PersistentDoorStateManager.Cast(scriptedState);

		foreach (string mapKey, EPF_PersistentDoorStateStruct state : m_mStates)
		{
			ResourceName prefab = mapKey.Substring(0, 16);
			vector origin = mapKey.Substring(17, mapKey.Length() - 17).ToVector();

			IEntity doorEntity = EPF_WorldUtils.FindNearestPrefab(prefab, origin, 0.5);
			DoorComponent door = EPF_Component<DoorComponent>.Find(doorEntity);
			if (!door)
			{
				Print(string.Format("Failed to restore door %1@%2. Entity not found.", prefab, origin), LogLevel.WARNING);
				continue;
			}

			if (doorStateManager.m_pFilter && !doorStateManager.m_pFilter.IsMatch(doorEntity))
				Print(string.Format("Skipped restoring door %1@%2. Filter does not match anymore.", prefab, origin), LogLevel.VERBOSE);

			if (state.ApplyTo(doorEntity, door))
			{
				doorStateManager.m_aTrackedDoors.Insert(new Tuple2<IEntity, DoorComponent>(doorEntity, door));
				Print(string.Format("Restored door %1@%2", prefab, origin), LogLevel.VERBOSE);
			}
		}

		// TODO: Replace hardcoded delay when we can use the max of all doors opening times: https://feedback.bistudio.com/T174535
		GetGame().GetCallqueue().CallLater(EPF_PersistentDoorStateManagerComponent.GetInstance().SetLoadingComplete, 1000);
		return EPF_EApplyResult.AWAIT_COMPLETION;
	}

	//------------------------------------------------------------------------------------------------
	protected bool SerializationSave(BaseSerializationSaveContext saveContext)
	{
		m_iDataLayoutVersion = 2;
		SerializeMetaData(saveContext);
		saveContext.WriteValue("m_mStates", m_mStates);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected bool SerializationLoad(BaseSerializationLoadContext loadContext)
	{
		DeserializeMetaData(loadContext);

		if (m_iDataLayoutVersion == 1)
		{
			map<string, float> mapV1();
			loadContext.ReadValue("m_mStates", mapV1);
			m_mStates = new map<string, ref EPF_PersistentDoorStateStruct>();
			foreach (string key, float controlValue : mapV1)
			{
				EPF_PersistentDoorStateStruct state();
				state.m_fControlValue = controlValue;
				m_mStates.Set(key, state);
			}
		}
		else
		{
			loadContext.ReadValue("m_mStates", m_mStates);
		}

		return true;
	}
}

class EPF_PersistentDoorStateStruct
{
	float m_fControlValue;

	//------------------------------------------------------------------------------------------------
	EPF_EReadResult ReadFrom(notnull IEntity doorOwner, notnull DoorComponent door)
	{
		m_fControlValue = door.GetControlValue();

		if (float.AlmostEqual(m_fControlValue, 0))
			return EPF_EReadResult.DEFAULT;

		return EPF_EReadResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	EPF_EApplyResult ApplyTo(notnull IEntity doorOwner, notnull DoorComponent door)
	{
		door.SetControlValue(m_fControlValue);
		return EPF_EApplyResult.AWAIT_COMPLETION;
	}
}
