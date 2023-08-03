[ComponentEditorProps(category: "Persistence", description: "Must be attached to the EPF_PersistenceManagerComponent.")]
class EPF_PersistenceManagerExtensionBaseComponentClass : ScriptComponentClass
{
}

class EPF_PersistenceManagerExtensionBaseComponent : ScriptComponent
{
	//------------------------------------------------------------------------------------------------
	void OnSetup(EPF_PersistenceManager persistenceManager);
	
	//------------------------------------------------------------------------------------------------
	bool IsSetupComplete(EPF_PersistenceManager persistenceManager);
}
