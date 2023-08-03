[EPF_ComponentSaveDataType(CompartmentAccessComponent), BaseContainerProps()]
class EPF_CompartmentAccessComponentSaveDataClass : EPF_ComponentSaveDataClass
{
	[Attribute(defvalue: "50", desc: "Maximum range the vehicle can have moved for the character to spawn back in.\nIf further away character will spawn where he was last in the world.\n-1 To disable the check.")]
	int m_iMaxPassangerLoadRange;
};

[EDF_DbName.Automatic()]
class EPF_CompartmentAccessComponentSaveData : EPF_ComponentSaveData
{
	string m_sEntity;
	int m_iSlotIdx;

	//------------------------------------------------------------------------------------------------
	override EPF_EReadResult ReadFrom(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
	{
		CompartmentAccessComponent compartment = CompartmentAccessComponent.Cast(component);
		BaseCompartmentSlot comparmentSlot = compartment.GetCompartment();
		if (comparmentSlot)
		{
			IEntity slotOwner = comparmentSlot.GetOwner();
			BaseCompartmentManagerComponent compartmentManager = EPF_Component<BaseCompartmentManagerComponent>.Find(slotOwner);
			while (compartmentManager)
			{
				IEntity parent = slotOwner.GetParent();
				BaseCompartmentManagerComponent parentManager = EPF_Component<BaseCompartmentManagerComponent>.Find(parent);
				if (!parentManager)
					break;

				slotOwner = parent;
				compartmentManager = parentManager;
			}

			if (compartmentManager)
			{
				array<BaseCompartmentSlot> outCompartments();
				compartmentManager.GetCompartments(outCompartments);
				foreach (int idx, BaseCompartmentSlot slot : outCompartments)
				{
					if (slot == comparmentSlot)
					{
						m_sEntity = EPF_PersistenceComponent.GetPersistentId(slotOwner);
						m_iSlotIdx = idx;
					}
				}
			}
		}
		else
		{
			m_iSlotIdx = -1;
		}

		if (!m_sEntity || m_iSlotIdx == -1)
			return EPF_EReadResult.DEFAULT;

		return EPF_EReadResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override EPF_EApplyResult ApplyTo(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
	{
		EPF_CompartmentAccessComponentSaveDataClass settings = EPF_CompartmentAccessComponentSaveDataClass.Cast(attributes);
		CompartmentAccessComponent compartment = CompartmentAccessComponent.Cast(component);

		vector currentPos = owner.GetOrigin();

		// compartment holder and compartment entity can not be too far apart.
		IEntity compartmentHolder = EPF_PersistenceManager.GetInstance().FindEntityByPersistentId(m_sEntity);
		BaseCompartmentManagerComponent compartmentManager = EPF_Component<BaseCompartmentManagerComponent>.Find(compartmentHolder);
		if (compartmentManager && 
			(settings.m_iMaxPassangerLoadRange == -1 || 
			vector.Distance(compartmentHolder.GetOrigin(), currentPos) <= settings.m_iMaxPassangerLoadRange))
		{
			array<BaseCompartmentSlot> outCompartments();
			compartmentManager.GetCompartments(outCompartments);
			if (m_iSlotIdx < outCompartments.Count())
			{
				compartment.MoveInVehicle(compartmentHolder, outCompartments.Get(m_iSlotIdx));
				return EPF_EApplyResult.OK;
			}
		}

		// No longer able to enter compartment so find free position on the ground to spawn instead
		if (SCR_WorldTools.FindEmptyTerrainPosition(currentPos, currentPos, 50, cylinderHeight: 1000))
			EPF_WorldUtils.Teleport(owner, currentPos);

		return EPF_EApplyResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override bool Equals(notnull EPF_ComponentSaveData other)
	{
		EPF_CompartmentAccessComponentSaveData otherData = EPF_CompartmentAccessComponentSaveData.Cast(other);
		return true;
	}
};
