param(
    [Parameter(Mandatory=$true)][ValidateSet('trust','install','uninstall','cleanup')][string]$Mode,
    [string]$Appx = '',
    [string]$Cer = '',
    [string]$External = '',
    [string]$Name = 'Folcolor',
    [string]$Subject = 'CN=Folcolor',
    [switch]$Machine
)

$ErrorActionPreference = 'Stop'
$log = Join-Path $env:TEMP 'Folcolor-menu.log'

function Write-MenuLog([string]$message) {
    try {
        Add-Content -LiteralPath $log -Value ((Get-Date).ToString('s') + ' [' + $Mode + '] ' + $message)
    } catch {}
}

function Remove-UserRegistration {
    Get-AppxPackage -Name $Name -ErrorAction SilentlyContinue | ForEach-Object {
        Write-MenuLog ('remove user package ' + $_.PackageFullName)
        try { Remove-AppxPackage -Package $_.PackageFullName -ErrorAction Stop }
        catch { Write-MenuLog ('remove user package failed: ' + $_.Exception.Message) }
    }
}

function Remove-MachineRegistration {
    if (-not $Machine) { return }
    try {
        Get-AppxPackage -Name $Name -AllUsers -ErrorAction SilentlyContinue | ForEach-Object {
            Write-MenuLog ('remove machine package ' + $_.PackageFullName)
            try { Remove-AppxPackage -Package $_.PackageFullName -AllUsers -ErrorAction Stop }
            catch { Write-MenuLog ('remove machine package failed: ' + $_.Exception.Message) }
        }
    } catch { Write-MenuLog ('enumerate machine packages failed: ' + $_.Exception.Message) }
    try {
        Get-AppxProvisionedPackage -Online -ErrorAction SilentlyContinue |
            Where-Object { $_.DisplayName -eq $Name } |
            ForEach-Object {
                Write-MenuLog ('remove provisioned package ' + $_.PackageName)
                try { Remove-AppxProvisionedPackage -Online -PackageName $_.PackageName -ErrorAction Stop }
                catch { Write-MenuLog ('remove provisioned failed: ' + $_.Exception.Message) }
            }
    } catch { Write-MenuLog ('enumerate provisioned failed: ' + $_.Exception.Message) }
}

$thumbprint = ''
if ($Cer -and (Test-Path -LiteralPath $Cer)) {
    try {
        $certObject = New-Object System.Security.Cryptography.X509Certificates.X509Certificate2($Cer)
        $thumbprint = $certObject.Thumbprint
    } catch {}
}

function Remove-TrustedCert([string]$store) {
    try {
        Get-ChildItem ('Cert:\' + $store + '\TrustedPeople') -ErrorAction SilentlyContinue |
            Where-Object { ($thumbprint -ne '' -and $_.Thumbprint -eq $thumbprint) -or $_.Subject -eq $Subject } |
            ForEach-Object {
                Write-MenuLog ('remove cert ' + $store + ' ' + $_.Thumbprint)
                try { Remove-Item -LiteralPath ('Cert:\' + $store + '\TrustedPeople\' + $_.Thumbprint) -Force -ErrorAction Stop }
                catch { Write-MenuLog ('remove cert failed: ' + $_.Exception.Message) }
            }
    } catch { Write-MenuLog ('open cert store ' + $store + ' failed: ' + $_.Exception.Message) }
}

switch ($Mode) {
    'cleanup' {
        Remove-MachineRegistration
        Remove-TrustedCert 'LocalMachine'
        exit 0
    }
    'trust' {
        # Add-AppxPackage validates the signature in the machine-scoped AppX
        # deployment service, which does not see the per-user TrustedPeople
        # store. Trust the signing certificate machine-wide (requires admin).
        try {
            Remove-TrustedCert 'LocalMachine'
            Import-Certificate -FilePath $Cer -CertStoreLocation 'Cert:\LocalMachine\TrustedPeople' | Out-Null
            Write-MenuLog 'machine certificate trusted'
        } catch {
            Write-MenuLog ('machine certificate import failed: ' + $_.Exception.Message)
            exit 4
        }
        exit 0
    }
    'install' {
        Remove-UserRegistration
        Remove-TrustedCert 'CurrentUser'
        try {
            Add-AppxPackage -Path $Appx -ExternalLocation $External -ErrorAction Stop
            Write-MenuLog 'package registered'
        } catch {
            Write-MenuLog ('package registration failed: ' + $_.Exception.Message)
            exit 3
        }
        exit 0
    }
    'uninstall' {
        Remove-UserRegistration
        Remove-TrustedCert 'CurrentUser'
        Remove-TrustedCert 'LocalMachine'
        exit 0
    }
}
exit 9
