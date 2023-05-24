[EntityEditorProps(category: "Persistence", description: "Global storage manager for ground item management. Requires the matching component to be attached")]
class EPF_GlobalInventoryStorageManagerClass : GenericEntityClass
{
};

class EPF_GlobalInventoryStorageManager : GenericEntity
{
};

[ComponentEditorProps(category: "Persistence", description: "Logic for the global storage manager, should only be added to and instance of EPF_GlobalInventoryStorageManager")]
class EPF_GlobalInventoryStorageManagerComponentClass : InventoryStorageManagerComponentClass
{
};

class EPF_GlobalInventoryStorageManagerComponent : InventoryStorageManagerComponent
{
	protected static const ResourceName PREFAB = "{B5D2C533815D511D}Prefabs/GlobalInventoryStorageManager.et";
	protected static EPF_GlobalInventoryStorageManagerComponent s_pInstance;

	//------------------------------------------------------------------------------------------------
	static EPF_GlobalInventoryStorageManagerComponent GetInstance()
	{
		if (!s_pInstance && Replication.IsServer())
		{
			EPF_GlobalInventoryStorageManager storageManager = EPF_GlobalInventoryStorageManager.Cast(EPF_Utils.SpawnEntityPrefab(PREFAB, "0 100 0"));
			if (!storageManager)
				return null;

			s_pInstance = EPF_GlobalInventoryStorageManagerComponent.Cast(storageManager.FindComponent(EPF_GlobalInventoryStorageManagerComponent));
			if (!s_pInstance)
				SCR_EntityHelper.DeleteEntityAndChildren(storageManager);
		}

		return s_pInstance;
	}
};
