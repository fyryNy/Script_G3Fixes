#include "Script_G3Fixes.h"

GEBool CompanionAutoDefend = GEFalse;
GEBool CompanionAutoDefendHotkeyPressed = GEFalse;
eCInpShared::eEKeyboardStateOffset CompanionAutoDefendHotkey = eCInpShared::eEKeyboardStateOffset::eEKeyboardStateOffset_APOSTROPHE;
GEBool TeleportCompanionTooFarAway = GETrue;
GEBool QuickCastChance = GETrue;
GEBool BlockMonsterRespawn = GEFalse;
GEBool RemoveWaterfallSoundEffects = GEFalse;

static CFFGFCBitmap CompanionIcon;
GEI32 CompanionIconSize = 16;
GEFloat CompanionIconPosTopX = 98.5;
GEFloat CompanionIconPosTopY = 2.5;

GEBool HerdUnityActive = GEFalse;
static GEBool HerdUnity = GEFalse;
static Entity HerdLeader = None;


gSScriptInit & GetScriptInit()
{
	static gSScriptInit s_ScriptInit;
	return s_ScriptInit;
}

void TryFixMist(void)
{
	auto Player = Entity::GetPlayer();

	if (!Player.NPC.HasStatusEffects(gEStatusEffect::gEStatusEffect_IsImNebel))
	{
		return;
	}

	eCSceneAdmin * pSceneAdmin = FindModule<eCSceneAdmin>();
	if (!pSceneAdmin)
	{
		return;
	}

	auto MistGameEntity = pSceneAdmin->GetEntityByName("Smn_Mist");
	if (MistGameEntity)
	{
		spy.Send(bCString::GetFormattedString("%s - Entity Smn_Mist found, skipping.", PLUGIN_NAME).GetText());
		return;
	}

	spy.Send(bCString::GetFormattedString("%s - Entity Smn_Mist not found, trying to fix hero's statuseffect.", PLUGIN_NAME).GetText());
	Player.NPC.EnableStatusEffects(gEStatusEffect::gEStatusEffect_IsImNebel, GEFalse);
}

static mCFunctionHook Hook_Entity_EndTransform;
void Entity_EndTransform(void)
{
	Hook_Entity_EndTransform.GetOriginalFunction(&Entity_EndTransform)();
	TryFixMist();
}

static mCCallHook Hook_AfterApplicationProcess;
void GE_STDCALL AfterApplicationProcess(void)
{
	Hook_AfterApplicationProcess.Disable();

	if (GetScriptAdmin().IsScriptDLLLoaded("Script_ItemUseFuncEnabler.dll"))
	{
		GE_FATAL_ERROR_EX("Script_G3Fixes", "Obsolete script \"Script_ItemUseFuncEnabler\" detected.\nPlease remove file \"Script_ItemUseFuncEnabler.dll\" from \"Gothic 3/scripts\" directory.");
	}
}

static mCFunctionHook Hook_AssessTarget;
GEInt GE_STDCALL AssessTarget(gCScriptProcessingUnit * a_pSPU, Entity * a_pSelfEntity, Entity * a_pOtherEntity, GEU32 a_iArgs)
{
	INIT_SCRIPT();

	auto Player = Entity::GetPlayer();

	auto result = Hook_AssessTarget.GetOriginalFunction(&AssessTarget)(SCRIPT_PARAMS);

	gEAttackReason AttackReason = static_cast<gEAttackReason>(a_iArgs);

	if (HerdLeader == None)
	{
		return result;
	}

	if (AttackReason != gEAttackReason::gEAttackReason_Angry && !HerdUnity && HerdLeader != SelfEntity/* && OtherEntity != Player*/)
	{
		return result;
	}

	bTObjArray<Entity> Ents = Entity::GetEntities();
	Entity::SortEntityListByDistanceTo(Ents, SelfEntity);

	for (GEInt X = 0; X < Ents.GetCount(); X++)
	{
		Entity Ent = Ents.GetAt(X);

		if (Ent == Player)
		{
			continue;
		}

		if (Ent == SelfEntity)
		{
			continue;
		}

		if (SelfEntity.GetDistanceTo(Ent) > 2500.0f)
		{
			continue;
		}

		if (Ent.IsDead() || Ent.IsDown())
		{
			continue;
		}

		if (Ent.Party.GetPartyLeader() == Player || Ent.Party.PartyMemberType == gEPartyMemberType::gEPartyMemberType_Summoned)
		{
			continue;
		}

		if (Ent.NPC.Species != SelfEntity.NPC.Species)
		{
			continue;
		}

		if (Ent.NPC.CombatState == 1)
		{
			continue;
		}

		if (Ent.Routine.GetCurrentTask().CompareFast("ZS_Threaten"))
		{
			continue;
		}

		gCScriptAdmin & ScriptAdmin = GetScriptAdmin();
		ScriptAdmin.CallScriptFromScript("AssessTarget", &Ent, &OtherEntity, gEAttackReason::gEAttackReason_Angry);
	}

	return result;
}

