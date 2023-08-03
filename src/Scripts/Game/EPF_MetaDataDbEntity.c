class EPF_MetaDataDbEntity : EDF_DbEntity
{
	int m_iDataLayoutVersion = 1;
	int m_iLastSaved;

	//------------------------------------------------------------------------------------------------
	//! Utility function to read meta-data
	void ReadMetaData(notnull EPF_PersistenceComponent persistenceComponent)
	{
		SetId(persistenceComponent.GetPersistentId());
		m_iLastSaved = persistenceComponent.GetLastSaved();
	}

	//------------------------------------------------------------------------------------------------
	//! Utility function to read meta-data
	void ReadMetaData(notnull EPF_PersistentScriptedState scriptedState)
	{
		SetId(scriptedState.GetPersistentId());
		m_iLastSaved = scriptedState.GetLastSaved();
	}

	//------------------------------------------------------------------------------------------------
	//! Utility function to write meta-data to serializer
	void SerializeMetaData(notnull BaseSerializationSaveContext saveContext)
	{
		WriteId(saveContext);

		if (m_iDataLayoutVersion != 1 || !ContainerSerializationSaveContext.Cast(saveContext).GetContainer().IsInherited(BaseJsonSerializationSaveContainer))
			saveContext.WriteValue("m_iDataLayoutVersion", m_iDataLayoutVersion);

		saveContext.WriteValue("m_iLastSaved", m_iLastSaved);
	}

	//------------------------------------------------------------------------------------------------
	//! Utility function to read meta-data from serializer
	void DeserializeMetaData(notnull BaseSerializationLoadContext loadContext)
	{
		ReadId(loadContext);
		loadContext.ReadValue("m_iDataLayoutVersion", m_iDataLayoutVersion);
		loadContext.ReadValue("m_iLastSaved", m_iLastSaved);
	}
}
