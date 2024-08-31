/*
        Rewrite of quest log with new context menu features.
        Author bluap/pjbroad Feb 2010
 */
#include <string>
#include <vector>
#include <map>
#include <queue>
#include <set>
#include <cctype>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include "asc.h"
#include "context_menu.h"
#include "dialogues.h"
#include "elconfig.h"
#include "elwindows.h"
#include "errors.h"
#include "gamewin.h"
#include "gl_init.h"
#include "hud.h"
#include "icon_window.h"
#include "init.h"
#include "interface.h"
#include "io/elpathwrapper.h"
#include "multiplayer.h"
#include "notepad.h"
#include "paste.h"
#include "questlog.h"
#include "sound.h"
#include "translate.h"
