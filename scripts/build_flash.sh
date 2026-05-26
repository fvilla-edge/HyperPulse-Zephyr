#!/usr/bin/env bash
# Build and flash the Hyperpulse demo (NCS west + sysbuild + pyocd).
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
MODULE_ROOT="${MODULE_ROOT:-$(cd "$SCRIPT_DIR/.." && pwd)}"
# v3.0.2/modules/lib/<this-repo> -> NCS install root
NCS_ROOT="${NCS_ROOT:-$(cd "$MODULE_ROOT/../../.." && pwd)}"
NCS_ENV="${NCS_ENV:-$HOME/ncs/env/ncs_3.0.2.sh}"
BOARD="${BOARD:-myriota_hyperpulse_dk/nrf9151/circuitdojo_ns}"
BUILD_DIR_REL="samples/demo/build"
BUILD_DIR="$MODULE_ROOT/$BUILD_DIR_REL"
APP_DIR="samples/demo"

usage() {
	cat <<EOF
Usage: $(basename "$0") <command> [options]

Commands:
  build       Run west build (sysbuild). Uses --pristine by default.
  flash       Flash merged.hex with pyocd (default target: nRF91).
  erase       Full chip erase via pyocd (erase --chip; default -t nRF91).
  all         build then flash.

Build options:
  --no-pristine   Incremental build (omit west --pristine).
  Env PRISTINE=0  Same as --no-pristine.

Environment overrides:
  MODULE_ROOT  (default: repo root, parent of scripts/)
  NCS_ROOT     (default: three levels up from MODULE_ROOT)
  NCS_ENV      (default: \$HOME/ncs/env/ncs_3.0.2.sh)
  BOARD        (default: $BOARD)
  PYOCD_TARGET (default: nRF91; some pyocd builds use nrf91 — override if needed)
EOF
}

require_file() {
	local f="$1"
	local msg="${2:-}"
	if [[ ! -f "$f" ]]; then
		echo "error: ${msg:-missing file}: $f" >&2
		exit 1
	fi
}

require_exec() {
	local f="$1"
	local msg="${2:-}"
	if [[ ! -x "$f" ]]; then
		echo "error: ${msg:-not executable}: $f" >&2
		exit 1
	fi
}

# pyocd must not inherit NCS toolchain PYTHONHOME/PYTHONPATH (breaks target/plugins).
run_pyocd() {
	local pyocd_bin="$1"
	shift
	(
		unset PYTHONHOME PYTHONPATH
		exec "$pyocd_bin" "$@"
	)
}

cmd_build() {
	local use_pristine=1
	if [[ "${PRISTINE:-1}" == "0" ]]; then
		use_pristine=0
	fi
	while [[ $# -gt 0 ]]; do
		case "$1" in
		--no-pristine)
			use_pristine=0
			shift
			;;
		-h | --help)
			usage
			exit 0
			;;
		*)
			echo "error: unknown build option: $1" >&2
			usage
			exit 1
			;;
		esac
	done

	require_file "$NCS_ENV" "NCS environment script not found (set NCS_ENV)"

	# Run west inside a subshell so NCS exports (PYTHONHOME/PYTHONPATH/LD_LIBRARY_PATH
	# from ncs_*.sh) do not leak into this process. Otherwise `make all` breaks pyocd
	# after build in the same shell, while `make flash` in a fresh terminal works.
	(
		set +u
		# shellcheck source=/dev/null
		source "$NCS_ENV"
		set -u

		if ! command -v west >/dev/null 2>&1; then
			echo "error: west not found in PATH after sourcing NCS_ENV" >&2
			exit 1
		fi

		cd "$MODULE_ROOT"

		west_args=(
			build
			--build-dir
			"$BUILD_DIR_REL"
			"$APP_DIR"
			--board
			"$BOARD"
			--sysbuild
		)
		if [[ "$use_pristine" -eq 1 ]]; then
			west_args+=(--pristine)
		fi

		echo "==> west ${west_args[*]}"
		west "${west_args[@]}"
	)
}

cmd_flash() {
	local pyocd="$NCS_ROOT/.venv/bin/pyocd"
	local target="${PYOCD_TARGET:-nRF91}"
	require_exec "$pyocd" "pyocd not found (expected NCS Python venv)"
	require_file "$BUILD_DIR/merged.hex" "Run build first; missing"

	cd "$BUILD_DIR"
	echo "==> $pyocd load --target $target merged.hex (cwd: $BUILD_DIR)"
	run_pyocd "$pyocd" load --target "$target" merged.hex
}

cmd_erase() {
	local pyocd="$NCS_ROOT/.venv/bin/pyocd"
	local target="${PYOCD_TARGET:-nRF91}"
	require_exec "$pyocd" "pyocd not found (expected NCS Python venv)"
	echo "==> $pyocd erase --chip -t $target"
	run_pyocd "$pyocd" erase --chip -t "$target"
}

cmd_all() {
	cmd_build "$@"
	cmd_flash
}

main() {
	local subcmd="${1:-}"
	if [[ -z "$subcmd" ]]; then
		usage
		exit 1
	fi
	shift

	case "$subcmd" in
	build) cmd_build "$@" ;;
	flash) cmd_flash ;;
	erase) cmd_erase ;;
	all) cmd_all "$@" ;;
	-h | --help)
		usage
		exit 0
		;;
	*)
		echo "error: unknown command: $subcmd" >&2
		usage
		exit 1
		;;
	esac
}

main "$@"
