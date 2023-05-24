[BaseContainerProps()]
class EPF_BaseMuzzleComponentSaveDataClass : EPF_ComponentSaveDataClass
{
};

[EDF_DbName.Automatic()]
class EPF_BaseMuzzleComponentSaveData : EPF_ComponentSaveData
{
	EMuzzleType m_eMuzzleType;
	ref array<bool> m_aChamberStatus;

	//------------------------------------------------------------------------------------------------
	override EPF_EReadResult ReadFrom(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
	{
		BaseMuzzleComponent muzzle = BaseMuzzleComponent.Cast(component);
		int barrelsCount = muzzle.GetBarrelsCount();

		m_eMuzzleType = muzzle.GetMuzzleType();
		m_aChamberStatus = {};
		m_aChamberStatus.Reserve(barrelsCount);

		bool isDefaultChambered = IsDefaultChambered(owner, component, attributes);
		bool isDefault = true;
		for (int nBarrel = 0; nBarrel < barrelsCount; nBarrel++)
		{
			bool isChambered = muzzle.IsBarrelChambered(nBarrel);
			if (isChambered != isDefaultChambered)
				isDefault = false;

			m_aChamberStatus.Insert(isChambered);
		}

		if (isDefault)
			return EPF_EReadResult.DEFAULT;

		return EPF_EReadResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsDefaultChambered(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes);

	//------------------------------------------------------------------------------------------------
	override EPF_EApplyResult ApplyTo(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
	{
		BaseMuzzleComponent muzzle = BaseMuzzleComponent.Cast(component);

		foreach (int idx, bool chambered : m_aChamberStatus)
		{
			if (!chambered)
				muzzle.ClearChamber(idx);
		}

		return EPF_EApplyResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override bool IsFor(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
	{
		BaseMuzzleComponent muzzle = BaseMuzzleComponent.Cast(component);
		return muzzle.GetMuzzleType() == m_eMuzzleType && muzzle.GetBarrelsCount() == m_aChamberStatus.Count();
	}

	//------------------------------------------------------------------------------------------------
	override bool Equals(notnull EPF_ComponentSaveData other)
	{
		EPF_BaseMuzzleComponentSaveData otherData = EPF_BaseMuzzleComponentSaveData.Cast(other);

		if (m_eMuzzleType != otherData.m_eMuzzleType ||
			m_aChamberStatus.Count() != otherData.m_aChamberStatus.Count())
			return false;

		foreach (int idx, bool chambered : m_aChamberStatus)
		{
			if (chambered != m_aChamberStatus.Get(idx))
				return false;
		}

		return true;
	}
};
