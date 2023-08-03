class EPF_PersistentDoorStateManager : EPF_PersistentScriptedState
{
	/*protected*/ ref map<string, float> m_mStates = new map<string, float>(); //TODO: Make protected again after 0.9.9.5X update

	protected EPF_PersistentDoorStateFilter m_pFilter;

	//------------------------------------------------------------------------------------------------
	static void UpdateDoorState(IEntity doorEntity)
	{
		EPF_PersistentDoorStateManagerComponent managerComponent = EPF_PersistentDoorStateManagerComponent.GetInstance();
		if (!managerComponent)
			return;

		EPF_PersistentDoorStateManager manager = managerComponent.GetPersistentDoorManager();
		if (!manager)
			return;

		DoorComponent door = EPF_Component<DoorComponent>.Find(doorEntity);
		if (!door)
			return;

		if (manager.m_pFilter && !manager.m_pFilter.IsMatch(doorEntity))
			return;

		string mapKey = EPF_Utils.GetPrefabGUID(EPF_Utils.GetPrefabName(doorEntity)) + "@" + doorEntity.GetOrigin().ToString(false);

		float controlValue = door.GetControlValue();
		if (float.AlmostEqual(controlValue, 0))
		{
			manager.m_mStates.Remove(mapKey);
		}
		else
		{
			manager.m_mStates.Set(mapKey, controlValue);
		}
	}

	//------------------------------------------------------------------------------------------------
	void SetFilterLogic(EPF_PersistentDoorStateFilter filter)
	{
		m_pFilter = filter;
	}

	//------------------------------------------------------------------------------------------------
	event void OnPersistenceLoaded()
	{
		foreach (string mapKey, float controlValue : m_mStates)
		{
			ResourceName prefab = mapKey.Substring(0, 16);
			vector origin = mapKey.Substring(17, mapKey.Length() - 17).ToVector();

			IEntity doorEntity = EPF_WorldUtils.FindNearestPrefab(prefab, origin, 0.5);
			DoorComponent door = EPF_Component<DoorComponent>.Find(doorEntity);
			if (!door)
			{
				Print(string.Format("Failed to restore door %1@%2 to controlValue: %3. Entity not found.", prefab, origin, controlValue), LogLevel.WARNING);
				continue;
			}

			door.SetControlValue(controlValue);
			Print(string.Format("Restored door %1@%2 to controlValue: %3.", prefab, origin, controlValue), LogLevel.VERBOSE);
		}

		// TODO: Replace hardcoded delay when we can use the max of all doors opening times: https://feedback.bistudio.com/T174535
		GetGame().GetCallqueue().CallLater(EPF_PersistentDoorStateManagerComponent.GetInstance().SetLoadingComplete, 1000);
	}
}
