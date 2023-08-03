[EPF_ComponentSaveDataType(FuelManagerComponent), BaseContainerProps()]
class EPF_FuelManagerComponentSaveDataClass : EPF_ComponentSaveDataClass
{
}

[EDF_DbName.Automatic()]
class EPF_FuelManagerComponentSaveData : EPF_ComponentSaveData
{
	ref array<ref EPF_PersistentFuelNode> m_aFuelNodes;

	//------------------------------------------------------------------------------------------------
	override EPF_EReadResult ReadFrom(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
	{
		m_aFuelNodes = {};

		array<BaseFuelNode> outNodes();
		FuelManagerComponent.Cast(component).GetFuelNodesList(outNodes);

		foreach (BaseFuelNode node : outNodes)
		{
			SCR_FuelNode fuelNode = SCR_FuelNode.Cast(node);
			if (!fuelNode)
			{
				Debug.Error(string.Format("'%1' contains non persistable fuel node type '%2'. Inherit from SCR_FuelNode instead. Ignored.", component, node.Type()));
				continue;
			}

			EPF_PersistentFuelNode persistentFuelNode();
			persistentFuelNode.m_iTankId = fuelNode.GetFuelTankID();
			persistentFuelNode.m_fFuel = fuelNode.GetFuel();

			if (attributes.m_bTrimDefaults)
			{
				if (persistentFuelNode.m_fFuel >= fuelNode.GetMaxFuel())
					continue;

				if (float.AlmostEqual(persistentFuelNode.m_fFuel, fuelNode.EPF_GetInitialFuelTankState())) 
					continue;
			}

			m_aFuelNodes.Insert(persistentFuelNode);
		}

		if (m_aFuelNodes.IsEmpty())
			return EPF_EReadResult.DEFAULT;

		return EPF_EReadResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override EPF_EApplyResult ApplyTo(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
	{
		array<BaseFuelNode> outNodes();
		FuelManagerComponent.Cast(component).GetFuelNodesList(outNodes);

		bool tryIdxAcces = outNodes.Count() >= m_aFuelNodes.Count();

		foreach (int idx, EPF_PersistentFuelNode persistentFuelNode : m_aFuelNodes)
		{
			SCR_FuelNode fuelNode;

			// Assume same ordering as on save and see if that matches
			if (tryIdxAcces)
			{
				SCR_FuelNode idxNode = SCR_FuelNode.Cast(outNodes.Get(idx));

				if (idxNode.GetFuelTankID() == persistentFuelNode.m_iTankId)
					fuelNode = idxNode;
			}

			// Iterate all fuel nodes to hopefully find the right tank id
			if (!fuelNode)
			{
				foreach (BaseFuelNode findNode : outNodes)
				{
					SCR_FuelNode findFuelNode = SCR_FuelNode.Cast(findNode);
					if (findFuelNode && findFuelNode.GetFuelTankID() == persistentFuelNode.m_iTankId)
					{
						fuelNode = findFuelNode;
						break;
					}
				}
			}

			if (!fuelNode)
			{
				Debug.Error(string.Format("'%1' unable to find fuel tank id '%2'. Ignored.", component, persistentFuelNode.m_iTankId));
				continue;
			}

			fuelNode.SetFuel(persistentFuelNode.m_fFuel);
		}

		return EPF_EApplyResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override bool Equals(notnull EPF_ComponentSaveData other)
	{
		EPF_FuelManagerComponentSaveData otherData = EPF_FuelManagerComponentSaveData.Cast(other);

		if (m_aFuelNodes.Count() != otherData.m_aFuelNodes.Count())
			return false;

		foreach (int idx, EPF_PersistentFuelNode fuelNode : m_aFuelNodes)
		{
			// Try same index first as they are likely to be the correct ones.
			if (fuelNode.Equals(otherData.m_aFuelNodes.Get(idx)))
				continue;

			bool found;
			foreach (int compareIdx, EPF_PersistentFuelNode otherFuelNode : otherData.m_aFuelNodes)
			{
				if (compareIdx == idx)
					continue; // Already tried in idx direct compare

				if (fuelNode.Equals(otherFuelNode))
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
}

class EPF_PersistentFuelNode
{
	int m_iTankId;
	float m_fFuel;

	//------------------------------------------------------------------------------------------------
	bool Equals(notnull EPF_PersistentFuelNode other)
	{
		return m_iTankId == other.m_iTankId && float.AlmostEqual(m_fFuel, other.m_fFuel);
	}
}

modded class SCR_FuelNode
{
	float EPF_GetInitialFuelTankState()
	{
		return m_fInitialFuelTankState;
	}
}
