class EPF_BasicRespawnSystemComponentClass : EPF_BaseRespawnSystemComponentClass
{
}

class EPF_BasicRespawnSystemComponent : EPF_BaseRespawnSystemComponent
{
	[Attribute(defvalue: "{37578B1666981FCE}Prefabs/Characters/Core/Character_Base.et")]
	ResourceName m_rDefaultPrefab;

	//------------------------------------------------------------------------------------------------
	override protected void GetCreationPosition(int playerId, string characterPersistenceId, out vector position, out vector yawPitchRoll)
	{
		EPF_SpawnPoint spawnPoint = EPF_SpawnPoint.GetRandomSpawnPoint();
		if (!spawnPoint)
		{
			Print("Could not spawn character, no spawn point on the map.", LogLevel.ERROR);
			return;
		}

		spawnPoint.GetPosYPR(position, yawPitchRoll);
	}

	//------------------------------------------------------------------------------------------------
	override protected ResourceName GetCreationPrefab(int playerId, string characterPersistenceId)
	{
		return m_rDefaultPrefab;
	}
}
