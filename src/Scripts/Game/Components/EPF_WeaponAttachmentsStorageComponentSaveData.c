[EPF_ComponentSaveDataType(WeaponAttachmentsStorageComponent), BaseContainerProps()]
class EPF_WeaponAttachmentsStorageComponentSaveDataClass : EPF_BaseInventoryStorageComponentSaveDataClass
{
	//------------------------------------------------------------------------------------------------
	override array<typename> Requires()
	{
		return {EPF_BaseMuzzleComponentSaveDataClass};
	}
};

[EDF_DbName.Automatic()]
class EPF_WeaponAttachmentsStorageComponentSaveData : EPF_BaseInventoryStorageComponentSaveData
{
};
