#!/usr/bin/env bash
# Setup script for Autosaw_main
# Usage: ./setup.sh [repository-url] [branch]

set -euo pipefail

REPO_URL=${1:-https://github.com/USERNAME/Autosaw_main.git}
BRANCH=${2:-master}
PROJECT_DIR=${REPO_URL##*/}
PROJECT_DIR=${PROJECT_DIR%.git}

# Allow overriding the Arduino data directory. This lets pre-downloaded
# board packages be used without accessing the network.
ARDUINO_DATA_PATH=${ARDUINO_DATA_PATH:-"$PWD/arduino_data"}
export ARDUINO_DATA_PATH
mkdir -p "$ARDUINO_DATA_PATH" "$ARDUINO_DATA_PATH/staging" "$PWD/Arduino"

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
directories:
  data: "$ARDUINO_DATA_PATH"
  downloads: "$ARDUINO_DATA_PATH/staging"
  user: "$PWD/Arduino"
EOCONF

arduino-cli --config-file arduino-cli.yaml core update-index
arduino-cli --config-file arduino-cli.yaml core install ClearCore:sam@1.7.0

# Install required libraries
arduino-cli --config-file arduino-cli.yaml lib install ClearCore genieArduinoDEV || true

# Compile the project
arduino-cli --config-file arduino-cli.yaml compile --fqbn ClearCore:sam:ClearCore Autosaw_main.ino

# Uncomment the line below to upload (requires connected board)
# arduino-cli upload -p /dev/ttyACM0 --fqbn ClearCore:sam:ClearCore Autosaw_main.ino

