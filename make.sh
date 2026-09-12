#!/bin/bash
set -euo pipefail

usage() {
	cat <<'EOF'
Usage: make.sh [-r] [-c] [-m] [-s] [-b] [-d <board>]

  -r   cleanup only (make distclean)
  -c   configure only (stm32_andy_defconfig + olddefconfig, covering initial config)
  -m   run menuconfig (after configure if -c is also set)
  -s   save defconfig to stm32_andy_defconfig (after menuconfig if -m is also set)
  -b   build only (uImage + dtbs)
  -d <board>  compile arch/arm/boot/dts/<board>.dtb only
  -h   show this help message

With no options, runs cleanup, configure, and build (-r -c -b).
Use -m to open menuconfig; it is not run by default.
EOF
}

ROOT="$(cd "$(dirname "$0")" && pwd)"
SDIR="${ROOT}/linux-5.4.31"
DEFCONFIG="${SDIR}/arch/arm/configs/stm32_andy_defconfig"
export KBUILD_OUTPUT="${ROOT}/build"
export ARCH=arm
export CROSS_COMPILE=arm-none-linux-gnueabihf-

DO_CLEANUP=0
DO_CONFIG=0
DO_MENUCONFIG=0
DO_BUILD=0
DO_SAVE=0
DO_DTC=0
DTC_BOARD=""

if [ $# -eq 0 ]; then
	DO_CLEANUP=1
	DO_CONFIG=1
	DO_BUILD=1
else
	while getopts ":rcmsbd:h" opt; do
		case "${opt}" in
		r) DO_CLEANUP=1 ;;
		c) DO_CONFIG=1 ;;
		m) DO_MENUCONFIG=1 ;;
		b) DO_BUILD=1 ;;
		s) DO_SAVE=1 ;;
		d)
			DO_DTC=1
			DTC_BOARD="${OPTARG%.dtb}"
			;;
		h)
			usage
			exit 0
			;;
		\?)
			echo "make.sh: unknown option '-${OPTARG}'" >&2
			usage >&2
			exit 1
			;;
		:)
			echo "make.sh: option '-${OPTARG}' requires an argument" >&2
			usage >&2
			exit 1
			;;
		esac
	done
	shift $((OPTIND - 1))
	if [ $# -ne 0 ]; then
		echo "make.sh: unexpected argument '$1'" >&2
		usage >&2
		exit 1
	fi
fi

mkdir -p "${KBUILD_OUTPUT}"
cd "${SDIR}"

if [ "${DO_CLEANUP}" = 1 ]; then
	echo "==> distclean"
	make distclean
fi

if [ "${DO_CONFIG}" = 1 ]; then
	if [ ! -f "${DEFCONFIG}" ]; then
		echo "==> stm32_andy_defconfig missing; generating it"
		"${ROOT}/gen_defconfig.sh"
	fi
	echo "==> stm32_andy_defconfig"
	make stm32_andy_defconfig
	make olddefconfig
fi

if [ "${DO_MENUCONFIG}" = 1 ]; then
	if [ ! -f "${KBUILD_OUTPUT}/.config" ]; then
		echo "make.sh: no .config found; run with -c first or use full build" >&2
		exit 1
	fi
	echo "==> menuconfig"
	make menuconfig
fi

if [ "${DO_SAVE}" = 1 ]; then
	echo "==> savedefconfig"
	make savedefconfig
	
	cp "${KBUILD_OUTPUT}/defconfig" "${DEFCONFIG}"
	echo "Wrote ${DEFCONFIG} ($(wc -l < "${DEFCONFIG}") lines)"
fi

if [ "${DO_BUILD}" = 1 ]; then
	if [ ! -f "${KBUILD_OUTPUT}/.config" ]; then
		echo "make.sh: no .config found; run with -c first or use full build" >&2
		exit 1
	fi
	echo "==> uImage dtbs"
	make uImage dtbs LOADADDR=0xC2000040 -j"$(nproc)"
fi

if [ "${DO_DTC}" = 1 ]; then
	if [ ! -f "${KBUILD_OUTPUT}/.config" ]; then
		echo "make.sh: no .config found; run with -c first or use full build" >&2
		exit 1
	fi
	if [ ! -f "${SDIR}/arch/arm/boot/dts/${DTC_BOARD}.dts" ]; then
		echo "make.sh: unknown DTB board '${DTC_BOARD}'" >&2
		exit 1
	fi
	echo "==> ${DTC_BOARD}.dtb"
	make "${DTC_BOARD}.dtb" -j"$(nproc)"
fi
