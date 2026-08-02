param(
    [string]$Target = "linux",
    [switch]$Clean
)

$BinDir = "bin"
$Version = "1.3.1"

function Write-Header {
    param([string]$Message)
    Write-Host ""
    Write-Host "-----------------------------------------" -ForegroundColor DarkGray
    Write-Host " $Message" -ForegroundColor Cyan
    Write-Host "-----------------------------------------" -ForegroundColor DarkGray
}

function Write-Success {
    param([string]$Message)
    Write-Host " [OK] $Message" -ForegroundColor Green
}

function Write-Fail {
    param([string]$Message)
    Write-Host " [FAIL] $Message" -ForegroundColor Red
}

function Build-Cpp-Binary {
    param(
        [string]$Source,
        [string]$Output,
        [string]$Label
    )

    Write-Host "  Building $Label..." -ForegroundColor Gray
    & g++ -std=c++17 $Source -O2 -o "$BinDir/$Output"
    if ($LASTEXITCODE -ne 0) {
        Write-Fail "Failed to build $Label"
        return $false
    }

    Write-Success "$Label -> $BinDir/$Output"
    return $true
}

if ($Clean) {
    Write-Header "Cleaning build artifacts"
    if (Test-Path $BinDir) {
        Remove-Item -Recurse -Force $BinDir
        Write-Success "Removed $BinDir"
    } else {
        Write-Host "  $BinDir does not exist" -ForegroundColor Gray
    }
    exit 0
}

New-Item -ItemType Directory -Path $BinDir -Force | Out-Null
Write-Header "Quill C++ Build v$Version"

$success = $true

switch ($Target.ToLower()) {
    "linux" {
        $success = Build-Cpp-Binary -Source "src/core/transpiler/trans.cpp" -Output "quill" -Label "transpiler"
        if (-not $success) { exit 1 }

        $success = Build-Cpp-Binary -Source "src/core/interpreter/interpreter.cpp" -Output "quill-c" -Label "interpreter"
        if (-not $success) { exit 1 }
    }
    "all" {
        $success = Build-Cpp-Binary -Source "src/core/transpiler/trans.cpp" -Output "quill" -Label "transpiler"
        if (-not $success) { exit 1 }

        $success = Build-Cpp-Binary -Source "src/core/interpreter/interpreter.cpp" -Output "quill-c" -Label "interpreter"
        if (-not $success) { exit 1 }
    }
    default {
        Write-Fail "Unsupported target: $Target"
        Write-Host "  Supported targets: linux, all" -ForegroundColor Gray
        exit 1
    }
}

Write-Header "Build Summary"
Get-ChildItem $BinDir | ForEach-Object {
    Write-Host "  $($_.Name)" -ForegroundColor White
}

Write-Success "Build complete"

