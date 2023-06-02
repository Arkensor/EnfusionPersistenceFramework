class EPF_DbFind : EDF_DbFind
{
	//------------------------------------------------------------------------------------------------
	static EPF_ComponentSelectorBuilder Component(typename componentType)
	{
		return new EPF_ComponentSelectorBuilder(EDF_DbFindFieldCollectionHandlingBuilder.Create()
			.Field("m_aComponents")
			.OfType(componentType));
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
	protected ref EDF_DbFindFieldBasicCollectionHandlingBuilder m_pBuilder;

	//------------------------------------------------------------------------------------------------
	EPF_ComponentFieldBuilder Any()
	{
		return new EPF_ComponentFieldBuilder(m_pBuilder.Any());
	}

	//------------------------------------------------------------------------------------------------
	EPF_ComponentFieldBuilder All()
	{
		return new EPF_ComponentFieldBuilder(m_pBuilder.All());
	}

	//------------------------------------------------------------------------------------------------
	void EPF_ComponentSelectorBuilder(EDF_DbFindFieldBasicCollectionHandlingBuilder builder)
	{
		m_pBuilder = builder;
	}
};
