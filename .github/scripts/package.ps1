param(
    [Parameter(Mandatory)][ValidateSet('Debug', 'Release')][string]$Configuration,
    [Parameter(Mandatory)][ValidateSet('cli', 'gui', 'all')][string]$Target,
    [Parameter(Mandatory)][string]$Output
)

$ErrorActionPreference = 'Stop'
$root = Resolve-Path (Join-Path $PSScriptRoot '..\..')
$vcpkg = Join-Path $root 'vcpkg_static\x86-windows-static-md'
$ffmpegVcpkg = Join-Path $root 'vcpkg_ffmpeg\installed\x86-windows'

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

$ffmpegRuntime = if ($Configuration -eq 'Debug') {
    Join-Path $ffmpegVcpkg 'debug\bin'
} else {
    Join-Path $ffmpegVcpkg 'bin'
}
$ffmpegDlls = Get-ChildItem $ffmpegRuntime -Filter '*.dll' -File
if (-not $ffmpegDlls) { throw "FFmpeg runtime DLLs are missing from $ffmpegRuntime" }
Copy-Item $ffmpegDlls.FullName $Output -Force

if ($Configuration -eq 'Release') {
    if (-not (Get-Command upx -ErrorAction SilentlyContinue)) { throw 'upx is required for Release packaging. Install it with: winget install UPX.UPX' }
    foreach ($exe in $exes) {
        & upx --best --quiet (Join-Path $Output (Split-Path $exe -Leaf))
        if ($LASTEXITCODE -ne 0) { throw "upx failed on $exe" }
    }
}

if ($Configuration -eq 'Release') {
    Copy-Item (Join-Path $root 'README.md'), (Join-Path $root 'LICENSE') $Output
    $ports = @('ffmpeg', 'libogg', 'libvorbis')
    if ($Target -in 'gui', 'all') { $ports += 'wxwidgets' }
    foreach ($port in $ports) {
        $dest = Join-Path $Output "licenses\$port"
        New-Item $dest -ItemType Directory -Force | Out-Null
        $licenseRoot = if ($port -eq 'ffmpeg') { $ffmpegVcpkg } else { $vcpkg }
        Copy-Item (Join-Path $licenseRoot "share\$port\copyright") $dest
        if ($port -eq 'ffmpeg') {
            Copy-Item (Join-Path $licenseRoot 'share\ffmpeg\vcpkg.spdx.json') $dest
        }
    }
}
