# Folcolor (c) 2020 Kevin Weatherman - MIT license
#
# Builds the Win11 sparse package that registers Folcolor's modern context
# menu. Run AFTER the solution has been built (x86 app) and the menu DLL has
# been built for the target architectures.
#
# Required inputs:
#   Win32\Release\Folcolor.exe
#   x64\Release\FolcolorMenu.dll
# Optional inputs (packaged only when present):
#   Win32\Release\FolcolorMenu.dll   -> FolcolorMenu.x86.dll
#   ARM64\Release\FolcolorMenu.dll   -> FolcolorMenu.arm64.dll
#
# Produces in Win32\Release:
#   Folcolor-sparse.appx, Folcolor.cer, FolcolorMenu.x64.dll, ...
#
# Signing uses FOLCOLOR_PFX_BASE64 / FOLCOLOR_PFX_PASSWORD when provided
# (CI secret), otherwise a local self-signed certificate is generated.

param(
    [string]$Version = "1.2.0.0",
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"

$here      = Split-Path -Parent $PSScriptRoot
$outDir    = Join-Path $here "Win32\$Configuration"
$sparseDir = $PSScriptRoot

$exePath = Join-Path $outDir "Folcolor.exe"
if (-not (Test-Path $exePath)) {
    throw "Folcolor.exe not found in $outDir - build Controller.sln first."
}

$dllX64 = Join-Path $here "x64\$Configuration\FolcolorMenu.dll"
if (-not (Test-Path $dllX64)) {
    throw "FolcolorMenu.dll (x64) not found at $dllX64 - build MenuDLL for x64 first."
}

$dllX86 = Join-Path $outDir "FolcolorMenu.dll"
$dllArm = Join-Path $here "ARM64\$Configuration\FolcolorMenu.dll"

function Find-SdkTool([string]$tool) {
    $kitsBase = "${env:ProgramFiles(x86)}/Windows Kits/10/bin"
    if (Test-Path $kitsBase) {
        $dirs = Get-ChildItem $kitsBase -Directory -ErrorAction SilentlyContinue |
            Sort-Object Name -Descending
        foreach ($d in $dirs) {
            $cand = Join-Path (Join-Path $d.FullName "x64") "$tool.exe"
            if (Test-Path $cand) { return $cand }
        }
    }
    $cmd = Get-Command $tool -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    throw "$tool.exe not found. Install the Windows SDK."
}

$makeappx = Find-SdkTool "MakeAppx"
$signtool = Find-SdkTool "SignTool"

# --- signing certificate -----------------------------------------------------
$pfxPath = Join-Path $sparseDir "Folcolor.pfx"
$pfxPass = $env:FOLCOLOR_PFX_PASSWORD
if (-not $pfxPass) { $pfxPass = "Folcolor-Sparse-2026" }

$temporaryPfx = $false
if ($env:FOLCOLOR_PFX_BASE64) {
    [IO.File]::WriteAllBytes($pfxPath, [Convert]::FromBase64String($env:FOLCOLOR_PFX_BASE64))
    $temporaryPfx = $true
}
elseif (-not (Test-Path $pfxPath)) {
    & (Join-Path $sparseDir "new-cert.ps1")
}
if (-not (Test-Path $pfxPath)) {
    throw "Folcolor.pfx missing - run new-cert.ps1 first."
}

$cert = New-Object System.Security.Cryptography.X509Certificates.X509Certificate2($pfxPath, $pfxPass)
$subject = $cert.Subject

# --- manifest ----------------------------------------------------------------
$pkgSrc = Join-Path $outDir "sparse-pkg"
$appxOut = Join-Path $outDir "Folcolor-sparse.appx"

if (Test-Path $pkgSrc) { Remove-Item $pkgSrc -Recurse -Force }
New-Item -ItemType Directory -Path $pkgSrc -Force | Out-Null

$manifestText = Get-Content -LiteralPath (Join-Path $sparseDir "AppxManifest.xml") -Raw
$manifestText = $manifestText -replace '(<Identity[^>]*?Version=")[^"]*(")', ('${1}' + $Version + '${2}')
$manifestText = $manifestText -replace 'Publisher="[^"]*"', ('Publisher="' + $subject + '"')
[IO.File]::WriteAllText((Join-Path $pkgSrc "AppxManifest.xml"), $manifestText,
    (New-Object System.Text.UTF8Encoding($false)))

# --- pack + sign -------------------------------------------------------------
& $makeappx pack /o /d $pkgSrc /p $appxOut /nv
if ($LASTEXITCODE -ne 0) { throw "MakeAppx failed ($LASTEXITCODE)" }

& $signtool sign /fd SHA256 /f $pfxPath /p $pfxPass $appxOut
if ($LASTEXITCODE -ne 0) { throw "SignTool failed ($LASTEXITCODE)" }

$cerBytes = $cert.Export([System.Security.Cryptography.X509Certificates.X509ContentType]::Cert)
[IO.File]::WriteAllBytes((Join-Path $outDir "Folcolor.cer"), $cerBytes)

# --- registration script + per-architecture DLLs ----------------------------
Copy-Item -LiteralPath (Join-Path $sparseDir "menu.ps1") -Destination (Join-Path $outDir "menu.ps1") -Force
Copy-Item -LiteralPath $dllX64 -Destination (Join-Path $outDir "FolcolorMenu.x64.dll") -Force
if (Test-Path $dllX86) {
    Copy-Item -LiteralPath $dllX86 -Destination (Join-Path $outDir "FolcolorMenu.x86.dll") -Force
}
if (Test-Path $dllArm) {
    Copy-Item -LiteralPath $dllArm -Destination (Join-Path $outDir "FolcolorMenu.arm64.dll") -Force
}

if ($temporaryPfx) {
    Remove-Item -LiteralPath $pfxPath -Force -ErrorAction SilentlyContinue
}

Write-Host "Created $appxOut"
Write-Host "Publisher: $subject"
Write-Host "Menu DLLs:"
Get-ChildItem (Join-Path $outDir "FolcolorMenu.*.dll") | ForEach-Object { Write-Host ("  " + $_.Name) }
