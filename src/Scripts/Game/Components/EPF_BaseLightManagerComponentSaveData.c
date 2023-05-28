[EPF_ComponentSaveDataType(BaseLightManagerComponent), BaseContainerProps()]
class EPF_BaseLightManagerComponentSaveDataClass : EPF_ComponentSaveDataClass
{
};

[EDF_DbName.Automatic()]
class EPF_BaseLightManagerComponentSaveData : EPF_ComponentSaveData
{
	ref array<ref EPF_PersistentLightSlot> m_aLightSlots;

	//------------------------------------------------------------------------------------------------
	override EPF_EReadResult ReadFrom(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
	{
		BaseLightManagerComponent lightManager = BaseLightManagerComponent.Cast(component);

		m_aLightSlots = {};

		array<BaseLightSlot> lightSlots();
		lightManager.GetLights(lightSlots);
		foreach (BaseLightSlot lightSlot : lightSlots)
		{
			EPF_PersistentLightSlot persistentLightSlot();
			persistentLightSlot.m_eType = lightSlot.GetLightType();
			persistentLightSlot.m_iSide = lightSlot.GetLightSide();
			persistentLightSlot.m_bFunctional = lightSlot.IsLightFunctional();
			persistentLightSlot.m_bState = lightManager.GetLightsState(persistentLightSlot.m_eType, persistentLightSlot.m_iSide);

			// Skip slots when trimming and the are not broken and not turned on
			if (attributes.m_bTrimDefaults && persistentLightSlot.m_bFunctional && !persistentLightSlot.m_bState)
				continue;

			m_aLightSlots.Insert(persistentLightSlot);
		}

		if (m_aLightSlots.IsEmpty())
			return EPF_EReadResult.DEFAULT;

		return EPF_EReadResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override EPF_EApplyResult ApplyTo(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
	{
		BaseLightManagerComponent lightManager = BaseLightManagerComponent.Cast(component);

		array<BaseLightSlot> lightSlots();
		lightManager.GetLights(lightSlots);
		foreach (EPF_PersistentLightSlot persistentLightSlot : m_aLightSlots)
		{
			foreach (int idx, BaseLightSlot lightSlot : lightSlots)
			{
				if (lightSlot.GetLightType() != persistentLightSlot.m_eType ||
					lightSlot.GetLightSide() != persistentLightSlot.m_iSide) continue;

				lightSlot.SetLightFunctional(persistentLightSlot.m_bFunctional);

				// TODO: Remove this hacky fix after https://feedback.bistudio.com/T171832 has been adressed
				ELightType lightType = persistentLightSlot.m_eType;
				if (lightSlot.IsPresence())
					lightType = ELightType.Presence;

				lightManager.SetLightsState(lightType, persistentLightSlot.m_bState, persistentLightSlot.m_iSide);
				lightSlots.Remove(idx);

				break;
			}
		}

		return EPF_EApplyResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override bool Equals(notnull EPF_ComponentSaveData other)
	{
		EPF_BaseLightManagerComponentSaveData otherData = EPF_BaseLightManagerComponentSaveData.Cast(other);

		if (m_aLightSlots.Count() != otherData.m_aLightSlots.Count())
			return false;

		foreach (int idx, EPF_PersistentLightSlot lightSlot : m_aLightSlots)
		{
			// Try same index first as they are likely to be the correct ones.
			if (lightSlot.Equals(otherData.m_aLightSlots.Get(idx)))
				continue;

			bool found;
			foreach (int compareIdx, EPF_PersistentLightSlot otherLightSlot : otherData.m_aLightSlots)
			{
				if (compareIdx == idx)
					continue; // Already tried in idx direct compare

				if (lightSlot.Equals(otherLightSlot))
				{
					found = true;
					break;
				}
			}

			if (!found)
				return false;
		}

		return true;
	}
};

class EPF_PersistentLightSlot
{
	ELightType m_eType;
	int m_iSide;
	bool m_bFunctional;
	bool m_bState;

	//------------------------------------------------------------------------------------------------
	bool Equals(notnull EPF_PersistentLightSlot other)
	{
		return m_eType == other.m_eType &&
			m_iSide == other.m_iSide &&
			m_bFunctional == other.m_bFunctional &&
			m_bState == other.m_bState;
	}
};