static mCFunctionHook Hook_ZS_Threaten_Loop;
GEInt GE_STDCALL ZS_Threaten_Loop(GEInt a0, gCScriptProcessingUnit * a_pSPU)
{
	if (HerdUnityActive)
	{
		HerdLeader = a_pSPU->GetSelfEntity();
		HerdUnity = GETrue;

		GEInt result = Hook_ZS_Threaten_Loop.GetOriginalFunction(&ZS_Threaten_Loop)(a0, a_pSPU);

		HerdLeader = nullptr;
		HerdUnity = GEFalse;

		return result;
	}

	return Hook_ZS_Threaten_Loop.GetOriginalFunction(&ZS_Threaten_Loop)(a0, a_pSPU);
}

static mCFunctionHook Hook_Respawn;
GEInt GE_STDCALL Respawn(gCScriptProcessingUnit * a_pSPU, Entity * a_pSelfEntity, Entity * a_pOtherEntity, GEU32 a_iArgs)
{
	INIT_SCRIPT();
	auto result = Hook_Respawn.GetOriginalFunction(&Respawn)(SCRIPT_PARAMS);

	if (result != 0 && BlockMonsterRespawn)
	{
		spy.Send(bCString::GetFormattedString("%s - Blocking respawn of %s", PLUGIN_NAME, SelfEntity.GetName()).GetText());
		SelfEntity.Kill();
	}

	return result;
}

static mCFunctionHook Hook_CanFreeze;
GEInt GE_STDCALL CanFreeze(gCScriptProcessingUnit * a_pSPU, Entity * a_pSelfEntity, Entity * a_pOtherEntity, GEU32 Damage)
{
	INIT_SCRIPT_EXT(Victim, Damager);

	Entity Player = Entity::GetPlayer();
	gESpecies VictimSpecies = Victim.NPC.Species;

	switch (VictimSpecies)
	{
	case gESpecies::gESpecies_Golem:
	case gESpecies::gESpecies_Demon:
	case gESpecies::gESpecies_Troll:
	case gESpecies::gESpecies_FireGolem:
	case gESpecies::gESpecies_IceGolem:
	case gESpecies::gESpecies_Dragon:
		return 0;
	default:
		break;
	}

	gCScriptAdmin & ScriptAdmin = GetScriptAdmin();
	auto VictimHP = ScriptAdmin.CallScriptFromScript("GetHitPoints", &Victim, &None);
	if (VictimHP - static_cast<GEInt>(Damage) <= 0)
	{
		return GEFalse;
	}

	GEInt Chance = 0;
	GEBool IceDamage = GEFalse;

	gEDamageType DamageType = Damager.Damage.GetProperty<PSDamage::PropertyDamageType>();
	if (DamageType == gEDamageType::gEDamageType_Ice)
	{
		IceDamage = GETrue;
	}

	if (Damager.IsItem())
	{
		GEU32 DamagerQuality = Damager.Item.GetQuality();
		GEU32 QualityFreezing = 16;
		if ((DamagerQuality & QualityFreezing) == QualityFreezing)
		{
			IceDamage = GETrue;
		}
	}

	if (!IceDamage)
	{
		return 0;
	}

	if (IsSpellContainer(Damager))
	{
		if (!IsMagicProjectile(Damager))
		{
			Chance = 100;
		}
		else if (IsMagicProjectile(Damager) && Damager.Projectile.PathStyle == gEProjectilePath_Missile)
		{
			Chance = 100;
		}
		else if (QuickCastChance)
		{
			Chance = 10;
		}
	}
	else if (IsNormalProjectile(Damager))
	{
		Chance = static_cast<GEInt>(150 * Damager.Damage.GetProperty<PSDamage::PropertyDamageHitMultiplier>());
	}
	else
	{
		Entity DamagerOwner = Damager.Interaction.GetOwner();
		if (DamagerOwner == None && Damager.Navigation.IsValid())
		{
			DamagerOwner = Damager;
		}
		gEAction DamagerOwnerAction = DamagerOwner.Routine.GetProperty<PSRoutine::PropertyAction>();
		if (DamagerOwnerAction == gEAction::gEAction_PowerAttack)
		{
			Chance = 35;
		}
		else
		{
			Chance = 30;
		}
	}

	if (Victim == Player && Player.Inventory.IsSkillActive("Perk_ResistCold"))
	{
		Chance = static_cast<GEInt>(Chance * 0.66);
	}

	GEInt Random = randomizer.Random(0, 100);

	if (Random < Chance)
	{
		return 1;
	}

	return 0;
}

