#pragma once

#include <g3sdk/Script.h>
#include <g3sdk/util/Hook.h>
#include <g3sdk/util/Util.h>
#include <g3sdk/util/Logging.h>
#include <g3sdk/util/Memory.h>
#include <g3sdk/util/ScriptUtil.h>

#include "SharedBase.h"
#include "Script.h"
#include "Game.h"

inline GELPVoid PROC_GFC(GELPCChar a_strName) {
	return GetProcAddress("GFC.dll", a_strName);
}

#define SCRIPT_PARAMS a_pSPU, a_pSelfEntity, a_pOtherEntity, a_iArgs

gSScriptInit & GetScriptInit();

#include "zSpy.h"
#include "Random.h"

#include <algorithm>

class mCG3Fixes :
	public eCEngineComponentBase
{
public: virtual void	Process(void);
public: virtual			~mCG3Fixes(void);

private:
	static bTPropertyObject<mCG3Fixes, eCEngineComponentBase> ms_PropertyObjectInstance_mCG3Fixes;

public:
	mCG3Fixes(void);

private:
	mCG3Fixes(mCG3Fixes const &);
	mCG3Fixes const &	operator = (mCG3Fixes const &);
};

enum class AttributeRequirementFixMode : int
{
	Disabled,
	DontAddEquipmentBonus,
	AddEquipmentBonus
};

bCString FormatTime(GEInt total)
{
	GEInt minutes, seconds;
	minutes = total / 60;
	seconds = total % 60;

	bCString ret;
	ret.Format("%02d:%02d", minutes, seconds);

	return ret;
}

#define PLUGIN_NAME "G3Fixes"

GEBool CompanionAutoDefend = GEFalse;
GEBool CompanionAutoDefendHotkeyPressed = GEFalse;
eCInpShared::eEKeyboardStateOffset CompanionAutoDefendHotkey = eCInpShared::eEKeyboardStateOffset::eEKeyboardStateOffset_APOSTROPHE;
GEBool TeleportCompanionTooFarAway = GETrue;
GEInt QuickCastFreezeChancePercent = 10;
GEInt QuickCastBurnChancePercent = 10;
GEInt MeleeFreezeChancePercent = 30;
GEInt MeleeFreezeChancePercentPowerAttack = 35;
GEInt MeleeBurnChancePercent = 30;
GEInt MeleeBurnChancePercentPowerAttack = 30;
GEBool BlockMonsterRespawn = GEFalse;
GEBool RemoveWaterfallSoundEffects = GEFalse;
GEBool AlwaysShowPercentageProtection = GEFalse;

static CFFGFCBitmap CompanionIcon;
GEI32 CompanionIconSize = 16;
GEFloat CompanionIconPosTopX = 98.5;
GEFloat CompanionIconPosTopY = 2.5;

static CFFGFCBitmap TransformationIcon;
GEI32 TransformationIconSize = 24;
GEFloat TransformationIconPosTopX = 0.5f;
GEFloat TransformationIconPosTopY = 6.0f;

GEBool HerdUnityActive = GEFalse;

AttributeRequirementFixMode CurrentAttributeRequirementFixModeSkills = AttributeRequirementFixMode::Disabled;
AttributeRequirementFixMode CurrentAttributeRequirementFixModeEquipment = AttributeRequirementFixMode::Disabled;
