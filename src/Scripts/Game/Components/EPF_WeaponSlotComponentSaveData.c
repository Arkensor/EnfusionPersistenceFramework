[EPF_ComponentSaveDataType(WeaponSlotComponent), BaseContainerProps()]
class EPF_WeaponSlotComponentSaveDataClass : EPF_ComponentSaveDataClass
{
};

[EDF_DbName.Automatic()]
class EPF_WeaponSlotComponentSaveData : EPF_ComponentSaveData
{
	int m_iSlotIndex;
	ref EPF_EntitySaveData m_pEntity;

	//------------------------------------------------------------------------------------------------
	override EPF_EReadResult ReadFrom(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
	{
		WeaponSlotComponent slot = WeaponSlotComponent.Cast(component);
		IEntity slotEntity = slot.GetWeaponEntity();
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

		// Weapon slot does not expose any additive/override transform, so no need to touch it in save-data :)

		// We can safely ignore baked objects with default info on them, but anything else needs to be saved.
		if (attributes.m_bTrimDefaults &&
			isPrefabMatch &&
			EPF_BitFlags.CheckFlags(slotPersistence.GetFlags(), EPF_EPersistenceFlags.BAKED) &&
			readResult == EPF_EReadResult.DEFAULT)
		{
			return EPF_EReadResult.DEFAULT;
		}

		m_iSlotIndex = slot.GetWeaponSlotIndex();
		m_pEntity = saveData;
		return EPF_EReadResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override bool IsFor(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
	{
		WeaponSlotComponent slot = WeaponSlotComponent.Cast(component);
		return slot.GetWeaponSlotIndex() == m_iSlotIndex;
	}

	//------------------------------------------------------------------------------------------------
	override EPF_EApplyResult ApplyTo(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
	{
		WeaponSlotComponent slot = WeaponSlotComponent.Cast(component);
		IEntity slotEntity = slot.GetWeaponEntity();

		// Found matching entity, no need to spawn, just apply save-data
		if (m_pEntity &&
			slotEntity &&
			EPF_Utils.GetPrefabName(slotEntity).StartsWith(m_pEntity.m_rPrefab))
		{
			EPF_PersistenceComponent slotPersistence = EPF_Component<EPF_PersistenceComponent>.Find(slotEntity);
			if (slotPersistence && !slotPersistence.Load(m_pEntity, false))
				return EPF_EApplyResult.ERROR;

			return EPF_EApplyResult.OK;
		}

		// Slot did not match save-data, delete current entity on it
		SCR_EntityHelper.DeleteEntityAndChildren(slotEntity);

		if (!m_pEntity)
			return EPF_EApplyResult.OK;

		// Spawn new entity and attach it
		slotEntity = m_pEntity.Spawn(false);
		if (slotEntity)
		{
			slot.SetWeapon(slotEntity);
			if (slot.GetWeaponEntity() == slotEntity)
				return EPF_EApplyResult.OK;
		}

		return EPF_EApplyResult.ERROR;
	}

	//------------------------------------------------------------------------------------------------
	override bool Equals(notnull EPF_ComponentSaveData other)
	{
		EPF_WeaponSlotComponentSaveData otherData = EPF_WeaponSlotComponentSaveData.Cast(other);
		return (m_iSlotIndex == otherData.m_iSlotIndex) && ((!m_pEntity && !otherData.m_pEntity) || m_pEntity.Equals(otherData.m_pEntity));
	}

	//------------------------------------------------------------------------------------------------
	protected bool SerializationSave(BaseSerializationSaveContext saveContext)
	{
		if (!saveContext.IsValid())
			return false;

		saveContext.WriteValue("m_iSlotIndex", m_iSlotIndex);

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

		loadContext.ReadValue("m_iSlotIndex", m_iSlotIndex);

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
