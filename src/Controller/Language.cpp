
// Folcolor(tm) (c) 2020 Kevin Weatherman
// MIT license https://opensource.org/licenses/MIT
#include "StdAfx.h"
#include "Language.h"

// Color name translations - index order matches nameTable in Installer.cpp/Folcolor (Red, Pink, Purple, etc.)
static const wchar_t* colorNames[LANG_COUNT][COLOR_LABEL_COUNT] = {
    // EN
    { L"Red", L"Pink", L"Purple", L"Blue", L"Cyan", L"Teal", L"Green", L"Lime", L"Yellow", L"Orange", L"Brown", L"Grey", L"Blue Grey", L"Black", L"White" },
    // ZH-CN (Simplified Chinese)
    { L"红色", L"粉色", L"紫色", L"蓝色", L"青色", L"蓝绿色", L"绿色", L"浅绿色", L"黄色", L"橙色", L"棕色", L"灰色", L"蓝灰", L"黑色", L"白色" },
    // ZH-TW (Traditional Chinese)
    { L"紅色", L"粉色", L"紫色", L"藍色", L"青色", L"藍綠色", L"綠色", L"淺綠色", L"黃色", L"橙色", L"棕色", L"灰色", L"藍灰", L"黑色", L"白色" },
    // JA (Japanese)
    { L"赤", L"ピンク", L"紫", L"青", L"シアン", L"ティール", L"緑", L"ライム", L"黄色", L"オレンジ", L"茶色", L"グレー", L"ブルーグレー", L"黒", L"白" },
    // KO (Korean)
    { L"빨강", L"분홍", L"보라", L"파랑", L"청록", L"청록색", L"초록", L"라임", L"노랑", L"주황", L"갈색", L"회색", L"파란 회색", L"검정", L"하양" },
};

const wchar_t* GetColorName(LANG_ID lang, int colorIndex) {
    if (lang < 0 || lang >= LANG_COUNT || colorIndex < 0 || colorIndex >= COLOR_LABEL_COUNT)
        return colorNames[LANG_EN][0];
    return colorNames[lang][colorIndex];
}


LANG_ID g_currentLang = LANG_EN;
BOOL g_isWin11 = FALSE;

