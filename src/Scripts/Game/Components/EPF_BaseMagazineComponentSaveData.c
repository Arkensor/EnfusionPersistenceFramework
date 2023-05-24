[EPF_ComponentSaveDataType(BaseMagazineComponent), BaseContainerProps()]
class EPF_BaseMagazineComponentSaveDataClass : EPF_ComponentSaveDataClass
{
};

[EDF_DbName.Automatic()]
class EPF_BaseMagazineComponentSaveData : EPF_ComponentSaveData
{
	int m_iAmmoCount;

	//------------------------------------------------------------------------------------------------
	override EPF_EReadResult ReadFrom(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
	{
		BaseMagazineComponent magazine = BaseMagazineComponent.Cast(component);
		m_iAmmoCount = magazine.GetAmmoCount();

		int maxAmmo = magazine.GetMaxAmmoCount();
		if (magazine.IsUsed())
		{
			BaseMuzzleComponent parentMuzzle = EPF_Component<BaseMuzzleComponent>.Find(owner.GetParent());
			if (parentMuzzle)
				maxAmmo -= parentMuzzle.GetBarrelsCount();
		}

		if (m_iAmmoCount >= maxAmmo)
			return EPF_EReadResult.DEFAULT;

		return EPF_EReadResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override EPF_EApplyResult ApplyTo(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
	{
		BaseMagazineComponent magazine = BaseMagazineComponent.Cast(component);
		magazine.SetAmmoCount(m_iAmmoCount);
		return EPF_EApplyResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override bool Equals(notnull EPF_ComponentSaveData other)
	{
		EPF_BaseMagazineComponentSaveData otherData = EPF_BaseMagazineComponentSaveData.Cast(other);
		return m_iAmmoCount == otherData.m_iAmmoCount;
	}
};
