[EntityEditorProps(category: "Persistence", description: "Proxy to assign name to base scene entities.")]
class EPF_BaseSceneNameProxyEntityClass : GenericEntityClass
{
}

class EPF_BaseSceneNameProxyEntity : GenericEntity
{
	[Attribute(uiwidget: UIWidgets.ResourcePickerThumbnail, desc: "Target prefab to find closest to proxy position.")]
	protected ResourceName m_rTarget;

	#ifdef WORKBENCH
	static ref array<EPF_BaseSceneNameProxyEntity> s_aAllProxies = {};

	static EPF_BaseSceneNameProxyEntity s_pSelectedProxy;

	IEntity m_pTarget;
	#endif

	//------------------------------------------------------------------------------------------------
	protected void EPF_BaseSceneNameProxyEntity(IEntitySource src, IEntity parent)
	{
		SetEventMask(EntityEvent.INIT);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		if (GetWorld().IsEditMode() || EPF_PersistenceManager.IsPersistenceMaster())
			FindTarget(owner.GetPrefabData().GetPrefab().ToEntitySource());

		if (!GetWorld().IsEditMode())
		{
			delete this;
			return;
		}

		#ifdef WORKBENCH
		s_aAllProxies.Insert(this);
		#endif
	}

	//------------------------------------------------------------------------------------------------
	#ifdef WORKBENCH
	protected void ~EPF_BaseSceneNameProxyEntity()
	{
		s_aAllProxies.RemoveItem(this);
	}
	#endif

	//------------------------------------------------------------------------------------------------
	protected void FindTarget(BaseContainer src = null, float radius = 1.0)
	{
		#ifdef WORKBENCH
		m_pTarget = null;
		#endif
		if (src && (!src.Get("m_rTarget", m_rTarget) || !m_rTarget))
		{
			#ifdef WORKBENCH
			if (!_WB_GetEditorAPI().IsDoingEditAction())
			#endif
				Print(string.Format("Base scene name proxy '%1' without assigned target prefab. Fix or delete proxy entity!", GetName()), LogLevel.ERROR);
		}
		else
		{
			if (GetWorld().QueryEntitiesBySphere(GetOrigin(), radius, WorldSearchCallback))
				Print(string.Format("Base scene name proxy '%1' failed to find target prefab '%2' in %3 meter radius. Fix or delete proxy entity!", GetName(), m_rTarget, radius), LogLevel.ERROR);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected bool WorldSearchCallback(IEntity ent)
	{
		if (EPF_Utils.GetPrefabName(ent) == m_rTarget)
		{
			#ifdef WORKBENCH
			if (GetWorld().IsEditMode())
			{
				m_pTarget = ent;
				return false;
			}
			Print(string.Format("Assigned entity %1@<%2> the name '%3' via proxy.", EPF_Utils.GetPrefabName(ent), ent.GetOrigin().ToString(false), GetName()), LogLevel.DEBUG);
			#endif

			ent.SetName(GetName());
			return false;
		}
		return true;
	}

	#ifdef WORKBENCH
	static EPF_BaseSceneNameProxyEntity GetProxyForBaseSceneEntity(notnull IEntity baseSceneEntity)
	{
		foreach (EPF_BaseSceneNameProxyEntity proxy : s_aAllProxies)
		{
			if (proxy.m_pTarget == baseSceneEntity)
				return proxy;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	override bool _WB_CanCopy(IEntitySource src)
	{
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override bool _WB_OnKeyChanged(BaseContainer src, string key, BaseContainerList ownerContainers, IEntity parent)
	{
		FindTarget(src);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override array<ref WB_UIMenuItem> _WB_GetContextMenuItems()
	{
		array<ref WB_UIMenuItem> items();

		if (m_pTarget)
		{
			items.Insert(new WB_UIMenuItem("Go to proxy target", 0));
		}
		else if (m_rTarget)
		{
			items.Insert(new WB_UIMenuItem("Try autofix proxy target", 1));
			items.Insert(new WB_UIMenuItem("Remember proxy", 2));
		}

		return items;
	}

	//------------------------------------------------------------------------------------------------
	override void _WB_OnContextMenu(int id)
	{
		WorldEditorAPI worldEditorApi = _WB_GetEditorAPI();

		if (id == 2)
		{
			s_pSelectedProxy = this;
			return;
		}

		if (id == 1)
		{
			FindTarget(radius: 50);
			if (!m_pTarget)
				return;

			worldEditorApi.BeginEntityAction("BaseSceneNameProxyEntity__AutoFix");
			worldEditorApi.SetVariableValue(worldEditorApi.EntityToSource(this), null, "coords", m_pTarget.GetOrigin().ToString(false));
			worldEditorApi.EndEntityAction();
		}

		worldEditorApi.SetEntitySelection(worldEditorApi.EntityToSource(m_pTarget));
		worldEditorApi.UpdateSelectionGui();
	}

	//------------------------------------------------------------------------------------------------
	override void _WB_AfterWorldUpdate(float timeSlice)
	{
		WorldEditorAPI worldEditorApi = _WB_GetEditorAPI();
		IEntity selected = worldEditorApi.SourceToEntity(worldEditorApi.GetSelectedEntity());
		if (!m_pTarget || (selected != this && selected != m_pTarget))
			return;

		//Shape.CreateSphere(Color.PINK, ShapeFlags.ONCE | ShapeFlags.WIREFRAME | ShapeFlags.NOZBUFFER, GetOrigin(), 0.5);

		vector position = m_pTarget.GetOrigin();
		vector bboxMin, bboxMax;
		m_pTarget.GetWorldBounds(bboxMin, bboxMax);
		DebugTextWorldSpace.Create(
			GetWorld(),
			GetName(),
			DebugTextFlags.ONCE | DebugTextFlags.CENTER | DebugTextFlags.FACE_CAMERA,
			position[0],
			position[1] + bboxMax[1] + 0.5,
			position[2],
			20.0,
			Color.PINK);
	}
	#endif
}
