[ComponentEditorProps(category: "Persistence", description: "Optional persistance handling of doors.")]
class EPF_PersistentDoorStateManagerComponentClass : EPF_PersistenceManagerExtensionBaseComponentClass
{
	[Attribute(desc: "Optional filter which door entities will be tracked by the manager.")]
	ref EPF_PersistentDoorStateFilter m_pFilter;
}

class EPF_PersistentDoorStateManagerComponent : EPF_PersistenceManagerExtensionBaseComponent
{
	protected static EPF_PersistentDoorStateManagerComponent m_pInstance;
	protected ref EPF_PersistentDoorStateManager m_pDoorStateManager;
	protected bool m_bLoaded;

	//------------------------------------------------------------------------------------------------
	override void OnSetup(EPF_PersistenceManager persistenceManager)
	{
		m_pInstance = this;
		EDF_DataCallbackSingle<EPF_PersistentDoorStateManager> callback(this, "OnDoorStateManagerLoaded");
		EPF_PersistentScriptedStateLoader<EPF_PersistentDoorStateManager>.LoadAsync(GetDoorStateManagerPersistentId(), callback);
	}

	//------------------------------------------------------------------------------------------------
	override bool IsSetupComplete(EPF_PersistenceManager persistenceManager)
	{
		return m_bLoaded;
	}

	//------------------------------------------------------------------------------------------------
	protected static string GetDoorStateManagerPersistentId()
	{
		return string.Format("00dc%1-0000-0000-0000-000000000000", EPF_PersistenceIdGenerator.GetHiveId().ToString(4));
	}

	//------------------------------------------------------------------------------------------------
	/*protected --Hotfix for 1.0 DO NOT CALL THIS MANUALLY*/
	void OnDoorStateManagerLoaded(EPF_PersistentDoorStateManager doorStateManager)
	{
		m_pDoorStateManager = doorStateManager;
		if (!m_pDoorStateManager)
		{
			m_pDoorStateManager = new EPF_PersistentDoorStateManager();
			m_pDoorStateManager.SetPersistentId(GetDoorStateManagerPersistentId());
			m_bLoaded = true;
		}

		auto settings = EPF_PersistentDoorStateManagerComponentClass.Cast(GetComponentData(GetOwner()));
		m_pDoorStateManager.m_pFilter = settings.m_pFilter;
	}

	//------------------------------------------------------------------------------------------------
	static EPF_PersistentDoorStateManagerComponent GetInstance()
	{
		return m_pInstance;
	}

	//------------------------------------------------------------------------------------------------
	void SetLoadingComplete()
	{
		m_bLoaded = true;
	}

	//------------------------------------------------------------------------------------------------
	EPF_PersistentDoorStateManager GetPersistentDoorManager()
	{
		return m_pDoorStateManager;
	}
}
