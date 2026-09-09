#!/bin/bash
set -euo pipefail

usage() {
	cat <<'EOF'
Usage: build.sh [-r] [-c] [-m] [-b]

  -r   cleanup only (make distclean)
  -c   configure only (stm32mp1_andy_defconfig + olddefconfig)
  -m   run menuconfig (after configure if -c is also set)
  -b   build only (uImage + dtbs)

With no options, runs cleanup, configure, and build (-r -c -b).
Use -m to open menuconfig; it is not run by default.
EOF
}

ROOT="$(cd "$(dirname "$0")" && pwd)"
export KBUILD_OUTPUT="${ROOT}/build"
export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabihf-
# Ubuntu's gnueabihf gcc rejects -march=armv7-a without -mfpu=vfp during
# kernel's cc-option probe, so force the correct architecture flags.
export KCFLAGS="-march=armv7-a -mfpu=vfp"

DO_CLEANUP=0
DO_CONFIG=0
DO_MENUCONFIG=0
DO_BUILD=0

if [ $# -eq 0 ]; then
	DO_CLEANUP=1
	DO_CONFIG=1
	DO_BUILD=1
else
	while getopts ":rcmbh" opt; do
		case "${opt}" in
		r) DO_CLEANUP=1 ;;
		c) DO_CONFIG=1 ;;
		m) DO_MENUCONFIG=1 ;;
		b) DO_BUILD=1 ;;
		h)
			usage
			exit 0
			;;
		\?)
			echo "build.sh: unknown option '-${OPTARG}'" >&2
			usage >&2
			exit 1
			;;
		:)
			echo "build.sh: option '-${OPTARG}' requires an argument" >&2
			usage >&2
			exit 1
			;;
		esac
	done
fi

mkdir -p "${KBUILD_OUTPUT}"
cd "${ROOT}/linux-5.4.31"

if [ "${DO_CLEANUP}" = 1 ]; then
	echo "==> distclean"
	make distclean
fi

if [ "${DO_CONFIG}" = 1 ]; then
	echo "==> stm32mp1_andy_defconfig"
	make stm32mp1_andy_defconfig
	make olddefconfig
fi

if [ "${DO_MENUCONFIG}" = 1 ]; then
	if [ ! -f "${KBUILD_OUTPUT}/.config" ]; then
		echo "build.sh: no .config found; run with -c first or use full build" >&2
		exit 1
	fi
	echo "==> menuconfig"
	make menuconfig
fi

if [ "${DO_BUILD}" = 1 ]; then
	if [ ! -f "${KBUILD_OUTPUT}/.config" ]; then
		echo "build.sh: no .config found; run with -c first or use full build" >&2
		exit 1
	fi
	echo "==> uImage dtbs"
	make uImage dtbs LOADADDR=0xC2000040 -j"$(nproc)"
fi
