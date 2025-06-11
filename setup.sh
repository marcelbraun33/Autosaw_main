#!/usr/bin/env bash
# Setup script for Autosaw_main
# Usage: ./setup.sh [repository-url] [branch]

set -euo pipefail

REPO_URL=${1:-https://github.com/USERNAME/Autosaw_main.git}
BRANCH=${2:-master}
PROJECT_DIR=${REPO_URL##*/}
PROJECT_DIR=${PROJECT_DIR%.git}

# Clone the repository if it doesn't exist
if [ ! -d "$PROJECT_DIR" ]; then
    git clone "$REPO_URL" -b "$BRANCH" "$PROJECT_DIR"
fi

cd "$PROJECT_DIR"

# Install arduino-cli if not already installed
if ! command -v arduino-cli >/dev/null 2>&1; then
    curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
    sudo mv bin/arduino-cli /usr/local/bin/arduino-cli
    rm -rf bin
fi

# Configure arduino-cli for ClearCore board support
cat <<EOCONF > arduino-cli.yaml
board_manager:
  additional_urls:
    - https://teknic-inc.github.io/clearcore/package_ClearCore_index.json
EOCONF

arduino-cli core update-index
arduino-cli core install ClearCore:sam@1.7.0

# Install required libraries
arduino-cli lib install ClearCore genieArduinoDEV || true

# Compile the project
arduino-cli compile --fqbn ClearCore:sam:ClearCore Autosaw_main.ino

# Uncomment the line below to upload (requires connected board)
# arduino-cli upload -p /dev/ttyACM0 --fqbn ClearCore:sam:ClearCore Autosaw_main.ino

