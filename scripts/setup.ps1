[CmdletBinding()]
param([string]$QmkDir)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (-not $QmkDir) { $QmkDir = Join-Path $root 'qmk_firmware' }
$revision = '666862cb8123b64a6b96718d739c6203ad99031f'
$repo = 'https://github.com/Keychron/qmk_firmware.git'

if (-not (Test-Path (Join-Path $QmkDir '.git'))) {
    if (Test-Path $QmkDir) { throw "Choose an unused folder for QMK: $QmkDir" }
    git init $QmkDir
    if ($LASTEXITCODE) { throw 'Could not initialize the QMK directory.' }
}
$head = git -C $QmkDir rev-parse --verify --quiet HEAD
if ($head -and $head -ne $revision) {
    throw "The QMK directory must be at $revision. Use a separate directory with -QmkDir."
}
if (-not $head) {
    git -C $QmkDir fetch --depth 1 $repo $revision
    if ($LASTEXITCODE) { throw 'Could not fetch the pinned Keychron source.' }
    git -C $QmkDir checkout --detach FETCH_HEAD
    if ($LASTEXITCODE) { throw 'Could not check out the pinned Keychron source.' }
}
git -C $QmkDir submodule update --init --depth 1 lib/chibios lib/chibios-contrib lib/printf
if ($LASTEXITCODE) { throw 'Could not install QMK dependencies.' }
Write-Host "QMK is ready in $QmkDir"
