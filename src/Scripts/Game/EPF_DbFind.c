class EPF_DbFind : EDF_DbFind
{
	// TODO: Remove this and all other ALLOC_BUFFER when https://feedback.bistudio.com/T173058  is fixed
	protected static ref array<ref EPF_ComponentSelectorBuilder>> ALLOC_BUFFER;

	//------------------------------------------------------------------------------------------------
	static EPF_ComponentSelectorBuilder Component(typename componentType)
	{
		// Is already save-data type?
		if (!componentType.IsInherited(EPF_ComponentSaveData))
		{
			// Is already save-data-class type?
			if (!componentType.IsInherited(EPF_ComponentSaveDataClass))
				componentType = EPF_ComponentSaveDataType.GetSaveDataType(componentType);

			componentType = EPF_Utils.TrimEnd(componentType.ToString(), 5).ToType();
		}

		if (!componentType)
		{
			Debug.Error("Can not create component db find condition before all component types are known. Consider moving creation of this condition to runtime (non static/const).");
			return null;
		}

		auto inst = EPF_ComponentSelectorBuilder(componentType);
		if (!ALLOC_BUFFER) ALLOC_BUFFER = {null};
		ALLOC_BUFFER.Set(0, inst);
		return inst;
	}
};

modded class EDF_DbFindFieldMainConditionBuilder
{
	//------------------------------------------------------------------------------------------------
	EDF_DbFindCondition PrefabEquals(ResourceName prefab)
	{
		string prefabString = prefab;
		if (prefabString.StartsWith("{"))
			prefabString = EPF_Utils.GetPrefabGUID(prefab);

		// Db might contain full paths so we need to do only contains check to cover both cases
		#ifdef PERSISTENCE_DEBUG
		return Field("m_rPrefab").Contains(prefabString);
		#endif

		// Assume only GUID of prefabs in non debug mode.
		return Field("m_rPrefab").Equals(prefabString);
	}

};

class EPF_ComponentFieldBuilder
{
	protected ref EDF_DbFindFieldMainConditionBuilder m_pBuilder;

	//------------------------------------------------------------------------------------------------
	EDF_DbFindFieldCollectionHandlingBuilder Field(string fieldPath)
	{
		return EDF_DbFindFieldCollectionHandlingBuilder.Cast(m_pBuilder.Field(fieldPath));
	}

	//------------------------------------------------------------------------------------------------
	void EPF_ComponentFieldBuilder(EDF_DbFindFieldMainConditionBuilder builder)
	{
		m_pBuilder = builder;
	}
};

class EPF_ComponentSelectorBuilder
{
	protected static ref array<ref EPF_ComponentFieldBuilder>> ALLOC_BUFFER;

	protected typename m_tComponentType;

	//------------------------------------------------------------------------------------------------
	EPF_ComponentFieldBuilder Any()
	{
		auto inst = new EPF_ComponentFieldBuilder(EDF_DbFindFieldFinalBuilder.Create()
			.Field("m_aComponents")
			.OfType(m_tComponentType)
			.Any()
			.Field("m_pData"));

		if (!ALLOC_BUFFER) ALLOC_BUFFER = {null};
		ALLOC_BUFFER.Set(0, inst);
		return inst;
	}

	//------------------------------------------------------------------------------------------------
	EPF_ComponentFieldBuilder All()
	{
		auto inst = EPF_ComponentFieldBuilder(EDF_DbFindFieldFinalBuilder.Create()
			.Field("m_aComponents")
			.OfType(m_tComponentType)
			.All()
			.Field("m_pData"));

		if (!ALLOC_BUFFER) ALLOC_BUFFER = {null};
		ALLOC_BUFFER.Set(0, inst);
		return inst;
	}

	//------------------------------------------------------------------------------------------------
	void EPF_ComponentSelectorBuilder(typename componentType)
	{
		m_tComponentType = componentType;
	}
};
