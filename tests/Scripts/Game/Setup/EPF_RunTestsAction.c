sealed class TAG_MyComponentClass : ScriptComponentClass
{
	//------------------------------------------------------------------------------------------------
	static override bool DependsOn(string className)
	{
		DoSomething(className.ToType(), {Class});
		return false;
	}

	//------------------------------------------------------------------------------------------------
	static void DoSomething(typename type, notnull array<typename> from)
	{
		Print(type);
	}
};

class TAG_MyComponent : ScriptComponent
{
};

class EPF_RunTestsAction : ScriptedUserAction
{
	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		EDF_AutoTestEntity.Run();
	}
};