static mCFunctionHook Hook_CanBurn;
GEInt GE_STDCALL CanBurn(gCScriptProcessingUnit * a_pSPU, Entity * a_pSelfEntity, Entity * a_pOtherEntity, GEU32 Damage)
{
	UNREFERENCED_PARAMETER(Damage);
	INIT_SCRIPT_EXT(Victim, Damager);

	Entity Player = Entity::GetPlayer();
	gESpecies VictimSpecies = Victim.NPC.Species;

	switch (VictimSpecies)
	{
	case gESpecies::gESpecies_Golem:
	case gESpecies::gESpecies_Demon:
	case gESpecies::gESpecies_Troll:
	case gESpecies::gESpecies_FireGolem:
	case gESpecies::gESpecies_IceGolem:
	case gESpecies::gESpecies_Dragon:
		return 0;
	default:
		break;
	}

	gCScriptAdmin & ScriptAdmin = GetScriptAdmin();
	auto VictimHP = ScriptAdmin.CallScriptFromScript("GetHitPoints", &Victim, &None);
	if (VictimHP - static_cast<GEInt>(Damage) <= 0)
	{
		return GEFalse;
	}

	GEInt Chance = 0;
	GEBool FireDamage = GEFalse;

	gEDamageType DamageType = Damager.Damage.GetProperty<PSDamage::PropertyDamageType>();
	if (DamageType == gEDamageType::gEDamageType_Fire)
	{
		FireDamage = GETrue;
	}

	if (Damager.IsItem())
	{
		GEU32 DamagerQuality = Damager.Item.GetQuality();
		GEU32 QualityBurning = 8;
		if ((DamagerQuality & QualityBurning) == QualityBurning)
		{
			FireDamage = GETrue;
		}
	}

	if (!FireDamage)
	{
		return 0;
	}

	if (IsSpellContainer(Damager))
	{
		if (!IsMagicProjectile(Damager))
		{
			Chance = 100;
		}
		else if (IsMagicProjectile(Damager) && Damager.Projectile.PathStyle == gEProjectilePath_Missile)
		{
			Chance = 100;
		}
		else if (QuickCastChance)
		{
			Chance = 10;
		}
	}
	else if (IsNormalProjectile(Damager))
	{
		Chance = static_cast<GEInt>(150 * Damager.Damage.GetProperty<PSDamage::PropertyDamageHitMultiplier>());
	}
	else
	{
		Entity DamagerOwner = Damager.Interaction.GetOwner();
		if (DamagerOwner == None && Damager.Navigation.IsValid())
		{
			DamagerOwner = Damager;
		}
		gEAction DamagerOwnerAction = DamagerOwner.Routine.GetProperty<PSRoutine::PropertyAction>();
		if (DamagerOwnerAction == gEAction::gEAction_PowerAttack)
		{
			Chance = 35;
		}
		else
		{
			Chance = 30;
		}
	}

	if (Victim == Player && Player.Inventory.IsSkillActive("Perk_ResistHeat"))
	{
		Chance = static_cast<GEInt>(Chance * 0.66);
	}

	GEInt Random = randomizer.Random(0, 100);

	if (Random < Chance)
	{
		return 1;
	}

	return 0;
}

GEBool IsValidNpc(Entity NPC)
{
	if (NPC == None)
	{
		return GEFalse;
	}

	if (!NPC.IsNPC())
	{
		return GEFalse;
	}

	if (!NPC.NPC.IsValid())
	{
		return GEFalse;
	}

	return GETrue;
}

GEBool IsDead(Entity NPC)
{
	if (!IsValidNpc(NPC))
	{
		return GEFalse;
	}

	if (NPC.NPC.Species != gESpecies::gESpecies_Human && NPC.NPC.Species != gESpecies::gESpecies_Orc)
	{
		return GEFalse;
	}

	if (NPC.IsDead())
	{
		return GETrue;
	}

	return GEFalse;
}

GEBool TryRevive(Entity NPC)
{
	if (IsDead(NPC))
	{
		NPC.Routine.FullStop();
		NPC.Routine.SetTask("ZS_Unconscious");

		return GETrue;
	}

	return GEFalse;
}

GEBool TryResetPos(Entity NPC)
{
	if (IsValidNpc(NPC))
	{
		NPC.Teleport(NPC.Navigation.GetCurrentDestinationPoint());

		return GETrue;
	}

	return GEFalse;
}

