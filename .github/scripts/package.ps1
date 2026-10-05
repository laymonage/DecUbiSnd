param(
    [Parameter(Mandatory)][ValidateSet('Debug', 'Release')][string]$Configuration,
    [Parameter(Mandatory)][ValidateSet('cli', 'gui', 'all')][string]$Target,
    [Parameter(Mandatory)][string]$Output
)

$ErrorActionPreference = 'Stop'
$root = Resolve-Path (Join-Path $PSScriptRoot '..\..')
$vcpkg = Join-Path $root 'vcpkg_installed\x86-windows'
$bin = if ($Configuration -eq 'Debug') { Join-Path $vcpkg 'debug\bin' } else { Join-Path $vcpkg 'bin' }

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

# Find the vcpkg DLLs the executables need, following DLL-to-DLL imports.
# Import names are stored as plain ASCII in the PE file, so searching the bytes is enough.
$available = Get-ChildItem $bin -Filter *.dll
$needed = [System.Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
$queue = [System.Collections.Generic.Queue[string]]::new()
$exes | ForEach-Object { $queue.Enqueue($_) }
while ($queue.Count -gt 0) {
    $file = $queue.Dequeue()
    $text = [Text.Encoding]::GetEncoding(28591).GetString([IO.File]::ReadAllBytes($file))
    foreach ($dll in $available) {
        if ($text.IndexOf($dll.Name, [StringComparison]::OrdinalIgnoreCase) -ge 0 -and $needed.Add($dll.Name)) {
            $queue.Enqueue($dll.FullName)
        }
    }
}
foreach ($name in $needed) { Copy-Item (Join-Path $bin $name) $Output }
Write-Host "Bundled DLLs: $($needed -join ', ')"

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
