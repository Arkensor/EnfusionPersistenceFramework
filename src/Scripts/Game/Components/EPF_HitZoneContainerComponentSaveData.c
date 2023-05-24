[EPF_ComponentSaveDataType(HitZoneContainerComponent), BaseContainerProps()]
class EPF_HitZoneContainerComponentSaveDataClass : EPF_ComponentSaveDataClass
{
	[Attribute(desc: "If set, only the explictly selected hitzones are persisted.")]
	ref array<string> m_aHitzoneFilter;
};

[EDF_DbName.Automatic()]
class EPF_HitZoneContainerComponentSaveData : EPF_ComponentSaveData
{
	ref array<ref EPF_PersistentHitZone> m_aHitzones;

	//------------------------------------------------------------------------------------------------
	override EPF_EReadResult ReadFrom(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
	{
		EPF_HitZoneContainerComponentSaveDataClass settings = EPF_HitZoneContainerComponentSaveDataClass.Cast(attributes);
		HitZoneContainerComponent hitZoneContainer = HitZoneContainerComponent.Cast(component);

		m_aHitzones = {};

		array<HitZone> outHitZones();
		hitZoneContainer.GetAllHitZones(outHitZones);

		foreach (HitZone hitZone : outHitZones)
		{
			EPF_PersistentHitZone persistentHitZone();
			persistentHitZone.m_sName = hitZone.GetName();
			persistentHitZone.m_fHealth = hitZone.GetHealthScaled();

			if (settings.m_bTrimDefaults && float.AlmostEqual(persistentHitZone.m_fHealth, 1.0)) continue;
			if (!settings.m_aHitzoneFilter.IsEmpty() && !settings.m_aHitzoneFilter.Contains(persistentHitZone.m_sName)) continue;

			m_aHitzones.Insert(persistentHitZone);
		}

		if (m_aHitzones.IsEmpty()) return EPF_EReadResult.DEFAULT;
		return EPF_EReadResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override EPF_EApplyResult ApplyTo(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
	{
		EPF_EApplyResult result = EPF_EApplyResult.OK;

		array<HitZone> outHitZones();
		HitZoneContainerComponent hitZoneContainer = HitZoneContainerComponent.Cast(component);
		hitZoneContainer.GetAllHitZones(outHitZones);

		bool tryIdxAcces = outHitZones.Count() >= m_aHitzones.Count();

		foreach (int idx, EPF_PersistentHitZone persistentHitZone : m_aHitzones)
		{
			HitZone hitZone;

			// Assume same ordering as on save and see if that matches
			if (tryIdxAcces)
			{
				HitZone idxHitZone = outHitZones.Get(idx);

				if (idxHitZone.GetName() == persistentHitZone.m_sName) hitZone = idxHitZone;
			}

			// Iterate all hitzones to hopefully find the right one
			if (!hitZone)
			{
				foreach (HitZone findHitZone : outHitZones)
				{
					if (findHitZone.GetName() == persistentHitZone.m_sName)
					{
						hitZone = findHitZone;
						break;
					}
				}
			}

			if (!hitZone)
			{
				Debug.Error(string.Format("'%1' unable to find hitZone with name '%2'. Ignored.", component, persistentHitZone.m_sName));
				continue;
			}

			hitZone.SetHealthScaled(persistentHitZone.m_fHealth);
		}

		return result;
	}

	//------------------------------------------------------------------------------------------------
	override bool Equals(notnull EPF_ComponentSaveData other)
	{
		EPF_HitZoneContainerComponentSaveData otherData = EPF_HitZoneContainerComponentSaveData.Cast(other);

		if (m_aHitzones.Count() != otherData.m_aHitzones.Count())
			return false;

		foreach (int idx, EPF_PersistentHitZone hitZone : m_aHitzones)
		{
			// Try same index first as they are likely to be the correct ones.
			if (hitZone.Equals(otherData.m_aHitzones.Get(idx)))
				continue;

			bool found;
			foreach (int compareIdx, EPF_PersistentHitZone otherhitZone : otherData.m_aHitzones)
			{
				if (compareIdx == idx)
					continue; // Already tried in idx direct compare

				if (hitZone.Equals(otherhitZone))
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

class EPF_PersistentHitZone
{
	string m_sName;
	float m_fHealth;

	//------------------------------------------------------------------------------------------------
	bool Equals(notnull EPF_PersistentHitZone other)
	{
		return m_sName == other.m_sName && float.AlmostEqual(m_fHealth, other.m_fHealth);
	}
};