static mCFunctionHook HookOnExecute;
GEBool OnExecuteHook(bCString const & a_rCommand, bCString & a_rStrResult)
{
	bTObjArray< bCString > arrParams = SplitString(a_rCommand, " ", GEFalse, GEFalse);

	GEInt iParamCount = arrParams.GetCount();
	Entity Player = Entity::GetPlayer();

	if (iParamCount > 0 && arrParams[0].CompareNoCaseFast(bCString("resetpos")))
	{
		if (iParamCount > 1)
		{
			bCString Search = arrParams[1];
			auto NPCs = Entity::GetNPCs();

			for (auto Iter = NPCs.Begin(); Iter < NPCs.End(); Iter++)
			{
				if (*Iter == None)
				{
					continue;
				}

				Entity NPC = *Iter;

				if (!NPC.GetName().CompareFast(Search))
					continue;

				if (TryResetPos(NPC))
				{
					a_rStrResult = "Reseted position for NPC: " + NPC.GetName();
					return GETrue;
				}
			}
		}
		else
		{
			Entity Focus = Player.Focus.GetFocusEntity();

			if (TryResetPos(Focus))
			{
				a_rStrResult = "Reseted position for NPC: " + Focus.GetName();
				return GETrue;
			}
		}

		a_rStrResult = "Could not find NPC to reset position";
		return GEFalse;
	}
	else if (iParamCount > 0 && arrParams[0].CompareNoCaseFast(bCString("revive")))
	{
		if (iParamCount > 1)
		{
			bCString Search = arrParams[1];
			auto NPCs = Entity::GetNPCs();

			for (auto Iter = NPCs.Begin(); Iter < NPCs.End(); Iter++)
			{
				if (*Iter == None)
				{
					continue;
				}

				Entity NPC = *Iter;

				if (!NPC.GetName().CompareFast(Search))
				{
					continue;
				}

				if (TryRevive(NPC))
				{
					a_rStrResult = "Revived NPC: " + NPC.GetName();
					return GETrue;
				}
			}
		}
		else
		{
			Entity Focus = Player.Focus.GetFocusEntity();

			if (TryRevive(Focus))
			{
				a_rStrResult = "Revived NPC: " + Focus.GetName();
				return GETrue;
			}
		}

		a_rStrResult = "Could not find NPC to revive";
		return GEFalse;
	}

	return HookOnExecute.GetOriginalFunction(&OnExecuteHook)(a_rCommand, a_rStrResult);
}

static mCFunctionHook Hook_ZS_ObserveSuspect;
GEInt GE_STDCALL ZS_ObserveSuspect(GEInt a0, gCScriptProcessingUnit * a_pSPU)
{
	GEInt result = Hook_ZS_ObserveSuspect.GetOriginalFunction(&ZS_ObserveSuspect)(a0, a_pSPU);

	Entity SelfEntity = Entity(a_pSPU->GetSelfEntity());
	Entity Player = Entity::GetPlayer();

	bCString LastTask = SelfEntity.Routine.GetLastTask();

	if (!LastTask.CompareFast("ZS_ObserveSuspect"))
	{
		auto Ents = SelfEntity.GetEntitiesByDistance();
		for (auto Iter = Ents.Begin(); Iter < Ents.End(); Iter++)
		{
			auto Ent = *Iter;

			if ((GEInt)SelfEntity.GetDistanceTo(Ent) > 2000)
			{
				continue;
			}

			if (Ent.Interaction.GetUser() != SelfEntity)
			{
				continue;
			}

			switch (Ent.Interaction.UseType)
			{
			case gEUseType::gEUseType_Bookstand:
			case gEUseType::gEUseType_Chest:
			case gEUseType::gEUseType_Alchemy:
			case gEUseType::gEUseType_Bed:
			case gEUseType::gEUseType_Anvil:
			case gEUseType::gEUseType_GrindStone:
			case gEUseType::gEUseType_Shrine:
			case gEUseType::gEUseType_PickOre:
			case gEUseType::gEUseType_Cauldron:
				break;
			default:
				continue;
			}

			Ent.Interaction.SetUser(None);
			SelfEntity.Navigation.SetCurrentDestinationPoint(None);
			SelfEntity.StartGoto(SelfEntity.GetEvadeDestination(Ent, gEDirection::gEDirection_Back), gEWalkMode::gEWalkMode_Walk);
		}
	}

	return result;
}

