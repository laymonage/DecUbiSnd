param(
    [Parameter(Mandatory)][ValidateSet('Debug', 'Release')][string]$Configuration,
    [Parameter(Mandatory)][ValidateSet('cli', 'gui', 'all')][string]$Target,
    [Parameter(Mandatory)][string]$Output
)

$ErrorActionPreference = 'Stop'
$root = Resolve-Path (Join-Path $PSScriptRoot '..\..')
$vcpkg = Join-Path $root 'vcpkg_static\x86-windows-static-md'

$exes = @()
if ($Target -in 'cli', 'all') { $exes += Join-Path $root "Decoding\$Configuration\DecUbiSnd.exe" }
if ($Target -in 'gui', 'all') { $exes += Join-Path $root "Gui\$Configuration\DecUbiSndGui.exe" }

if (Test-Path $Output) { Remove-Item $Output -Recurse -Force }
New-Item $Output -ItemType Directory | Out-Null

foreach ($exe in $exes) {
    Copy-Item $exe $Output
    $pdb = [IO.Path]::ChangeExtension($exe, '.pdb')
    if ($Configuration -eq 'Debug' -and (Test-Path $pdb)) { Copy-Item $pdb $Output }
}

if ($Configuration -eq 'Release') {
    Copy-Item (Join-Path $root 'README.md'), (Join-Path $root 'LICENSE') $Output
    $ports = @('libogg', 'libvorbis')
    if ($Target -in 'gui', 'all') { $ports += 'wxwidgets' }
    foreach ($port in $ports) {
        $dest = Join-Path $Output "licenses\$port"
        New-Item $dest -ItemType Directory -Force | Out-Null
        Copy-Item (Join-Path $vcpkg "share\$port\copyright") $dest
    }
}
