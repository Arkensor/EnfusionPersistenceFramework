[EPF_ComponentSaveDataType(BaseSlotComponent), BaseContainerProps()]
class EPF_BaseSlotComponentSaveDataClass : EPF_ComponentSaveDataClass
{
};

[EDF_DbName.Automatic()]
class EPF_BaseSlotComponentSaveData : EPF_ComponentSaveData
{
	ref EPF_EntitySaveData m_pEntity;

	//------------------------------------------------------------------------------------------------
	override EPF_EReadResult ReadFrom(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
	{
		BaseSlotComponent slot = BaseSlotComponent.Cast(component);
		IEntity slotEntity = slot.GetAttachedEntity();
		ResourceName prefab = EPF_Utils.GetPrefabName(slotEntity);

		EPF_EntitySlotPrefabInfo prefabInfo = EPF_EntitySlotPrefabInfo.GetSlotInfo(owner, slot);
		bool isPrefabMatch = prefab == prefabInfo.GetEnabledSlotPrefab();

		EPF_PersistenceComponent slotPersistence = EPF_Component<EPF_PersistenceComponent>.Find(slotEntity);
		if (!slotPersistence)
		{
			// Slot is different from prefab and the new slot has no persistence component so we clear it.
			if (!isPrefabMatch)
				return EPF_EReadResult.OK;

			return EPF_EReadResult.DEFAULT;
		}

		EPF_EReadResult readResult;
		EPF_EntitySaveData saveData = slotPersistence.Save(readResult);
		if (!saveData)
			return EPF_EReadResult.ERROR;

		// Read transform to see if slot uses OverrideTransformLS set.
		// Reuse logic from slot manager that should be used instead of base slot anyway ...
		EPF_SlotManagerComponentSaveData.ReadTransform(slotEntity, saveData, prefabInfo, readResult);

		// We can safely ignore baked objects with default info on them, but anything else needs to be saved.
		if (attributes.m_bTrimDefaults &&
			isPrefabMatch &&
			EPF_BitFlags.CheckFlags(slotPersistence.GetFlags(), EPF_EPersistenceFlags.BAKED) &&
			readResult == EPF_EReadResult.DEFAULT)
		{
			return EPF_EReadResult.DEFAULT;
		}

		m_pEntity = saveData;
		return EPF_EReadResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override EPF_EApplyResult ApplyTo(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
	{
		BaseSlotComponent slot = BaseSlotComponent.Cast(component);
		return EPF_SlotManagerComponentSaveData.ApplySlot(slot.GetSlotInfo(), m_pEntity);
	}

	//------------------------------------------------------------------------------------------------
	override bool Equals(notnull EPF_ComponentSaveData other)
	{
		EPF_BaseSlotComponentSaveData otherData = EPF_BaseSlotComponentSaveData.Cast(other);
		return (!m_pEntity && !otherData.m_pEntity) || m_pEntity.Equals(otherData.m_pEntity);
	}

	//------------------------------------------------------------------------------------------------
	protected bool SerializationSave(BaseSerializationSaveContext saveContext)
	{
		if (!saveContext.IsValid())
			return false;

		string entityType = "EMPTY";
		if (m_pEntity)
			entityType = EDF_DbName.Get(m_pEntity.Type());

		saveContext.WriteValue("_type", entityType);

		if (entityType)
			saveContext.WriteValue("m_pEntity", m_pEntity);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected bool SerializationLoad(BaseSerializationLoadContext loadContext)
	{
		if (!loadContext.IsValid())
			return false;

		string entityTypeString;
		loadContext.ReadValue("_type", entityTypeString);

		// TODO: Remove backwards compatiblity in 0.9.9
		if (!entityTypeString && ContainerSerializationLoadContext.Cast(loadContext).GetContainer().IsInherited(JsonLoadContainer))
			loadContext.ReadValue("entityType", entityTypeString);

		if (entityTypeString == "EMPTY")
			return true;

		typename entityType = EDF_DbName.GetTypeByName(entityTypeString);
		if (!entityType)
			return false;

		m_pEntity = EPF_EntitySaveData.Cast(entityType.Spawn());
		loadContext.ReadValue("m_pEntity", m_pEntity);

		return true;
	}
};
