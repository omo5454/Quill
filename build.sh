#!/usr/bin/env bash

# ---- Quill Build Script -----------------------------------------------------
# Usage:
#   ./build.sh            - builds Linux binaries (or fallback to host)
#   ./build.sh -t all     - builds all platforms
#   ./build.sh -t win
#   ./build.sh -t mac
#   ./build.sh -t mac-arm
#   ./build.sh -c         - removes bin/ directory
# -----------------------------------------------------------------------------

# Exit on unset variables
set -u

# ---- Defaults ---------------------------------------------------------------
TARGET="linux"
CLEAN=false

# ---- Config -----------------------------------------------------------------
BIN_DIR="bin"
QUILL_SRC="./cmd/quill"
QUILLC_SRC="./cmd/quill-c"
VERSION="1.1.3.1"

# ---- Color Codes ------------------------------------------------------------
COLOR_RESET="\033[0m"
COLOR_GRAY="\033[90m"
COLOR_CYAN="\033[36m"
COLOR_GREEN="\033[32m"
COLOR_RED="\033[31m"
COLOR_YELLOW="\033[33m"

# ---- Helpers ----------------------------------------------------------------
write_header() {
    echo -e ""
    echo -e "${COLOR_GRAY}-----------------------------------------${COLOR_RESET}"
    echo -e "${COLOR_CYAN} $1${COLOR_RESET}"
    echo -e "${COLOR_GRAY}-----------------------------------------${COLOR_RESET}"
}

write_success() {
    echo -e "${COLOR_GREEN} [OK] $1${COLOR_RESET}"
}

write_fail() {
    echo -e "${COLOR_RED} [FAIL] $1${COLOR_RESET}"
}

build_binary() {
    local os="$1"
    local arch="$2"
    local out_quill="$3"
    local out_quillc="$4"

    echo -e "${COLOR_GRAY}  Building quill...${COLOR_RESET}"
    GOOS="$os" GOARCH="$arch" go build -o "$BIN_DIR/$out_quill" "$QUILL_SRC"
    if [ $? -ne 0 ]; then
        write_fail "Failed to build quill for $os/$arch"
        return 1
    fi
    write_success "quill -> $BIN_DIR/$out_quill"

    echo -e "${COLOR_GRAY}  Building quill-c...${COLOR_RESET}"
    GOOS="$os" GOARCH="$arch" go build -o "$BIN_DIR/$out_quillc" "$QUILLC_SRC"
    if [ $? -ne 0 ]; then
        write_fail "Failed to build quill-c for $os/$arch"
        return 1
    fi
    write_success "quill-c -> $BIN_DIR/$out_quillc"

    return 0
}

# ---- Parse Arguments --------------------------------------------------------
while [[ $# -gt 0 ]]; do
    case "$1" in
        -t|--target)
            TARGET="$2"
            shift 2
            ;;
        -c|--clean)
            CLEAN=true
            shift
            ;;
        *)
            write_fail "Unknown option: $1"
            echo -e "${COLOR_GRAY}Valid options: -t|--target [win|linux|mac|mac-arm|all], -c|--clean${COLOR_RESET}"
            exit 1
            ;;
    esac
done

# ---- Clean ------------------------------------------------------------------
if [ "$CLEAN" = true ]; then
    write_header "Cleaning bin/"
    if [ -d "$BIN_DIR" ]; then
        rm -rf "$BIN_DIR"
        write_success "bin/ removed"
    else
        echo -e "${COLOR_GRAY}  bin/ does not exist, nothing to clean${COLOR_RESET}"
    fi
    exit 0
fi

# ---- Setup ------------------------------------------------------------------
write_header "Quill Build v$VERSION"

if [ ! -d "$BIN_DIR" ]; then
    mkdir -p "$BIN_DIR"
    echo -e "${COLOR_GRAY}  Created bin/${COLOR_RESET}"
fi

if ! command -v go &> /dev/null; then
    write_fail "Go is not installed or not in PATH"
    exit 1
fi

GO_VERSION=$(go version)
echo -e "${COLOR_GRAY}  Using $GO_VERSION${COLOR_RESET}"

# ---- Builds -----------------------------------------------------------------
SUCCESS=true

# Convert target to lowercase
TARGET=$(echo "$TARGET" | tr '[:upper:]' '[:lower:]')

case "$TARGET" in
    "win")
        write_header "Building for Windows (x64)"
        build_binary "windows" "amd64" "quill.exe" "quill-c.exe" || SUCCESS=false
        ;;

    "linux")
        write_header "Building for Linux (x64)"
        build_binary "linux" "amd64" "quill-linux" "quill-c-linux" || SUCCESS=false
        ;;

    "mac")
        write_header "Building for macOS (Intel x64)"
        build_binary "darwin" "amd64" "quill-macos-intel" "quill-c-macos-intel" || SUCCESS=false
        ;;

    "mac-arm")
        write_header "Building for macOS (Apple Silicon arm64)"
        build_binary "darwin" "arm64" "quill-macos-arm" "quill-c-macos-arm" || SUCCESS=false
        ;;

    "all")
        write_header "Building for all platforms"

        echo -e ""
        echo -e "${COLOR_YELLOW}  Windows x64${COLOR_RESET}"
        build_binary "windows" "amd64" "quill.exe" "quill-c.exe" || SUCCESS=false

        echo -e ""
        echo -e "${COLOR_YELLOW}  Linux x64${COLOR_RESET}"
        build_binary "linux" "amd64" "quill-linux" "quill-c-linux" || SUCCESS=false

        echo -e ""
        echo -e "${COLOR_YELLOW}  macOS Intel x64${COLOR_RESET}"
        build_binary "darwin" "amd64" "quill-macos-intel" "quill-c-macos-intel" || SUCCESS=false

        echo -e ""
        echo -e "${COLOR_YELLOW}  macOS Apple Silicon arm64${COLOR_RESET}"
        build_binary "darwin" "arm64" "quill-macos-arm" "quill-c-macos-arm" || SUCCESS=false
        ;;

    *)
        write_fail "Unknown target: $TARGET"
        echo -e "${COLOR_GRAY}  Valid targets: win, linux, mac, mac-arm, all${COLOR_RESET}"
        exit 1
        ;;
esac

# ---- Summary ----------------------------------------------------------------
write_header "Build Summary"

if [ "$SUCCESS" = true ]; then
    echo -e ""
    echo -e "${COLOR_GREEN}  All binaries built successfully${COLOR_RESET}"
    echo -e ""
    echo -e "${COLOR_GRAY}  Output files:${COLOR_RESET}"
    
    # Loop through files in bin/ and calculate file size
    for file in "$BIN_DIR"/*; do
        if [ -f "$file" ]; then
            filename=$(basename "$file")
            
            # Cross-platform file size calculation (Linux & macOS friendly)
            if [[ "$OSTYPE" == "darwin"* ]]; then
                bytes=$(stat -f%z "$file")
            else
                bytes=$(stat -c%s "$file")
            fi
            
            # Convert bytes to MB with 2 decimal places using awk
            size=$(awk -v b="$bytes" 'BEGIN {printf "%.2f", b/1048576}')
            echo -e "    $filename ($size MB)"
        fi
    done
    echo -e ""
    exit 0
else
    echo -e ""
    write_fail "Build completed with errors"
    echo -e ""
    exit 1
fi