static mCFunctionHook Hook_OnGuidePlayer;
GEBool GE_STDCALL OnGuidePlayer(gCScriptProcessingUnit * a_pSPU)
{
	GEBool result = Hook_OnGuidePlayer.GetOriginalFunction(&OnGuidePlayer)(a_pSPU);

	if (!result)
	{
		return result;
	}

	Entity Guide = a_pSPU->GetSelfEntity();
	Entity Player = Entity::GetPlayer();

	gECharMovementMode PlayerMovement = Player.GetMovementMode();

	if (PlayerMovement == gECharMovementMode::gECharMovementMode_Sprint)
	{
		Guide.SetMovementMode(gECharMovementMode::gECharMovementMode_Sprint);
	}
	else if (PlayerMovement == gECharMovementMode::gECharMovementMode_Walk)
	{
		Guide.SetMovementMode(gECharMovementMode::gECharMovementMode_Walk);
	}
	else
	{
		Guide.SetMovementMode(gECharMovementMode::gECharMovementMode_Run);
	}

	if (!CompanionAutoDefend)
	{
		return result;
	}

	bTObjArray<Entity> Ents = Entity::GetEntities();
	Entity::SortEntityListByDistanceTo(Ents, Player);

	for (GEInt X = 0; X < Ents.GetCount(); X++)
	{
		Entity Ent = Ents.GetAt(X);

		if (Ent == Player)
		{
			continue;
		}

		if (Ent == Guide)
		{
			continue;
		}

		if (Ent.Party.GetPartyLeader() == Player)
		{
			continue;
		}

		if (static_cast<GEInt>(Ent.GetDistanceTo(Player)) > 2000)
		{
			break;
		}

		if (Ent.IsNPC() && Ent.NPC.CombatState == 1 && Guide.IsFreeLOS(Ent, GETrue) && (Ent.NPC.GetCurrentTarget() == Player || Ent.NPC.GetCurrentTarget() == Guide || Ent.NPC.GetCurrentTarget().Party.GetPartyLeader() == Player))
		{
			if (Ent.NPC.AttackReason == gEAttackReason::gEAttackReason_Arena || Ent.NPC.AttackReason == gEAttackReason::gEAttackReason_Duel || Ent.NPC.AttackReason == gEAttackReason::gEAttackReason_None)
			{
				continue;
			}

			gCScriptAdmin & ScriptAdmin = GetScriptAdmin();
			ScriptAdmin.CallScriptFromScript("AssessTarget", &Guide, &Ent, gEAttackReason::gEAttackReason_PlayerCommand);
			break;
		}
	}

	return result;
}

static mCFunctionHook Hook_OnFollowPlayer;
GEBool GE_STDCALL OnFollowPlayer(gCScriptProcessingUnit * a_pSPU)
{
	GEBool result = Hook_OnFollowPlayer.GetOriginalFunction(&OnFollowPlayer)(a_pSPU);

	if (!result)
	{
		return result;
	}

	Entity Follower = Entity(a_pSPU->GetSelfEntity());
	Entity Player = Entity::GetPlayer();

	gECharMovementMode PlayerMovement = Player.GetMovementMode();
	gECharMovementMode FollowerMovement = Follower.GetMovementMode();

	if (FollowerMovement == gECharMovementMode::gECharMovementMode_Walk || FollowerMovement == gECharMovementMode::gECharMovementMode_Run)
	{
		Follower.SetMovementMode(gECharMovementMode::gECharMovementMode_Sprint);
	}

	if (TeleportCompanionTooFarAway)
	{
		eCSceneAdmin & SceneAdmin = *FindModule<eCSceneAdmin>();
		eCEntityAdmin & EntityAdmin = SceneAdmin.GetAccessToEntityAdmin();
		GEFloat radius = EntityAdmin.GetROISphere().GetRadius() - 250.0f;
		if (Follower.GetDistanceTo(Player) >= radius)
		{
			bCMatrix NewPos;
			if (Follower.FindSpawnPose(NewPos, Player, false, 1))
			{
				Follower.MoveTo(NewPos);
			}
			else
			{
				Follower.Teleport(Player);
			}
		}
	}

	if (!CompanionAutoDefend)
	{
		return result;
	}

	bTObjArray<Entity> Ents = Entity::GetEntities();
	Entity::SortEntityListByDistanceTo(Ents, Player);

	for (GEInt X = 0; X < Ents.GetCount(); X++)
	{
		Entity Ent = Ents.GetAt(X);

		if (Ent == Player)
		{
			continue;
		}

		if (Ent == Follower)
		{
			continue;
		}

		if (Ent.Party.GetPartyLeader() == Player)
		{
			continue;
		}

		if (static_cast<GEInt>(Ent.GetDistanceTo(Player)) > 2000)
		{
			break;
		}

		if (Ent.IsNPC() && Ent.NPC.CombatState == 1 && Follower.IsFreeLOS(Ent, GETrue) && (Ent.NPC.GetCurrentTarget() == Player || Ent.NPC.GetCurrentTarget() == Follower || Ent.NPC.GetCurrentTarget().Party.GetPartyLeader() == Player))
		{
			if (Ent.NPC.AttackReason == gEAttackReason::gEAttackReason_Arena || Ent.NPC.AttackReason == gEAttackReason::gEAttackReason_Duel || Ent.NPC.AttackReason == gEAttackReason::gEAttackReason_None)
			{
				continue;
			}

			gCScriptAdmin & ScriptAdmin = GetScriptAdmin();
			ScriptAdmin.CallScriptFromScript("AssessTarget", &Follower, &Ent, gEAttackReason::gEAttackReason_PlayerCommand);
			break;
		}
	}

	return result;
}

