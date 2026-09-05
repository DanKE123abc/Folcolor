# Folcolor (c) 2020 Kevin Weatherman - MIT license
# Builds the Win11 sparse package that registers Folcolor's modern context
# menu. Run AFTER Controller.sln has been built so that
# Win32\Release\Folcolor.exe and Win32\Release\FolcolorMenu.dll exist.
#
# Produces: Win32\Release\Folcolor-sparse.appx (signed).

$ErrorActionPreference = "Stop"

$here      = Split-Path -Parent $PSScriptRoot
$outDir    = Join-Path $here "Win32\Release"
$sparseDir = $PSScriptRoot

if (-not (Test-Path (Join-Path $outDir "Folcolor.exe"))) {
    throw "Folcolor.exe not found in $outDir - build Controller.sln first."
}
if (-not (Test-Path (Join-Path $outDir "FolcolorMenu.dll"))) {
    throw "FolcolorMenu.dll not found in $outDir - build Controller.sln first."
}

$pfxPath = Join-Path $sparseDir "Folcolor.pfx"
if (-not (Test-Path $pfxPath)) {
    & (Join-Path $sparseDir "new-cert.ps1")
}
if (-not (Test-Path $pfxPath)) {
    throw "Folcolor.pfx missing - run new-cert.ps1 first."
}

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
$pkgSrc   = Join-Path $here "Win32\Release\sparse-pkg"
$appxOut  = Join-Path $outDir "Folcolor-sparse.appx"

if (Test-Path $pkgSrc) { Remove-Item $pkgSrc -Recurse -Force }
New-Item -ItemType Directory -Path $pkgSrc -Force | Out-Null
Copy-Item (Join-Path $sparseDir "AppxManifest.xml") (Join-Path $pkgSrc "AppxManifest.xml") -Force

# Pack (signed .appx, files live in ExternalLocation at registration time).
& $makeappx pack /d $pkgSrc /p $appxOut /nv
if ($LASTEXITCODE -ne 0) { throw "MakeAppx failed ($LASTEXITCODE)" }

& $signtool sign /fd SHA256 /a /f $pfxPath /p "Folcolor-Sparse-2026" $appxOut
if ($LASTEXITCODE -ne 0) { throw "SignTool failed ($LASTEXITCODE)" }

Write-Host "Created $appxOut"