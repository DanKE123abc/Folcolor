// Folcolor(tm) (c) 2020 Kevin Weatherman
// MIT license https://opensource.org/licenses/MIT
#pragma once

#include <windows.h>

enum LANG_ID
{
    LANG_EN = 0,
    LANG_ZH_CN,
    LANG_ZH_TW,
    LANG_JA,
    LANG_KO,
    LANG_COUNT
};

extern LANG_ID g_currentLang;

LANG_ID DetectSystemLanguage();
const wchar_t* GetLangString(LANG_ID lang, int stringId);

#define STR_COLOR_FOLDER 0
#define STR_RESTORE_DEFAULT 1
#define STR_LAUNCH_FOLCOLOR 2
#define STR_INSTALL 3
#define STR_UNINSTALL 4
#define STR_CONFIRM_UNINSTALL 5
#define STR_COMPLETION 6
#define STR_INSTALL_COMPLETE 7
#define STR_UNINSTALL_COMPLETE 8
#define STR_REGISTRY_UNINSTALLED 9
#define STR_MANUAL_DELETE 10
#define STR_ABORT 11
#define STR_SPECIAL_FOLDER 12
#define STR_MANUAL_EDIT 13

// Color label count
#define COLOR_LABEL_COUNT 15

// Get color label in the appropriate language
const wchar_t* GetColorName(LANG_ID lang, int colorIndex);
