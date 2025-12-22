#!/bin/bash
#
# QupZilla Relaunch Test Script
# Tests for intermittent crash on relaunch by performing multiple launch/close cycles
#
# Usage:
#   ./test-relaunch.sh [num_cycles] [fresh_install]
#
# Arguments:
#   num_cycles   - Number of launch/close cycles to perform (default: 10)
#   fresh_install - If "fresh", will uninstall/reinstall first (default: no)
#

set -e

NUM_CYCLES=${1:-10}
FRESH_INSTALL=${2:-""}
APP_ID="com.nizovn.qupzilla"
IPK_FILE="package-staging/com.nizovn.qupzilla_2.3.0_all.ipk"
LOG_FILE="/tmp/qupzilla-relaunch-test.log"
DEVICE_LOG="/media/internal/qupzilla/startup.log"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "=============================================="
echo "QupZilla Relaunch Test Script"
echo "=============================================="
echo "Cycles to run: $NUM_CYCLES"
echo "Fresh install: ${FRESH_INSTALL:-no}"
echo ""

# Function to log with timestamp
log() {
    local msg="[$(date '+%H:%M:%S')] $1"
    echo -e "$msg" | tee -a "$LOG_FILE"
}

# Function to check if device is connected
check_device() {
    if ! novacom -l 2>/dev/null | grep -q .; then
        echo -e "${RED}ERROR: No webOS device connected${NC}"
        echo "Please connect your TouchPad via USB and enable developer mode"
        exit 1
    fi
    log "Device connected"
}

# Function to check if app is running
is_app_running() {
    palm-launch -l 2>/dev/null | grep -q "$APP_ID" && return 0 || return 1
}

# Function to wait for app to start (checks log for specific message)
wait_for_startup() {
    local timeout=$1
    local started=0
    local elapsed=0

    while [ $elapsed -lt $timeout ]; do
        # Pull log and check for successful startup indicator
        local log_content=$(novacom get file://$DEVICE_LOG 2>/dev/null || echo "")

        if echo "$log_content" | grep -q "Entering event loop"; then
            started=1
            break
        fi

        if echo "$log_content" | grep -q "CRASH\|SIGSE\|Aborted\|Killed"; then
            log "${RED}CRASH DETECTED in log${NC}"
            return 2
        fi

        sleep 2
        elapsed=$((elapsed + 2))
    done

    if [ $started -eq 1 ]; then
        return 0
    else
        return 1
    fi
}

# Function to get last N lines of device log
get_log_tail() {
    local lines=${1:-20}
    novacom get file://$DEVICE_LOG 2>/dev/null | tail -$lines
}

# Initialize log
echo "QupZilla Relaunch Test - $(date)" > "$LOG_FILE"
echo "==========================================" >> "$LOG_FILE"

# Check device connection
check_device

# Fresh install if requested
if [ "$FRESH_INSTALL" = "fresh" ]; then
    log "${YELLOW}Performing fresh install...${NC}"

    # Close app if running
    palm-launch -c $APP_ID 2>/dev/null || true
    sleep 5

    # Uninstall
    log "Uninstalling $APP_ID..."
    palm-install -r $APP_ID 2>/dev/null || true
    sleep 5

    # Build if needed
    if [ ! -f "$IPK_FILE" ]; then
        log "Building package..."
        cd package-staging && palm-package $APP_ID && cd ..
    fi

    # Install
    log "Installing from $IPK_FILE..."
    if [ -f "$IPK_FILE" ]; then
        palm-install "$IPK_FILE"
    else
        log "${RED}ERROR: IPK file not found: $IPK_FILE${NC}"
        exit 1
    fi
    sleep 5
    log "${GREEN}Fresh install complete${NC}"
fi

# Run launch/close cycles
SUCCESSFUL=0
FAILED=0

for i in $(seq 1 $NUM_CYCLES); do
    echo ""
    log "========== CYCLE $i of $NUM_CYCLES =========="

    # Close app if running from previous cycle
    if is_app_running; then
        log "Closing app from previous run..."
        palm-launch -c $APP_ID 2>/dev/null || true
        sleep 10
    fi

    # Launch app
    log "Launching app..."
    palm-launch $APP_ID

    # Wait for startup (60 seconds timeout for slow hardware)
    log "Waiting for startup (60s timeout)..."
    if wait_for_startup 60; then
        log "${GREEN}✓ Cycle $i: App started successfully${NC}"
        SUCCESSFUL=$((SUCCESSFUL + 1))

        # Let it run for a few seconds
        sleep 5

        # Show log tail
        log "Log tail:"
        get_log_tail 10 | while read line; do
            echo "    $line" | tee -a "$LOG_FILE"
        done
    else
        exit_code=$?
        if [ $exit_code -eq 2 ]; then
            log "${RED}✗ Cycle $i: CRASHED${NC}"
        else
            log "${RED}✗ Cycle $i: Failed to start (timeout)${NC}"
        fi
        FAILED=$((FAILED + 1))

        # Save crash log
        log "Saving crash log..."
        get_log_tail 50 > "/tmp/qupzilla-crash-cycle-$i.log"

        log "Last 30 lines of log:"
        get_log_tail 30 | while read line; do
            echo "    $line" | tee -a "$LOG_FILE"
        done

        # Wait before continuing
        sleep 15
    fi

    # Close app before next cycle
    log "Closing app..."
    palm-launch -c $APP_ID 2>/dev/null || true
    sleep 15
done

# Summary
echo ""
echo "=============================================="
log "TEST SUMMARY"
echo "=============================================="
log "Total cycles: $NUM_CYCLES"
log "${GREEN}Successful: $SUCCESSFUL${NC}"
log "${RED}Failed: $FAILED${NC}"

if [ $FAILED -eq 0 ]; then
    log "${GREEN}ALL TESTS PASSED!${NC}"
    exit 0
else
    log "${RED}SOME TESTS FAILED${NC}"
    log "Check /tmp/qupzilla-crash-cycle-*.log for crash logs"
    exit 1
fi