static mCFunctionHook Hook_IsDeadlyDamage;
GEInt IsDeadlyDamage(gCScriptProcessingUnit* a_pSPU, Entity* a_pSelfEntity, Entity* a_pOtherEntity, GEU32 a_iArgs) {
	UNREFERENCED_PARAMETER(a_iArgs);
	INIT_SCRIPT_EXT(Victim, Damager);

	gCScriptAdmin& ScriptAdmin = GetScriptAdmin();

	Entity DamagerOwner = Damager.Interaction.GetOwner();
	if (DamagerOwner == None && Damager.Navigation.IsValid())
	{
		DamagerOwner = Damager;
	}

	if (Victim == None || DamagerOwner == None || !ScriptAdmin.CallScriptFromScript("CanBeKilled", &Victim, &None, 0))
	{
		return 0;
	}

	if (ScriptAdmin.CallScriptFromScript("IsBoss", &Victim, &None, 0))
	{
		return 1;
	}

	if (Victim.GetName() == "Xardas")
	{
		return 1;
	}

	if (IsSpellContainer(Damager))
	{
		return 1;
	}

	if (IsNormalProjectile(Damager))
	{
		if (Damager.Damage.GetProperty<PSDamage::PropertyDamageType>() == gEDamageType_Impact && ScriptAdmin.CallScriptFromScript("IsHumanoid", &Victim, &None, 0))
		{
			if (ScriptAdmin.CallScriptFromScript("GetAttitude", &Victim, &DamagerOwner, 0) == gEAttitude::gEAttitude_Hostile
				|| ScriptAdmin.CallScriptFromScript("GetAttitude", &Victim, &DamagerOwner, 0) == gEAttitude::gEAttitude_Panic)
			{
				return 1;
			}

			if (ScriptAdmin.CallScriptFromScript("GetAttitude", &DamagerOwner, &Victim, 0) == gEAttitude::gEAttitude_Hostile
				|| ScriptAdmin.CallScriptFromScript("GetAttitude", &DamagerOwner, &Victim, 0) == gEAttitude::gEAttitude_Panic)
			{
				return 1;
			}

			return 0;
		}

		return 1;
	}

	if (DamagerOwner.Routine.Action != gEAction_FinishingAttack && ScriptAdmin.CallScriptFromScript("IsHumanoid", &DamagerOwner, &None, 0)
		&& ScriptAdmin.CallScriptFromScript("IsHumanoid", &Victim, &None, 0))
	{
		if (ScriptAdmin.CallScriptFromScript("GetAttitude", &Victim, &DamagerOwner, 0) == 4
			|| ScriptAdmin.CallScriptFromScript("GetAttitude", &Victim, &DamagerOwner, 0) == 6)
		{
			return 1;
		}

		if (ScriptAdmin.CallScriptFromScript("GetAttitude", &DamagerOwner, &Victim, 0) == 4
			|| ScriptAdmin.CallScriptFromScript("GetAttitude", &DamagerOwner, &Victim, 0) == 6)
		{
			return 1;
		}

		return 0;
	}

	return 1;
}

mCFunctionHook Hook__AI_UseInventoryItem;
GEBool GE_STDCALL _AI_UseInventoryItem(bTObjStack<gScriptRunTimeSingleState> & a_rRunTimeStack, gCScriptProcessingUnit * a_pSPU)
{
	Entity Player = Entity::GetPlayer();
	Entity SelfEntity = Entity(a_pSPU->GetSelfEntity());
	GEU32 BreakBlock = a_rRunTimeStack.AccessAt(a_rRunTimeStack.GetCount() - 1).m_iBreakBlock;
	gSArgsFor__AI_UseInventoryItem const * pArgs = static_cast<gSArgsFor__AI_UseInventoryItem *>(a_rRunTimeStack.Peek().m_pArguments);

	if (BreakBlock == 7 && SelfEntity == Player)
	{
		auto UsedItem = pArgs->m_Consumer.Inventory.GetTemplateItem(pArgs->m_iIndex);
		bCString ItemFuncs = UsedItem.Interaction.UseFunc;
		if (!ItemFuncs.IsEmpty())
		{
			gCScriptAdmin & ScriptAdmin = GetScriptAdmin();
			bCString delimeter = ";";
			for (GEInt i = 0; i < ItemFuncs.CountWords(delimeter); i++)
			{
				bCString ItemFunc; ItemFuncs.GetWord(i, delimeter, ItemFunc, GETrue, GETrue);
				if (!ItemFunc.IsEmpty())
				{
					ScriptAdmin.CallScriptFromScript(ItemFunc, &UsedItem, &SelfEntity, 0);
				}
			}
		}
	}

	return Hook__AI_UseInventoryItem.GetOriginalFunction(&_AI_UseInventoryItem)(a_rRunTimeStack, a_pSPU);
}

