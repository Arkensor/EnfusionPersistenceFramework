[EPF_ComponentSaveDataType(MuzzleInMagComponent), BaseContainerProps()]
class EPF_MuzzleInMagComponentSaveDataClass : EPF_BaseMuzzleComponentSaveDataClass
{
};

[EDF_DbName.Automatic()]
class EPF_MuzzleInMagComponentSaveData : EPF_BaseMuzzleComponentSaveData
{
	//------------------------------------------------------------------------------------------------
	override protected bool IsDefaultChambered(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
	{
		MuzzleInMagComponent muzzleInMag = MuzzleInMagComponent.Cast(component);
		BaseContainer muzzleInMagSource = muzzleInMag.GetComponentSource(owner);

		int initialAmmo;
		muzzleInMagSource.Get("InitialAmmo", initialAmmo);

		ResourceName ammoTemplate;
		if (initialAmmo > 0)
			muzzleInMagSource.Get("AmmoTemplate", ammoTemplate);

		return ammoTemplate;
	}
};
