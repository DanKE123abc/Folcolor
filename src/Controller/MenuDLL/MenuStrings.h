// Folcolor(tm) (c) 2020 Kevin Weatherman
// MIT license https://opensource.org/licenses/MIT
//
// Self-contained string table for the Win11 sparse-package context menu DLL.
// Kept deliberately isolated (no dependency on Language.cpp) so the DLL is the
// single binary that owns display strings for the modern context menu.
#pragma once
#include <windows.h>

// Language index order MUST match LANG_ID in Language.h.
enum MFACOLOR_LANG
{
    MLANG_EN = 0,
    MLANG_ZH_CN,
    MLANG_ZH_TW,
    MLANG_JA,
    MLANG_KO,
    MLANG_COUNT
};

// Menu label ids. Order matches STR_COLOR_FOLDER / STR_RESTORE_DEFAULT / STR_LAUNCH_FOLCOLOR.
enum MFACOLOR_MENUID
{
    MMENU_COLOR_FOLDER = 0,
    MMENU_RESTORE_DEFAULT = 1,
    MMENU_LAUNCH = 2
};

static const wchar_t* mfColorNames[MLANG_COUNT][15] =
{
    // EN
    { L"Red", L"Pink", L"Purple", L"Blue", L"Cyan", L"Teal", L"Green", L"Lime", L"Yellow", L"Orange", L"Brown", L"Grey", L"Blue Grey", L"Black", L"White" },
    // ZH-CN
    { L"红色", L"粉色", L"紫色", L"蓝色", L"青色", L"蓝绿色", L"绿色", L"浅绿色", L"黄色", L"橙色", L"棕色", L"灰色", L"蓝灰", L"黑色", L"白色" },
    // ZH-TW
    { L"紅色", L"粉色", L"紫色", L"藍色", L"青色", L"藍綠色", L"綠色", L"淺綠色", L"黃色", L"橙色", L"棕色", L"灰色", L"藍灰", L"黑色", L"白色" },
    // JA
    { L"赤", L"ピンク", L"紫", L"青", L"シアン", L"ティール", L"緑", L"ライム", L"黄色", L"オレンジ", L"茶色", L"グレー", L"ブルーグレー", L"黒", L"白" },
    // KO
    { L"빨강", L"분홍", L"보라", L"파랑", L"청록", L"청록색", L"초록", L"라임", L"노랑", L"주황", L"갈색", L"회색", L"파란 회색", L"검정", L"하양" },
};

static const wchar_t* mfMenuLabels[MLANG_COUNT][3] =
{
    { L"Color Folder", L"Restore Default", L"Launch Folcolor" },
    { L"文件夹颜色", L"恢复默认", L"启动 Folcolor" },
    { L"資料夾彩色", L"還原預設", L"啟動 Folcolor" },
    { L"カラーをフォルダ", L"既定に戻す", L"Folcolor を起動" },
    { L"색상 폴더", L"기본 복원", L"Folcolor 실행" },
};

static int mfDetectLanguage()
{
    wchar_t locale[64] = { 0 };
    if (GetUserDefaultLocaleName(locale, _countof(locale)) != 0)
    {
        if (wcsncmp(locale, L"zh-CN", 4) == 0 || wcsncmp(locale, L"zh-Hans", 6) == 0)
            return MLANG_ZH_CN;
        if (wcsncmp(locale, L"zh-TW", 4) == 0 || wcsncmp(locale, L"zh-Hant", 6) == 0 || wcsncmp(locale, L"zh-HK", 4) == 0)
            return MLANG_ZH_TW;
        if (wcsncmp(locale, L"ja", 2) == 0)
            return MLANG_JA;
        if (wcsncmp(locale, L"ko", 2) == 0)
            return MLANG_KO;
        return MLANG_EN;
    }

    LANGID langId = GetUserDefaultLangID();
    if (PRIMARYLANGID(langId) == LANG_CHINESE)
        return MLANG_ZH_CN;
    if (PRIMARYLANGID(langId) == LANG_JAPANESE)
        return MLANG_JA;
    if (PRIMARYLANGID(langId) == LANG_KOREAN)
        return MLANG_KO;
    return MLANG_EN;
}

static const wchar_t* mfColorName(int lang, int index)
{
    if (lang < 0 || lang >= MLANG_COUNT || index < 0 || index >= 15)
        return mfColorNames[MLANG_EN][0];
    return mfColorNames[lang][index];
}

static const wchar_t* mfMenuLabel(int lang, int id)
{
    if (lang < 0 || lang >= MLANG_COUNT || id < 0 || id >= 3)
        return mfMenuLabels[MLANG_EN][0];
    return mfMenuLabels[lang][id];
}
