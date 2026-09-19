#!/usr/bin/env bash
# ==============================================================================
# ilrmuzika — Automated Setup & Build Script
# Supports: Linux (Debian/Ubuntu, Arch, Fedora), macOS, Android (Termux)
# ==============================================================================

set -e

OS="$(uname -s)"
APP_NAME="ilrmuzika"

check_command() {
    command -v "$1" >/dev/null 2>&1
}

print_dep() {
    local name="$1"
    local command="$2"
    if check_command "$command"; then
        printf "  %-16s \033[32m[INSTALLED]\033[0m\n" "$name"
        return 0
    else
        printf "  %-16s \033[31m[MISSING]\033[0m\n" "$name"
        return 1
    fi
}

echo ""
echo "=================================================="
echo "           ilrmuzika Installer & Build            "
echo "=================================================="
echo ""

IS_TERMUX=false
if [ -n "$TERMUX_VERSION" ] || [ -d "/data/data/com.termux" ]; then
    IS_TERMUX=true
    PLATFORM="Termux (Android)"
elif [ "$OS" = "Darwin" ]; then
    PLATFORM="macOS"
elif [ "$OS" = "Linux" ]; then
    PLATFORM="Linux"
else
    echo "Error: Unsupported operating system: $OS"
    exit 1
fi

echo "Platform detected: $PLATFORM"
echo ""
echo "--- Checking Dependencies ---"

MISSING=()
if ! print_dep "ffmpeg" "ffmpeg"; then MISSING+=("ffmpeg"); fi
if ! print_dep "yt-dlp" "yt-dlp"; then MISSING+=("yt-dlp"); fi
if ! print_dep "cmake" "cmake"; then MISSING+=("cmake"); fi

if [ "$IS_TERMUX" = true ]; then
    if ! print_dep "clang" "clang"; then MISSING+=("clang"); fi
    if ! print_dep "make" "make"; then MISSING+=("make"); fi
    if ! print_dep "python" "python"; then MISSING+=("python"); fi
    PYTHON_CMD="python"
else
    if ! print_dep "make" "make"; then MISSING+=("make"); fi
    if ! print_dep "c++ compiler" "c++"; then MISSING+=("build-essential"); fi
    if ! print_dep "python3" "python3"; then MISSING+=("python3"); fi
    PYTHON_CMD="python3"
fi

# ------------------------------------------------------------------------------
# Auto-Install Missing Dependencies
# ------------------------------------------------------------------------------
if [ ${#MISSING[@]} -gt 0 ]; then
    echo ""
    echo "Missing required packages: ${MISSING[*]}"
    echo "Attempting automated installation..."

    if [ "$IS_TERMUX" = true ]; then
        pkg update -y
        pkg install -y clang make cmake ffmpeg python
        if ! check_command yt-dlp; then
            pip install yt-dlp
        fi
    elif [ "$OS" = "Darwin" ]; then
        if ! check_command brew; then
            echo "Error: Homebrew required on macOS. Install from https://brew.sh/"
            exit 1
        fi
        brew install cmake ffmpeg yt-dlp python3
    elif [ "$OS" = "Linux" ]; then
        if check_command apt-get; then
            sudo apt-get update
            sudo apt-get install -y cmake build-essential ffmpeg python3 python3-pip
            if ! check_command yt-dlp; then
                sudo apt-get install -y yt-dlp 2>/dev/null || pip3 install --user yt-dlp
            fi
        elif check_command pacman; then
            sudo pacman -Sy --noconfirm cmake base-devel ffmpeg yt-dlp python python-pip
        elif check_command dnf; then
            sudo dnf install -y cmake gcc-c++ make ffmpeg yt-dlp python3 python3-pip
        else
            echo "Error: Package manager not recognized. Please install: ${MISSING[*]}"
            exit 1
        fi
    fi
fi

# Python requests dependency for lyrics
if ! "$PYTHON_CMD" -c "import requests" >/dev/null 2>&1; then
    echo "Installing optional python package: requests (for synchronized lyrics)..."
    pip3 install --user requests 2>/dev/null || pip install requests 2>/dev/null || true
fi

# ------------------------------------------------------------------------------
# Configure Directories
# ------------------------------------------------------------------------------
CONFIG_DIR="$HOME/.config/ilrmuzika"
mkdir -p "$CONFIG_DIR"
if [ ! -f "$CONFIG_DIR/config.txt" ]; then
    if [ -f "config.txt" ]; then
        cp config.txt "$CONFIG_DIR/config.txt"
    fi
fi

# yt-dlp client fix
YTDLP_DIR="$HOME/.config/yt-dlp"
mkdir -p "$YTDLP_DIR"
if [ ! -f "$YTDLP_DIR/config" ]; then
    echo '--extractor-args "youtube:player_client=android"' > "$YTDLP_DIR/config"
fi

# ------------------------------------------------------------------------------
# Build
# ------------------------------------------------------------------------------
echo ""
echo "--- Compiling $APP_NAME (Release, -O3) ---"
mkdir -p build
cmake -B build -DCMAKE_BUILD_TYPE=Release
CORES=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
cmake --build build -j"$CORES"

BINARY="./build/$APP_NAME"
if [ ! -f "$BINARY" ]; then
    echo "Build failed: binary $BINARY not found."
    exit 1
fi

echo ""
echo "Compile complete: $BINARY"

# ------------------------------------------------------------------------------
# Installation to PATH
# ------------------------------------------------------------------------------
if [ "$IS_TERMUX" = true ]; then
    INSTALL_DIR="$PREFIX/bin"
else
    INSTALL_DIR="$HOME/.local/bin"
fi

mkdir -p "$INSTALL_DIR"
cp "$BINARY" "$INSTALL_DIR/$APP_NAME"
chmod +x "$INSTALL_DIR/$APP_NAME"

echo ""
echo "=================================================="
echo "  Success! $APP_NAME installed to: $INSTALL_DIR/$APP_NAME"
echo "  Run with:"
echo "    $APP_NAME"
echo "=================================================="
