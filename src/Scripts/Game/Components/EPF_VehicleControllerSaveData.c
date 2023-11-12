[EPF_ComponentSaveDataType(VehicleControllerComponent), EPF_ComponentSaveDataType(VehicleControllerComponent_SA), BaseContainerProps()]
class EPF_VehicleControllerSaveDataClass : EPF_ComponentSaveDataClass
{
}

[EDF_DbName.Automatic()]
class EPF_VehicleControllerSaveData : EPF_ComponentSaveData
{
	bool m_bEngineOn;

	//------------------------------------------------------------------------------------------------
	override EPF_EReadResult ReadFrom(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
	{
		VehicleControllerComponent_SA vehicleController_SA = VehicleControllerComponent_SA.Cast(component);
		if (vehicleController_SA)
		{
			m_bEngineOn = vehicleController_SA.IsEngineOn();
		}
		else
		{
			VehicleControllerComponent vehicleController = VehicleControllerComponent.Cast(component);
			if (vehicleController)
				m_bEngineOn = vehicleController.IsEngineOn();
		}

		if (!m_bEngineOn)
			return EPF_EReadResult.DEFAULT;

		return EPF_EReadResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override EPF_EApplyResult ApplyTo(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
	{
		VehicleControllerComponent_SA vehicleController_SA = VehicleControllerComponent_SA.Cast(component);
		if (vehicleController_SA)
		{
			if (m_bEngineOn)
				vehicleController_SA.ForceStartEngine();
		}
		else
		{
			VehicleControllerComponent vehicleController = VehicleControllerComponent.Cast(component);
			if (vehicleController && m_bEngineOn)
				vehicleController.StartEngine();
		}

		return EPF_EApplyResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override bool Equals(notnull EPF_ComponentSaveData other)
	{
		EPF_VehicleControllerSaveData otherData = EPF_VehicleControllerSaveData.Cast(other);
		return m_bEngineOn == otherData.m_bEngineOn;
	}
}