static const wchar_t* langStrings[LANG_COUNT][14] =
{
    {
        L"Color Folder",
        L"Restore Default",
        L"Launch Folcolor",
        L"Install",
        L"Uninstall",
        L"Uninstall Folcolor?",
        L"Completion:",
        L"Installation complete.",
        L"Folcolor uninstalled.",
        L"Folcolor registry uninstalled, but to complete the uninstallation manually delete the",
        L"folder after this dialog closes.",
        L"Abort:",
        L"Folcolor detects this as possibly a special folder (I.E. Downloads, Documents, Music, etc.) and is not supported since restoring them is complex.",
        L"If you really want to set the color/icon for this folder, manually edit or just delete the existing desktop.ini (hidden, system) file first.",
    },
    {
        L"文件夹颜色",
        L"恢复默认",
        L"启动 Folcolor",
        L"安装",
        L"卸载",
        L"卸载 Folcolor?",
        L"完成：",
        L"安装完成。",
        L"Folcolor 已卸载。",
        L"Folcolor 注册表已卸载，但为完成卸载，请在关闭此对话框后手动删除",
        L"文件夹。",
        L"中止：",
        L"Folcolor 检测到这可能是一个特殊文件夹（例如下载、音乐、文档等），不受支持，因为恢复它们很复杂。",
        L"如果真的想为此文件夹设置颜色/图标，请先手动编辑或删除现有的 desktop.ini（隐藏、系统）文件。",
    },
    {
        L"資料夾彩色",
        L"還原預設",
        L"啟動 Folcolor",
        L"安裝",
        L"解除安裝",
        L"解除安裝 Folcolor?",
        L"完成：",
        L"安裝完成。",
        L"Folcolor 已解除安裝。",
        L"Folcolor 登入檔已解除安裝，但為完成解除安裝，請在關閉此對話方塊後手動刪除",
        L"資料夾。",
        L"中止：",
        L"Folcolor 偵測到這可能是一個特殊資料夾（例如下載、音樂、文件等），不受支援，因為還原它們很複雜。",
        L"如果真的想為此資料夾設定顏色/圖示，請先手動編輯或刪除現有的 desktop.ini（隱藏、系統）檔案。",
    },
    {
        L"カラーをフォルダ",
        L"既定に戻す",
        L"Folcolor を起動",
        L"インストール",
        L"アンインストール",
        L"Folcolor をアンインストールしますか？",
        L"完了：",
        L"インストールが完了しました。",
        L"Folcolor がアンインストールされました。",
        L"Folcolor のレジストリがアンインストールされましたが、アンインストールを完了するには、このダイアログを閉じた後、手動で",
        L"フォルダを削除してください。",
        L"中止：",
        L"Folcolor は特殊フォルダ（ダウンロード、ドキュメント、音楽など）と判断する可能性があり、これら，恢复は複雑なためサポートされていません。",
        L"このフォルダに色/アイコンを設定する場合は、まず既存の desktop.ini（隠し、システム）ファイルを編集または削除してください。",
    },
    {
        L"색상 폴더",
        L"기본 복원",
        L"Folcolor 실행",
        L"설치",
        L"제거",
        L"Folcolor을(를) 제거하시겠습니까?",
        L"완료:",
        L"설치가 완료되었습니다.",
        L"Folcolor이(가) 제거되었습니다.",
        L"Folcolor 레지스트리가 제거되었지만, 제거를 완전히 완료하려면 이 대화상자가 닫힌 후 수동으로",
        L"폴더를 삭제해야 합니다.",
        L"중단:",
        L"Folcolor은 특수 폴더(예: 다운로드, 문서, 음악 등)로 감지되며 복원이 복잡하므로 지원되지 않습니다.",
        L"이 폴더에 색상/아이콘을 설정하려면 먼저 기존 desktop.ini(숨김, 시스템) 파일을 수동으로 편집하거나 삭제하세요.",
    }
};

static LANG_ID DetectLanguageByLocale(LPCWSTR localeName)
{
    if (!localeName)
        return LANG_EN;

    if (wcsncmp(localeName, L"zh-CN", 4) == 0 || wcsncmp(localeName, L"zh-Hans", 6) == 0)
        return LANG_ZH_CN;

    if (wcsncmp(localeName, L"zh-TW", 4) == 0 || wcsncmp(localeName, L"zh-Hant", 6) == 0 || wcsncmp(localeName, L"zh-HK", 4) == 0)
        return LANG_ZH_TW;

    if (wcsncmp(localeName, L"ja", 2) == 0)
        return LANG_JA;

    if (wcsncmp(localeName, L"ko", 2) == 0)
        return LANG_KO;

    return LANG_EN;
}

LANG_ID DetectSystemLanguage()
{
    WCHAR localeName[64] = { 0 };

    if (GetUserDefaultLocaleName(localeName, _countof(localeName)) != 0)
        return DetectLanguageByLocale(localeName);

    LANGID langId = GetUserDefaultLangID();

    if (PRIMARYLANGID(langId) == LANG_CHINESE)
        return LANG_ZH_CN;
    if (PRIMARYLANGID(langId) == LANG_JAPANESE)
        return LANG_JA;
    if (PRIMARYLANGID(langId) == LANG_KOREAN)
        return LANG_KO;

    return LANG_EN;
}

const wchar_t* GetLangString(LANG_ID lang, int stringId)
{
    if (lang < 0 || lang >= LANG_COUNT || stringId < 0 || stringId >= 14)
        return langStrings[LANG_EN][0];

    return langStrings[lang][stringId];
}