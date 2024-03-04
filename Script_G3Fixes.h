#ifndef SCRIPTG3FIXES_H_INCLUDED
#define SCRIPTG3FIXES_H_INCLUDED

#include "util/Memory.h"
#include "util/Logging.h"
#include "util/Hook.h"
#include "util/Util.h"
#include "util/ScriptUtil.h"

#include "SharedBase.h"
#include "Script.h"
#include "Game.h"

gSScriptInit & GetScriptInit();

#include "util/zSpy.h"
#include "util/Random.h"

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

#endif