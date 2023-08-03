[EPF_ComponentSaveDataType(SCR_CharacterControllerComponent), BaseContainerProps()]
class EPF_CharacterControllerComponentSaveDataClass : EPF_ComponentSaveDataClass
{
	override array<typename> Requires()
	{
		return {EPF_BaseInventoryStorageComponentSaveDataClass};
	}
}

[EDF_DbName.Automatic()]
class EPF_CharacterControllerComponentSaveData : EPF_ComponentSaveData
{
	ECharacterStance m_eStance;
	string m_sLeftHandItemId;
	string m_sRightHandItemId;
	EEquipItemType m_eRightHandType;
	bool m_bRightHandRaised;

	[NonSerialized()]
	protected SCR_CharacterControllerComponent m_pCharacterController;

	//------------------------------------------------------------------------------------------------
	override EPF_EReadResult ReadFrom(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
	{
		SCR_CharacterControllerComponent characterController = SCR_CharacterControllerComponent.Cast(component);

		m_eStance = characterController.GetStance();
		m_sLeftHandItemId = EPF_PersistenceComponent.GetPersistentId(characterController.GetAttachedGadgetAtLeftHandSlot());
		m_sRightHandItemId = EPF_PersistenceComponent.GetPersistentId(characterController.GetRightHandItem());
		if (m_sRightHandItemId)
		{
			m_eRightHandType = EEquipItemType.EEquipTypeGeneric;
		}
		else
		{
			BaseWeaponManagerComponent weaponManager = characterController.GetWeaponManagerComponent();
			if (weaponManager && weaponManager.GetCurrentSlot())
			{
				m_sRightHandItemId = EPF_PersistenceComponent.GetPersistentId(weaponManager.GetCurrentSlot().GetWeaponEntity());
				m_eRightHandType = EEquipItemType.EEquipTypeWeapon;
				m_bRightHandRaised = characterController.IsWeaponRaised();
			}
		}

		if (attributes.m_bTrimDefaults &&
			m_eStance == ECharacterStance.STAND &&
			!m_sLeftHandItemId &&
			!m_sRightHandItemId &&
			m_eRightHandType == EEquipItemType.EEquipTypeNone &&
			!m_bRightHandRaised)
		{
			return EPF_EReadResult.DEFAULT;
		}

		return EPF_EReadResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override EPF_EApplyResult ApplyTo(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
	{
		EPF_EApplyResult result = EPF_EApplyResult.OK;

		// Apply stance
		m_pCharacterController = SCR_CharacterControllerComponent.Cast(component);
		if (m_pCharacterController.GetStance() != m_eStance)
		{
			switch (m_eStance)
			{
				case ECharacterStance.STAND:
				{
					m_pCharacterController.SetStanceChange(ECharacterStanceChange.STANCECHANGE_TOERECTED);
					break;
				}
				case ECharacterStance.CROUCH:
				{
					m_pCharacterController.SetStanceChange(ECharacterStanceChange.STANCECHANGE_TOCROUCH);
					break;
				}
				case ECharacterStance.PRONE:
				{
					m_pCharacterController.SetStanceChange(ECharacterStanceChange.STANCECHANGE_TOPRONE);
					break;
				}
			}

			result = EPF_EApplyResult.AWAIT_COMPLETION;
			EPF_DeferredApplyResult.AddPending(this, "CharacterControllerComponentSaveData::StanceChanged");
			GetGame().GetCallqueue().CallLater(ListenForStanceChangeComplete, m_pCharacterController.GetStanceChangeDelayTime() * 1000, true);
		}

		// Apply hand items
		EPF_PersistenceManager persistenceManager = EPF_PersistenceManager.GetInstance();
		IEntity rightHandEntity = persistenceManager.FindEntityByPersistentId(m_sRightHandItemId);
		if (rightHandEntity)
		{
			BaseWeaponManagerComponent weaponManager = EPF_Component<BaseWeaponManagerComponent>.Find(m_pCharacterController.GetOwner());
			if (weaponManager)
			{
				WeaponSlotComponent curWeaponSlot = weaponManager.GetCurrentSlot();
				if (!curWeaponSlot || curWeaponSlot.GetWeaponEntity() != rightHandEntity)
				{
					// Because of MP ownership transfer mid animation issues, we wait for the animation event of weapon change instead of the weapon manager scripted events
					EPF_DeferredApplyResult.AddPending(this, "CharacterControllerComponentSaveData::RightHandItemEquipped");
					m_pCharacterController.GetOnAnimationEvent().Insert(ListenForWeaponEquipComplete);
					m_pCharacterController.TryEquipRightHandItem(rightHandEntity, m_eRightHandType, false);
				}
				else if (m_bRightHandRaised)
				{
					StartWeaponRaise();
				}

				result = EPF_EApplyResult.AWAIT_COMPLETION;
			}
		}
		else if (m_sLeftHandItemId)
		{
			// No weapon so gadget can be equipped as soon as manager is ready
			SCR_GadgetManagerComponent gadgetMananger = SCR_GadgetManagerComponent.GetGadgetManager(m_pCharacterController.GetOwner());
			if (gadgetMananger)
			{
				EPF_DeferredApplyResult.AddPending(this, "CharacterControllerComponentSaveData::GadgetEquipped");
				gadgetMananger.GetOnGadgetInitDoneInvoker().Insert(OnGadgetInitDone);
				result = EPF_EApplyResult.AWAIT_COMPLETION;
			}
		}

		return result;
	}

	//------------------------------------------------------------------------------------------------
	protected void ListenForStanceChangeComplete()
	{
		if (m_pCharacterController.IsChangingStance())
			return;

		GetGame().GetCallqueue().Remove(ListenForStanceChangeComplete);
		EPF_DeferredApplyResult.SetFinished(this, "CharacterControllerComponentSaveData::StanceChanged");
	}

	//------------------------------------------------------------------------------------------------
	protected void ListenForWeaponEquipComplete(AnimationEventID animEventType)
	{
		if (GameAnimationUtils.GetEventString(animEventType) != "EquipComplete")
			return;

		m_pCharacterController.GetOnAnimationEvent().Remove(ListenForWeaponEquipComplete);

		if (m_bRightHandRaised)
		{
			StartWeaponRaise();
		}
		else if (m_sLeftHandItemId)
		{
			StartInitGadget();
		}

		EPF_DeferredApplyResult.SetFinished(this, "CharacterControllerComponentSaveData::RightHandItemEquipped");
	}

	//------------------------------------------------------------------------------------------------
	protected void StartWeaponRaise()
	{
		EPF_DeferredApplyResult.AddPending(this, "CharacterControllerComponentSaveData::WeaponRaised");
		m_pCharacterController.SetWeaponRaised(true);

		// There is a "StanceTrans" event for when char is erect, but in crouch there is no anim event we could wait for, so for now we just hardcode it.
		// TODO: Refactor when https://feedback.bistudio.com/T172051 is merged.
		GetGame().GetCallqueue().CallLater(OnRaiseFinished, 1000);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnRaiseFinished()
	{
		if (m_sLeftHandItemId)
			StartInitGadget();

		EPF_DeferredApplyResult.SetFinished(this, "CharacterControllerComponentSaveData::WeaponRaised");
	}

	//------------------------------------------------------------------------------------------------
	protected void StartInitGadget()
	{
		EPF_DeferredApplyResult.AddPending(this, "CharacterControllerComponentSaveData::GadgetEquipped");
		IEntity owner = m_pCharacterController.GetOwner();
		OnGadgetInitDone(owner, SCR_GadgetManagerComponent.GetGadgetManager(owner));
	}

	//------------------------------------------------------------------------------------------------
	protected void OnGadgetInitDone(IEntity owner, SCR_GadgetManagerComponent gadgetMananger)
	{
		IEntity leftHandEntity = EPF_PersistenceManager.GetInstance().FindEntityByPersistentId(m_sLeftHandItemId);
		if (!leftHandEntity)
			return;

		m_pCharacterController.m_OnGadgetStateChangedInvoker.Insert(OnGadgetStateChanged);
		gadgetMananger.HandleInput(leftHandEntity, 1);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnGadgetStateChanged(IEntity gadget, bool isInHand, bool isOnGround)
	{
		if (!isInHand)
			return;

		m_pCharacterController.m_OnGadgetStateChangedInvoker.Remove(OnGadgetStateChanged);

		// Ever so slight delay until animation is really completed.
		// Wait it out so no MP animation issues arise from network ownership transfer too early.
		GetGame().GetCallqueue().CallLater(OnGadgetStateChangedCompleted, 500);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnGadgetStateChangedCompleted()
	{
		EPF_DeferredApplyResult.SetFinished(this, "CharacterControllerComponentSaveData::GadgetEquipped");
	}

	//------------------------------------------------------------------------------------------------
	override bool Equals(notnull EPF_ComponentSaveData other)
	{
		EPF_CharacterControllerComponentSaveData otherData = EPF_CharacterControllerComponentSaveData.Cast(other);
		return m_eStance == otherData.m_eStance &&
			m_sLeftHandItemId == otherData.m_sLeftHandItemId &&
			m_sRightHandItemId == otherData.m_sRightHandItemId &&
			m_eRightHandType == otherData.m_eRightHandType &&
			m_bRightHandRaised == otherData.m_bRightHandRaised;
	}
}
