#include <ultra64.h>
#include "sm64.h"
#include "segment_symbols.h"
#include "level_commands.h"

#include "levels/intro/header.h"

#include "make_const_nonconst.h"

#include "levels/scripts.h"

const LevelScript level_script_entry[] = {
   SET_REG(LEVEL_CASTLE_GROUNDS),
//    SET_REG(LEVEL_WF),
    EXECUTE(0,0,0,level_main_scripts_entry),
    // INIT_LEVEL(),
    // SLEEP(/*frames*/ 2),
    // BLACKOUT(/*active*/ FALSE),
    // SET_REG(/*value*/ 0),
    // EXECUTE(/*seg*/ 0x14, /*script*/ _introSegmentRomStart, /*scriptEnd*/ _introSegmentRomEnd, /*entry*/ level_intro_entry_1),
    // JUMP(/*target*/ level_script_entry),
};
