#!/usr/bin/env bash

# Run from the tests/ directory
BIN="../Project2"
EXPECTED_DIR="expected"
CURRENT_DIR="current"

mkdir -p "$CURRENT_DIR"

pass=0
fail=0

shopt -s nullglob
tests=( "$EXPECTED_DIR"/test-*.expected )
shopt -u nullglob

if (( ${#tests[@]} == 0 )); then
  echo "No expected files found in '$EXPECTED_DIR'."
  exit 1
fi

for exp in "${tests[@]}"; do
  base="${exp##*/}"              # e.g., test-00.expected
  id="${base%.expected}"         # e.g., test-00

  code_file="${id}.sstack"
  out_file="${CURRENT_DIR}/${id}.current"
  status_file="${EXPECTED_DIR}/${id}.status"

  if [[ ! -x "$BIN" ]]; then
    echo "Missing executable: $BIN"
    exit 1
  fi
  if [[ ! -f "$code_file" ]]; then
    echo "Missing code file: $code_file"
    ((fail++))
    continue
  fi

  # Run, capture BOTH stdout and stderr, and the exit code
  "$BIN" "$code_file" >"$out_file" 2>&1
  rc=$?

  expected_rc=0
  if [[ -f "$status_file" ]]; then
    expected_rc=$(tr -d '[:space:]' < "$status_file")
  fi

  if [[ "$expected_rc" != "0" ]]; then
    # === ERROR-EXPECTED CASE ===
    # Pass if exit non-zero OR output contains 'error' (case-insensitive)
    if [[ "$rc" -ne 0 ]] || grep -qi 'error' "$out_file"; then
      echo "$id ... Passed (error correctly detected)"
      ((pass++))
    else
      echo "$id ... Failed (expected an error)"
      # show a snippet of actual output to help
      sed -n '1,10p' "$out_file"
      ((fail++))
    fi
  else
    # === NORMAL CASE (EXPECT SUCCESS) ===
    # Require whitespace-insensitive match
    if diff -q -w "$exp" "$out_file" >/dev/null; then
      echo "$id ... Passed!"
      ((pass++))
    else
      echo "$id ... Failed.  Diff (ignoring whitespace):"
      diff -u -w "$exp" "$out_file" | sed -n '1,60p'
      ((fail++))
    fi
  fi
done

total=$((pass + fail))
echo "Passed $pass of $total tests (Failed $fail)"
