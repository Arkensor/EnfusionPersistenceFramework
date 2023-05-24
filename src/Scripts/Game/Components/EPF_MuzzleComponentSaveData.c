[EPF_ComponentSaveDataType(MuzzleComponent), BaseContainerProps()]
class EPF_MuzzleComponentSaveDataClass : EPF_BaseMuzzleComponentSaveDataClass
{
};

[EDF_DbName.Automatic()]
class EPF_MuzzleComponentSaveData : EPF_BaseMuzzleComponentSaveData
{
	//------------------------------------------------------------------------------------------------
	override protected bool IsDefaultChambered(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
	{
		EPF_PersistenceComponent persistence = EPF_Component<EPF_PersistenceComponent>.Find(owner);
		return MuzzleComponent.Cast(component).GetDefaultMagazinePrefab() &&
			EPF_BitFlags.CheckFlags(persistence.GetFlags(), EPF_EPersistenceFlags.WAS_SELECTED);
	}
};
