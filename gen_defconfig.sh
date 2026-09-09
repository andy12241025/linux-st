#!/bin/bash
set -euo pipefail

usage() {
	cat <<'EOF'
Usage: gen_defconfig.sh [-m]

Regenerate linux-5.4.31/arch/arm/configs/stm32mp1_andy_defconfig from:
  multi_v7_defconfig + in-tree fragment*.config + repo fragment-*.config

  -m   run menuconfig before saving (optional tweaks)

Writes a minimal defconfig via savedefconfig. Does not build the kernel.
EOF
}

ROOT="$(cd "$(dirname "$0")" && pwd)"
KDIR="${ROOT}/linux-5.4.31"
DEFCONFIG="${KDIR}/arch/arm/configs/stm32mp1_andy_defconfig"
export KBUILD_OUTPUT="${ROOT}/build"
export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabihf-
export KCFLAGS="-march=armv7-a -mfpu=vfp"

RUN_MENUCONFIG=0

while getopts ":mh" opt; do
	case "${opt}" in
	m) RUN_MENUCONFIG=1 ;;
	h)
		usage
		exit 0
		;;
	\?)
		echo "gen_defconfig.sh: unknown option '-${OPTARG}'" >&2
		usage >&2
		exit 1
		;;
	esac
done

mkdir -p "${KBUILD_OUTPUT}"
cd "${KDIR}"

echo "==> multi_v7_defconfig + in-tree fragment*.config"
make multi_v7_defconfig "fragment*.config"

echo "==> merge repo fragment-*.config"
for fragment in "${ROOT}"/fragment-*.config; do
	[ -f "${fragment}" ] || continue
	echo "    ${fragment}"
	scripts/kconfig/merge_config.sh -m -r \
		-O "${KBUILD_OUTPUT}" "${KBUILD_OUTPUT}/.config" "${fragment}"
done

echo "==> olddefconfig"
make olddefconfig

if [ "${RUN_MENUCONFIG}" = 1 ]; then
	echo "==> menuconfig"
	make menuconfig
fi

echo "==> savedefconfig"
make savedefconfig

cp "${KBUILD_OUTPUT}/defconfig" "${DEFCONFIG}"
echo "Wrote ${DEFCONFIG} ($(wc -l < "${DEFCONFIG}") lines)"
