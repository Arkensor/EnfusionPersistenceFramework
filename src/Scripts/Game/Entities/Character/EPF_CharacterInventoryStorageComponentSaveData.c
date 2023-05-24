[EPF_ComponentSaveDataType(SCR_CharacterInventoryStorageComponent), BaseContainerProps()]
class EPF_CharacterInventoryStorageComponentSaveDataClass : EPF_BaseInventoryStorageComponentSaveDataClass
{
	[Attribute(defvalue: "10", uiwidget: UIWidgets.Slider, desc: "Maximum time until the quickbar is synced after a change in SECONDS. Higher values reduce traffic.", params: "1 1000 1")]
	int m_iMaxQuickbarSaveTime;
};

[EDF_DbName.Automatic()]
class EPF_CharacterInventoryStorageComponentSaveData : EPF_BaseInventoryStorageComponentSaveData
{
	ref array<ref EPF_PersistentQuickSlotItem> m_aQuickSlotEntities;

	//------------------------------------------------------------------------------------------------
	override EPF_EReadResult ReadFrom(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
	{
		EPF_EReadResult result = super.ReadFrom(owner, component, attributes);
		if (!result)
			return EPF_EReadResult.ERROR;

		m_aQuickSlotEntities = {};

		SCR_CharacterInventoryStorageComponent inventoryStorage = SCR_CharacterInventoryStorageComponent.Cast(component);
		foreach (int idx, IEntity item : inventoryStorage.GetQuickSlotItems())
		{
			string persistentId = EPF_PersistenceComponent.GetPersistentId(item);
			if (!persistentId) continue;

			EPF_PersistentQuickSlotItem slot();
			slot.m_iIndex = idx;
			slot.m_sEntityId = persistentId;
			m_aQuickSlotEntities.Insert(slot);
		}

		if (result == EPF_EReadResult.DEFAULT && m_aQuickSlotEntities.IsEmpty())
			return EPF_EReadResult.DEFAULT;

		return EPF_EReadResult.OK;
	}

	//! >>> "ApplyTo" happens in modded SCR_RespawnComponent as it needs to be done on the client and data is sent via RPC. <<<
};

class EPF_PersistentQuickSlotItem
{
	int m_iIndex;
	string m_sEntityId;
};
