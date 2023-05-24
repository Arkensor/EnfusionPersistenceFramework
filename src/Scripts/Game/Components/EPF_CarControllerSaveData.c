[EPF_ComponentSaveDataType(CarControllerComponent), BaseContainerProps()]
class EPF_CarControllerSaveDataClass : EPF_VehicleControllerSaveDataClass
{
};

[EDF_DbName.Automatic()]
class EPF_CarControllerSaveData : EPF_VehicleControllerSaveData
{
	bool m_bHandBrake;

	//------------------------------------------------------------------------------------------------
	override EPF_EReadResult ReadFrom(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
	{
		EPF_EReadResult readResult = super.ReadFrom(owner, component, attributes);
		if (!readResult) return EPF_EReadResult.ERROR;

		CarControllerComponent carController = CarControllerComponent.Cast(component);
		m_bHandBrake = carController.GetPersistentHandBrake();

		if (readResult == EPF_EReadResult.DEFAULT && m_bHandBrake) return EPF_EReadResult.DEFAULT;
		return EPF_EReadResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override EPF_EApplyResult ApplyTo(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
	{
		if (!super.ApplyTo(owner, component, attributes)) return EPF_EApplyResult.ERROR;

		CarControllerComponent carController = CarControllerComponent.Cast(component);
		carController.SetPersistentHandBrake(m_bHandBrake);

		return EPF_EApplyResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override bool Equals(notnull EPF_ComponentSaveData other)
	{
		EPF_CarControllerSaveData otherData = EPF_CarControllerSaveData.Cast(other);
		return super.Equals(other) && m_bHandBrake == otherData.m_bHandBrake;
	}
};
