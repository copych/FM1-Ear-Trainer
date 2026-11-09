#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
node tools/emu-controls-test.cjs
python3 tools/test-hub-metadata.py
sh tools/build-melodee.sh game-test
sh tools/build-melodee.sh ui-test
sh tools/build-melodee.sh input-test
sh tools/build-melodee.sh settings-test
sh tools/build-melodee.sh web-test
python3 melodee/tools/fm1_recover.py melodee/build/melodee.fwsc --check
node tools/serve-site.cjs 8011 /FM1-Ear-Trainer/ > build/pages-server.log 2>&1 &
server_pid=$!
trap 'kill "$server_pid" 2>/dev/null || true' EXIT
for attempt in {1..20}; do
  if curl -fsS http://127.0.0.1:8011/FM1-Ear-Trainer/build-info.json > /dev/null; then break; fi
  sleep 1
done
node tools/check-site.cjs http://127.0.0.1:8011/FM1-Ear-Trainer/