void LoadSettings()
{
	spy.Send(bCString::GetFormattedString("%s - Loading settings", PLUGIN_NAME).GetText());
	eCConfigFile config = eCConfigFile();
	if (config.ReadFile(bCString("G3Fixes.ini")))
	{
		CompanionAutoDefendHotkey = eCApplication::GetInstance().GetKeyboard().GetKeyByName(config.GetString(bCString("Main"), bCString("CompanionAutoDefendHotkey"), bCString("APOSTROPHE")));
		TeleportCompanionTooFarAway = config.GetBool(bCString("Main"), bCString("TeleportCompanionTooFarAway"), TeleportCompanionTooFarAway);
		QuickCastChance = config.GetBool(bCString("Main"), bCString("QuickCastChance"), QuickCastChance);
		BlockMonsterRespawn = config.GetBool(bCString("Main"), bCString("BlockMonsterRespawn"), BlockMonsterRespawn);
		HerdUnityActive = config.GetBool(bCString("Main"), bCString("HerdUnity"), HerdUnityActive);

		RemoveWaterfallSoundEffects = config.GetBool(bCString("Optional"), bCString("RemoveWaterfallSoundEffects"), RemoveWaterfallSoundEffects);

		CompanionIconSize = config.GetI32(bCString("CompanionIcon"), bCString("CompanionIconSize"), CompanionIconSize);
		CompanionIconPosTopX = config.GetFloat(bCString("CompanionIcon"), bCString("CompanionIconPosTopX"), CompanionIconPosTopX);
		CompanionIconPosTopY = config.GetFloat(bCString("CompanionIcon"), bCString("CompanionIconPosTopY"), CompanionIconPosTopY);
	}
	else
	{
		spy.Send(bCString::GetFormattedString("%s - Couldn't find G3Fixes.ini, using default settings", PLUGIN_NAME).GetText());
	}
}

void RemoveWaterfallSounds()
{
	eCSceneAdmin * pSceneAdmin = FindModule<eCSceneAdmin>();
	if (!pSceneAdmin)
	{
		return;
	}

	auto TemplateIterator = pSceneAdmin->m_mapTemplateEntities.Begin();

	while (TemplateIterator != pSceneAdmin->m_mapTemplateEntities.End())
	{
		auto _Template = TemplateIterator.GetNode()->m_Element;
		TemplateIterator++;

		if (!_Template)
		{
			continue;
		}

		auto templatename = _Template->GetName();
		templatename.ToLower();
		if (!templatename.Contains("waterfall"))
		{
			continue;
		}

		if (_Template->HasPropertySet("eCAudioEmitter_PS"))
		{
			spy.Send(bCString::GetFormattedString("%s - Removing audio emitter from: %s", PLUGIN_NAME, _Template->GetName()).GetText());
			_Template->RemovePropertySet("eCAudioEmitter_PS");
		}
	}
}

void RenderIcon(CFFGFCWnd* DesktopWindow)
{
	if (!gCSession::GetSession().IsValid() || gCSession::GetSession().IsPaused() || !gCSession::GetSession().GetGUIManager())
	{
		return;
	}

	if (gCSession::GetInstance().GetGUIManager()->IsMenuOpen() || gCSession::GetInstance().GetGUIManager()->IsAnyPageOpen() || gCInfoManager_PS::GetInstance()->IsRunning)
	{
		return;
	}

	if (eCApplication::GetInstance().GetConsole().IsActive())
	{
		return;
	}

	if (CompanionAutoDefend)
	{
		CompanionIcon.Create("G3Fixes_Companion_Active.png");
	}
	else
	{
		CompanionIcon.Create("G3Fixes_Companion_Passive.png");
	}

	auto DeviceContext = DesktopWindow->GetDC();
	bCRect ClientRect;
	DesktopWindow->GetClientRect(ClientRect);

	GEI32 TopX = (ClientRect.GetWidth() * CompanionIconPosTopX) / 100;
	GEI32 TopY = (ClientRect.GetHeight() * CompanionIconPosTopY) / 100;

	bCRect DrawBox(TopX, TopY, TopX + CompanionIconSize, TopY + CompanionIconSize);
	DeviceContext->DrawBitmap(&CompanionIcon, &DrawBox, 0, 0.0f);
}

static mCFunctionHook Hook__CFFGFCWnd_OnPaint;
void CFFGFCWnd_OnPaint(void) {
	CFFGFCWnd * This = Hook__CFFGFCWnd_OnPaint.GetSelf<CFFGFCWnd *>();

	if (This->GetDlgCtrlID() == 4294967295)
	{
		RenderIcon(This->GetDesktopWindow());
	}

	Hook__CFFGFCWnd_OnPaint.GetOriginalFunction(&CFFGFCWnd_OnPaint)();
}

bTPropertyObject<mCG3Fixes, eCEngineComponentBase> mCG3Fixes::ms_PropertyObjectInstance_mCG3Fixes(GETrue);

