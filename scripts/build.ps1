[CmdletBinding()]
param(
    [ValidateSet('montech/mkey_pro_75:via', 'montech/mkey_pro_75:default')]
    [string]$Target = 'montech/mkey_pro_75:via',
    [string]$QmkDir,
    [string]$Image = 'mkey-qmk:666862c',
    [ValidateRange(1, 64)]
    [int]$Jobs = 8
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (-not $QmkDir) { $QmkDir = Join-Path $root 'qmk_firmware' }
if (-not (Test-Path (Join-Path $QmkDir 'Makefile'))) {
    throw 'Run scripts/setup.ps1 or point -QmkDir at the complete source from the release.'
}
$QmkDir = (Resolve-Path $QmkDir).Path

$destination = Join-Path $QmkDir 'keyboards/montech'
New-Item -ItemType Directory -Force -Path $destination | Out-Null
Copy-Item -Recurse -Force (Join-Path $root 'keyboards/montech/mkey_pro_75') $destination
Copy-Item -Recurse -Force (Join-Path $root 'overlay/keyboards') $QmkDir

docker build -t $Image -f (Join-Path $PSScriptRoot 'Dockerfile') $QmkDir
if ($LASTEXITCODE) { throw 'Docker image build failed.' }

$name = $Target -replace '[:/]', '_'
docker run --rm -e QMK_USERSPACE= -e SKIP_GIT=1 -v "${QmkDir}:/qmk_firmware" -w /qmk_firmware $Image make $Target "-j$Jobs"
if ($LASTEXITCODE) { throw 'QMK build failed.' }

$outDir = Join-Path $root 'firmware'
New-Item -ItemType Directory -Force -Path $outDir | Out-Null
Copy-Item (Join-Path $QmkDir ".build/$name.bin") (Join-Path $outDir "$name.bin") -Force
Write-Host (Join-Path $outDir "$name.bin")
