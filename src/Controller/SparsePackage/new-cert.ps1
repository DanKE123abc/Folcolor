# Folcolor (c) 2020 Kevin Weatherman - MIT license
#
# Generates the self-signed code-signing certificate used to sign the
# Win11 sparse package (Folcolor-sparse.appx). Run once; the output
# (Folcolor.pfx / Folcolor.cer) is consumed by build-sparse-pkg.ps1.
#
# Production releases should replace this cert with a real one from a
# trusted CA (e.g. Azure Trusted Signing) by providing Folcolor.pfx / .cer.

$ErrorActionPreference = "Stop"

$here = $PSScriptRoot
$pfxPath  = Join-Path $here "Folcolor.pfx"
$cerPath  = Join-Path $here "Folcolor.cer"
$pfxPass  = "Folcolor-Sparse-2026"

if ((Test-Path $pfxPath) -and (Test-Path $cerPath)) {
    Write-Host "Certificate already exists, skipping generation."
    exit 0
}

$store = "Cert:\CurrentUser\My"
$old = Get-ChildItem $store -ErrorAction SilentlyContinue | Where-Object { $_.Subject -match "CN=Folcolor" }
if ($old) {
    $old | Remove-Item -Force
}

$cert = New-SelfSignedCertificate `
    -Type Custom `
    -KeyUsage DigitalSignature `
    -CertStoreLocation $store `
    -TextExtension @("2.5.29.37={text}1.3.6.1.5.5.7.3.3", "2.5.29.19={text}") `
    -Subject "CN=Folcolor" `
    -FriendlyName "Folcolor Sparse Package" `
    -KeyExportPolicy Exportable `
    -KeyAlgorithm RSA `
    -KeyLength 2048 `
    -NotAfter (Get-Date).AddYears(5)

$password = ConvertTo-SecureString -String $pfxPass -Force -AsPlainText

Export-PfxCertificate -Cert $cert -FilePath $pfxPath -Password $password | Out-Null
Export-Certificate -Cert $cert -FilePath $cerPath | Out-Null

Write-Host "Created $pfxPath and $cerPath"