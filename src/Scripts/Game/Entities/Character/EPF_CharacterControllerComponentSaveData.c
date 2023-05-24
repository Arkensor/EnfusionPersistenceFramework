[EPF_ComponentSaveDataType(SCR_CharacterControllerComponent), BaseContainerProps()]
class EPF_CharacterControllerComponentSaveDataClass : EPF_ComponentSaveDataClass
{
	override array<typename> Requires()
	{
		return {EPF_BaseInventoryStorageComponentSaveDataClass};
	}
};

[EDF_DbName.Automatic()]
class EPF_CharacterControllerComponentSaveData : EPF_ComponentSaveData
{
	ECharacterStance m_eStance;
	string m_sLeftHandItemId;
	string m_sRightHandItemId;
	EEquipItemType m_eRightHandType;
	bool m_bRightHandRaised;

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
			!m_bRightHandRaised) return EPF_EReadResult.DEFAULT;

		return EPF_EReadResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override EPF_EApplyResult ApplyTo(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
	{
		EPF_EApplyResult result = EPF_EApplyResult.OK;

		// Apply stance
		SCR_CharacterControllerComponent characterController = SCR_CharacterControllerComponent.Cast(component);
		if (characterController.GetStance() != m_eStance)
		{
			switch (m_eStance)
			{
				case ECharacterStance.STAND:
				{
					characterController.SetStanceChange(ECharacterStanceChange.STANCECHANGE_TOERECTED);
					break;
				}
				case ECharacterStance.CROUCH:
				{
					characterController.SetStanceChange(ECharacterStanceChange.STANCECHANGE_TOCROUCH);
					break;
				}
				case ECharacterStance.PRONE:
				{
					characterController.SetStanceChange(ECharacterStanceChange.STANCECHANGE_TOPRONE);
					break;
				}
			}
		}

		// Apply hand items
		EPF_PersistenceManager persistenceManager = EPF_PersistenceManager.GetInstance();
		IEntity rightHandEntity = persistenceManager.FindEntityByPersistentId(m_sRightHandItemId);
		if (rightHandEntity)
		{
			BaseWeaponManagerComponent weaponManager = EPF_Component<BaseWeaponManagerComponent>.Find(characterController.GetOwner());
			if (weaponManager)
			{
				EPF_DeferredApplyResult.AddPending(this, "CharacterControllerComponentSaveData::RightHandItemEquipped");
				weaponManager.m_OnWeaponChangeCompleteInvoker.Insert(OnWeaponChangeComplete);
				characterController.TryEquipRightHandItem(rightHandEntity, m_eRightHandType, false);
				result = EPF_EApplyResult.AWAIT_COMPLETION;
			}
		}
		else if (m_sLeftHandItemId)
		{
			// No weapon so gadget can be equipped as soon as manager is ready
			SCR_GadgetManagerComponent gadgetMananger = SCR_GadgetManagerComponent.GetGadgetManager(characterController.GetOwner());
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
	protected void OnWeaponChangeComplete(BaseWeaponComponent newWeaponSlot)
	{
		if (newWeaponSlot)
		{
			IEntity owner = newWeaponSlot.GetOwner();
			SCR_CharacterControllerComponent characterController = EPF_Component<SCR_CharacterControllerComponent>.Find(owner);

			BaseWeaponManagerComponent weaponManager = EPF_Component<BaseWeaponManagerComponent>.Find(owner);
			weaponManager.m_OnWeaponChangeCompleteInvoker.Remove(OnWeaponChangeComplete);

			if (m_bRightHandRaised)
			{
				EPF_DeferredApplyResult.AddPending(this, "CharacterControllerComponentSaveData::WeaponRaised");
				characterController.SetWeaponRaised(m_bRightHandRaised);
				// Hardcode weapon raise complete after fixed amount of time until we have a nice even for OnRaiseFinished or something
				GetGame().GetCallqueue().CallLater(OnRaiseFinished, 1000);
			}

			if (m_sLeftHandItemId)
			{
				// By the time a weapon was equipped the manager will be ready so we can take gadget to hands now
				EPF_DeferredApplyResult.AddPending(this, "CharacterControllerComponentSaveData::GadgetEquipped");
				OnGadgetInitDone(owner, SCR_GadgetManagerComponent.GetGadgetManager(owner));
			}
		}

		EPF_DeferredApplyResult.SetFinished(this, "CharacterControllerComponentSaveData::RightHandItemEquipped");
	}

	//------------------------------------------------------------------------------------------------
	protected void OnRaiseFinished()
	{
		EPF_DeferredApplyResult.SetFinished(this, "CharacterControllerComponentSaveData::WeaponRaised");
	}

	//------------------------------------------------------------------------------------------------
	protected void OnGadgetInitDone(IEntity owner, SCR_GadgetManagerComponent gadgetMananger)
	{
		IEntity leftHandEntity = EPF_PersistenceManager.GetInstance().FindEntityByPersistentId(m_sLeftHandItemId);
		if (!leftHandEntity)
			return;

		gadgetMananger.HandleInput(leftHandEntity, 1);

		SCR_CharacterControllerComponent characterController = EPF_Component<SCR_CharacterControllerComponent>.Find(owner);
		characterController.m_OnGadgetStateChangedInvoker.Insert(OnGadgetStateChanged);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnGadgetStateChanged(IEntity gadget, bool isInHand, bool isOnGround)
	{
		if (isInHand)
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
};
