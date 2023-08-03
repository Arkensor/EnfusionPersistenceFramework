modded class SCR_DoorUserAction
{
	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		super.PerformAction(pOwnerEntity, pUserEntity);
		EPF_PersistentDoorStateManager.UpdateDoorState(pOwnerEntity);
	}
}