void mCG3Fixes::Process(void)
{
	if (!gCSession::GetSession().IsValid() || gCSession::GetSession().IsPaused() || !gCSession::GetSession().GetGUIManager())
	{
		return;
	}

	if (gCSession::GetInstance().GetGUIManager()->IsMenuOpen() || gCSession::GetInstance().GetGUIManager()->IsAnyPageOpen() || gCInfoManager_PS::GetInstance()->IsRunning)
	{
		return;
	}

	if (eCApplication::GetInstance().GetConsole().IsActive())
	{
		return;
	}

	if (eCApplication::GetInstance().GetKeyboard().KeyPressed(CompanionAutoDefendHotkey))
	{
		if (!CompanionAutoDefendHotkeyPressed)
		{
			CompanionAutoDefend = !CompanionAutoDefend;
			CompanionAutoDefendHotkeyPressed = GETrue;
		}
	}
	else
	{
		CompanionAutoDefendHotkeyPressed = GEFalse;
	}
}

mCG3Fixes::~mCG3Fixes(void)
{
	CompanionIcon.Destroy();
}

mCG3Fixes::mCG3Fixes(void)
{
	eCModuleAdmin::GetInstance().RegisterModule(*this);
}

static mCFunctionHook Hook_Test;
void TestHook()
{

}

extern "C" __declspec(dllexport)
gSScriptInit const * GE_STDCALL ScriptInit(void)
{
	GetScriptAdmin().LoadScriptDLL("Script_Game.dll");

	static bCAccessorCreator G3Fixes(bTClassName<mCG3Fixes>::GetUnmangled());

	Hook_Entity_EndTransform.Hook(PROC_Script("?EndTransform@Entity@@QAEXXZ"), &Entity_EndTransform, mCBaseHook::mEHookType_ThisCall);

	Hook__AI_UseInventoryItem.Hook(GetScriptAdminExt().GetScriptAIFunction("_AI_UseInventoryItem")->m_funcScriptAIFunction, &_AI_UseInventoryItem, mCBaseHook::mEHookType_OnlyStack);

	Hook_IsDeadlyDamage.Hook(GetScriptAdminExt().GetScript("IsDeadlyDamage")->m_funcScript, &IsDeadlyDamage);

	Hook_CanBurn.Hook(GetScriptAdminExt().GetScript("CanBurn")->m_funcScript, &CanBurn);

	Hook_CanFreeze.Hook(GetScriptAdminExt().GetScript("CanFreeze")->m_funcScript, &CanFreeze);

	Hook_Respawn.Hook(GetScriptAdminExt().GetScript("Respawn")->m_funcScript, &Respawn);

	Hook_AssessTarget.Hook(GetScriptAdminExt().GetScript("AssessTarget")->m_funcScript, &AssessTarget);

	Hook_ZS_Threaten_Loop.Hook(GetScriptAdminExt().GetScriptAIState("ZS_Threaten_Loop")->m_funcScriptAIState, &ZS_Threaten_Loop, mCBaseHook::mEHookType_OnlyStack);

	Hook_ZS_ObserveSuspect.Hook(GetScriptAdminExt().GetScriptAIState("ZS_ObserveSuspect")->m_funcScriptAIState, &ZS_ObserveSuspect, mCBaseHook::mEHookType_OnlyStack);

	Hook_OnFollowPlayer.Hook(GetScriptAdminExt().GetScriptAICallback("OnFollowPlayer")->m_funcScriptAICallback, &OnFollowPlayer, mCBaseHook::mEHookType_OnlyStack);

	Hook_OnGuidePlayer.Hook(GetScriptAdminExt().GetScriptAICallback("OnGuidePlayer")->m_funcScriptAICallback, &OnGuidePlayer, mCBaseHook::mEHookType_OnlyStack);

	HookOnExecute.Hook(PROC_Engine("?OnExecute@eCConsole@@MAE_NABVbCString@@AAV2@@Z"), &OnExecuteHook, mCFunctionHook::mEHookType_ThisCall);

	Hook__CFFGFCWnd_OnPaint.Hook(PROC_GFC("?OnPaint@CFFGFCWnd@@UAEXXZ"), &CFFGFCWnd_OnPaint, mCBaseHook::mEHookType_ThisCall);

	Hook_AfterApplicationProcess
		.Prepare(RVA_Engine(0x1677C), &AfterApplicationProcess)
		.InsertCall().Hook();

	LoadSettings();

	if (RemoveWaterfallSoundEffects)
	{
		RemoveWaterfallSounds();
	}

	//Hook_Test.Hook(PROC_Engine("?"), &TestHook, mCBaseHook::mEHookType_ThisCall);

	spy.Send(bCString::GetFormattedString("%s - Plugin loaded", PLUGIN_NAME).GetText());

	return &GetScriptInit();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		::DisableThreadLibraryCalls(hModule);
		break;
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
