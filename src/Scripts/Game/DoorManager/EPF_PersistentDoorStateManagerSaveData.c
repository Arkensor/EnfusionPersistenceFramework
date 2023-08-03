[
	EPF_PersistentScriptedStateSettings(EPF_PersistentDoorStateManager),
	EDF_DbName.Automatic()
]
class EPF_PersistentDoorStateManagerSaveData : EPF_ScriptedStateSaveData
{
	ref map<string, float> m_mStates;

	//------------------------------------------------------------------------------------------------
	override EPF_EReadResult ReadFrom(notnull Managed scriptedState)
	{
		auto doorStateManager = EPF_PersistentDoorStateManager.Cast(scriptedState);
		ReadMetaData(doorStateManager);
		m_mStates = doorStateManager.m_mStates;

		if (m_mStates.IsEmpty())
			return EPF_EReadResult.DEFAULT;

		return EPF_EReadResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override EPF_EApplyResult ApplyTo(notnull Managed scriptedState)
	{
		auto doorStateManager = EPF_PersistentDoorStateManager.Cast(scriptedState);
		doorStateManager.m_mStates = m_mStates;
		doorStateManager.OnPersistenceLoaded();
		return EPF_EApplyResult.AWAIT_COMPLETION;
	}
}
