class EPF_DeferredApplyResult
{
	protected static ref map<ref EPF_EntitySaveData, ref EPF_PendingIdentifierHolder> s_mPendingIdentifiers =
		new map<ref EPF_EntitySaveData, ref EPF_PendingIdentifierHolder>;

	protected static ref map<EPF_ComponentSaveData, ref EPF_PendingComponentIdentifierHolder> s_mPendingComponentIdentifiers =
		new map<EPF_ComponentSaveData, ref EPF_PendingComponentIdentifierHolder>;

	protected static ref set<ref EPF_EntitySaveData> s_aCheckQueue;

	//------------------------------------------------------------------------------------------------
	static bool IsPending(notnull EPF_EntitySaveData saveData)
	{
		return s_mPendingIdentifiers.Contains(saveData);
	}

	//------------------------------------------------------------------------------------------------
	static ScriptInvoker GetOnApplied(notnull EPF_EntitySaveData saveData)
	{
		EPF_PendingIdentifierHolder data = s_mPendingIdentifiers.Get(saveData);
		if (!data)
			return null;

		return data.m_pOnAppliedEvent;
	}

	//------------------------------------------------------------------------------------------------
	static void AddPending(notnull EPF_EntitySaveData saveData, string awaitIdentifier)
	{
		Print(string.Format("EPF_DeferredApplyResult.AddPending(%1, %2)", saveData, awaitIdentifier), LogLevel.VERBOSE);

		EPF_PendingIdentifierHolder data = s_mPendingIdentifiers.Get(saveData);
		if (!data)
		{
			data = new EPF_PendingIdentifierHolder({awaitIdentifier});
			s_mPendingIdentifiers.Set(saveData, data);
			return;
		}

		data.m_aIdentifiers.Insert(awaitIdentifier);
	}

	//------------------------------------------------------------------------------------------------
	static void AddPending(notnull EPF_ComponentSaveData componentSaveData, string awaitIdentifier)
	{
		Print(string.Format("EPF_DeferredApplyResult.AddPending(%1, %2)", componentSaveData, awaitIdentifier), LogLevel.VERBOSE);

		EPF_PendingComponentIdentifierHolder data = s_mPendingComponentIdentifiers.Get(componentSaveData);
		if (!data)
		{
			data = EPF_PendingComponentIdentifierHolder({awaitIdentifier});
			s_mPendingComponentIdentifiers.Set(componentSaveData, data);
			return;
		}

		data.m_aIdentifiers.Insert(awaitIdentifier);
	}

	//------------------------------------------------------------------------------------------------
	static void SetFinished(notnull EPF_EntitySaveData saveData, string awaitIdentifier)
	{
		Print(string.Format("EPF_DeferredApplyResult.SetFinished(%1, %2)", saveData, awaitIdentifier), LogLevel.VERBOSE);

		EPF_PendingIdentifierHolder data = s_mPendingIdentifiers.Get(saveData);
		if (!data)
			return;

		data.m_aIdentifiers.RemoveItem(awaitIdentifier);

		if (data.m_aIdentifiers.IsEmpty())
			QueueComplectionCheck(saveData);
	}

	//------------------------------------------------------------------------------------------------
	static void SetFinished(notnull EPF_ComponentSaveData componentSaveData, string awaitIdentifier)
	{
		Print(string.Format("EPF_DeferredApplyResult.SetFinished(%1, %2)", componentSaveData, awaitIdentifier), LogLevel.VERBOSE);

		EPF_PendingComponentIdentifierHolder data = s_mPendingComponentIdentifiers.Get(componentSaveData);
		if (!data)
			return;

		data.m_aIdentifiers.RemoveItem(awaitIdentifier);
		if (!data.m_aIdentifiers.IsEmpty())
			return;

		if (data.m_pSaveData)
		{
			QueueComplectionCheck(data.m_pSaveData);
		}
		else
		{
			// Remove info if pending + finished without ask for event or SetEntitySaveData invoke.
			s_mPendingComponentIdentifiers.Remove(componentSaveData);
		}
	}

	//------------------------------------------------------------------------------------------------
	static bool SetEntitySaveData(notnull EPF_ComponentSaveData componentSaveData, notnull EPF_EntitySaveData entitySaveData)
	{
		EPF_PendingComponentIdentifierHolder componentData = s_mPendingComponentIdentifiers.Get(componentSaveData);
		if (componentData)
		{
			componentData.m_pSaveData = entitySaveData;

			EPF_PendingIdentifierHolder data;
			if (s_mPendingIdentifiers)
				data = s_mPendingIdentifiers.Get(entitySaveData);

			if (!data)
			{
				data = new EPF_PendingIdentifierHolder(components: {componentSaveData});
				s_mPendingIdentifiers.Set(entitySaveData, data);
			}
			else
			{
				data.m_aComponents.Insert(componentSaveData);
			}

			return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static void QueueComplectionCheck(EPF_EntitySaveData saveData)
	{
		if (!s_aCheckQueue)
			s_aCheckQueue = new set<ref EPF_EntitySaveData>();

		s_aCheckQueue.Insert(saveData);

		ScriptCallQueue callQueue = GetGame().GetCallqueue();
		if (callQueue.GetRemainingTime(CheckForCompletion) == -1)
			callQueue.Call(CheckForCompletion);
	}

	//------------------------------------------------------------------------------------------------
	protected static void CheckForCompletion()
	{
		array<EPF_EntitySaveData> remove();
		foreach (EPF_EntitySaveData saveData : s_aCheckQueue)
		{
			EPF_PendingIdentifierHolder data = s_mPendingIdentifiers.Get(saveData);

			if (!data.m_aIdentifiers.IsEmpty())
				continue;

			bool completed = true;
			foreach (EPF_ComponentSaveData componentSaveData : data.m_aComponents)
			{
				EPF_PendingComponentIdentifierHolder componentData = s_mPendingComponentIdentifiers.Get(componentSaveData);
				if (!componentData.m_aIdentifiers.IsEmpty())
					completed = false;
			}
			if (!completed)
				continue;

			if (data.m_pOnAppliedEvent)
				data.m_pOnAppliedEvent.Invoke(saveData);

			remove.Insert(saveData);

			// Free save data again after apply has finished
			foreach (EPF_ComponentSaveData componentSaveData : data.m_aComponents)
			{
				s_mPendingComponentIdentifiers.Remove(componentSaveData);
			}
			s_mPendingIdentifiers.Remove(saveData);
		}

		foreach (EPF_EntitySaveData saveData : remove)
		{
			s_aCheckQueue.RemoveItem(saveData);
		}
	}
}

class EPF_PendingIdentifierHolder
{
	ref array<string> m_aIdentifiers;
	ref array<EPF_ComponentSaveData> m_aComponents;
	ref ScriptInvoker<EPF_EntitySaveData> m_pOnAppliedEvent;

	void EPF_PendingIdentifierHolder(array<string> identifiers = null, array<EPF_ComponentSaveData> components = null)
	{
		m_aIdentifiers = identifiers;
		if (!m_aIdentifiers)
			m_aIdentifiers = {};

		m_aComponents = components;
		if (!m_aComponents)
			m_aComponents = {};

		m_pOnAppliedEvent = new ScriptInvoker();
	}
}

class EPF_PendingComponentIdentifierHolder
{
	EPF_EntitySaveData m_pSaveData;
	ref array<string> m_aIdentifiers;

	void EPF_PendingComponentIdentifierHolder(array<string> identifiers = null)
	{
		m_aIdentifiers = identifiers;
		if (!m_aIdentifiers)
			m_aIdentifiers = {};
	}
}
