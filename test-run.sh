#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
palm-run "${SCRIPT_DIR}/package-staging/com.nizovn.qupzilla"
