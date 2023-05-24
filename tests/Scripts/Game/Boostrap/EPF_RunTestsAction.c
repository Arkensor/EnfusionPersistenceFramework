class EPF_RunTestsAction : ScriptedUserAction
{
	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		EDF_AutoTestEntity.Run();
	}
};
