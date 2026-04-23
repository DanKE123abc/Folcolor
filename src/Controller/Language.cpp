
// Folcolor(tm) (c) 2020 Kevin Weatherman
// MIT license https://opensource.org/licenses/MIT
#include "StdAfx.h"
#include "Language.h"

// Color name translations - index order matches nameTable in Installer.cpp/Folcolor (Red, Pink, Purple, etc.)
static const char* colorNames[LANG_COUNT][COLOR_LABEL_COUNT] = {
    // EN
    { "Red", "Pink", "Purple", "Blue", "Cyan", "Teal", "Green", "Lime", "Yellow", "Orange", "Brown", "Grey", "Blue Grey", "Black", "White" },
    // ZH-CN (Simplified Chinese)
    { "红色", "粉色", "紫色", "蓝色", "青色", "蓝绿色", "绿色", "浅绿色", "黄色", "橙色", "棕色", "灰色", "蓝灰", "黑色", "白色" },
    // ZH-TW (Traditional Chinese)
    { "紅色", "粉色", "紫色", "藍色", "青色", "藍綠色", "綠色", "淺綠色", "黃色", "橙色", "棕色", "灰色", "藍灰", "黑色", "白色" },
    // JA (Japanese)
    { "赤", "ピンク", "紫", "青", "シアン", "ティール", "緑", "ライム", "黄色", "オレンジ", "茶色", "グレー", "ブルーグレー", "黒", "白" },
    // KO (Korean)
    { "빨강", "분홍", "보라", "파랑", "청록", "청록색", "초록", "라임", "노랑", "주황", "갈색", "회색", "파란 회색", "검정", "하양" },
};

const char* GetColorName(LANG_ID lang, int colorIndex) {
    if (lang < 0 || lang >= LANG_COUNT || colorIndex < 0 || colorIndex >= COLOR_LABEL_COUNT)
        return colorNames[LANG_EN][0];
    return colorNames[lang][colorIndex];
}


LANG_ID g_currentLang = LANG_EN;

static const char* langStrings[LANG_COUNT][14] =
{
    {
        "Color Folder",
        "Restore Default",
        "Launch Folcolor",
        "Install",
        "Uninstall",
        "Uninstall Folcolor?",
        "Completion:",
        "Installation complete.",
        "Folcolor uninstalled.",
        "Folcolor registry uninstalled, but to complete the uninstallation manually delete the",
        "folder after this dialog closes.",
        "Abort:",
        "Folcolor detects this as possibly a special folder (I.E. Downloads, Documents, Music, etc.) and is not supported since restoring them is complex.",
        "If you really want to set the color/icon for this folder, manually edit or just delete the existing desktop.ini (hidden, system) file first.",
    },
    {
        "彩色文件夹",
        "恢复默认",
        "启动 Folcolor",
        "安装",
        "卸载",
        "卸载 Folcolor?",
        "完成：",
        "安装完成。",
        "Folcolor 已卸载。",
        "Folcolor 注册表已卸载，但为完成卸载，请在关闭此对话框后手动删除",
        "文件夹。",
        "中止：",
        "Folcolor 检测到这可能是一个特殊文件夹（例如下载、音乐、文档等），不受支持，因为恢复它们很复杂。",
        "如果真的想为此文件夹设置颜色/图标，请先手动编辑或删除现有的 desktop.ini（隐藏、系统）文件。",
    },
    {
        "彩色資料夾",
        "還原預設",
        "啟動 Folcolor",
        "安裝",
        "解除安裝",
        "解除安裝 Folcolor?",
        "完成：",
        "安裝完成。",
        "Folcolor 已解除安裝。",
        "Folcolor 登入檔已解除安裝，但為完成解除安裝，請在關閉此對話方塊後手動刪除",
        "資料夾。",
        "中止：",
        "Folcolor 偵測到這可能是一個特殊資料夾（例如下載、音樂、文件等），不受支援，因為還原它們很複雜。",
        "如果真的想為此資料夾設定顏色/圖示，請先手動編輯或刪除現有的 desktop.ini（隱藏、系統）檔案。",
    },
    {
        "カラーをフォルダ",
        "既定に戻す",
        "Folcolor を起動",
        "インストール",
        "アンインストール",
        "Folcolor をアンインストールしますか？",
        "完了：",
        "インストールが完了しました。",
        "Folcolor がアンインストールされました。",
        "Folcolor のレジストリがアンインストールされましたが、アンインストールを完了するには、このダイアログを閉じた後、手動で",
        "フォルダを削除してください。",
        "中止：",
        "Folcolor は特殊フォルダ（ダウンロード、ドキュメント、音楽など）と判断する可能性があり、これら，恢复は複雑なためサポートされていません。",
        "このフォルダに色/アイコンを設定する場合は、まず既存の desktop.ini（隠し、システム）ファイルを編集または削除してください。",
    },
    {
        "색상 폴더",
        "기본 복원",
        "Folcolor 실행",
        "설치",
        "제거",
        "Folcolor을(를) 제거하시겠습니까?",
        "완료:",
        "설치가 완료되었습니다.",
        "Folcolor이(가) 제거되었습니다.",
        "Folcolor 레지스트리가 제거되었지만, 제거를 완전히 완료하려면 이 대화상자가 닫힌 후 수동으로",
        "폴더를 삭제해야 합니다.",
        "중단:",
        "Folcolor은 특수 폴더(예: 다운로드, 문서, 음악 등)로 감지되며 복원이 복잡하므로 지원되지 않습니다.",
        "이 폴더에 색상/아이콘을 설정하려면 먼저 기존 desktop.ini(숨김, 시스템) 파일을 수동으로 편집하거나 삭제하세요.",
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

const char* GetLangString(LANG_ID lang, int stringId)
{
    if (lang < 0 || lang >= LANG_COUNT || stringId < 0 || stringId >= 14)
        return langStrings[LANG_EN][0];

    return langStrings[lang][stringId];
}