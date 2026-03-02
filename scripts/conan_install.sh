#!/usr/bin/env bash

usage_message="Usage: conan_install <directory>"

if [[ $# -ne 1 ]]; then
	printf "Error: expected 1 argument\n" >&2
	printf "%s\n" "${usage_message}" >&2
	exit 1
fi

if [[ ! -d "$1" ]]; then
	printf "Error: '%s' is not a directory\n" "$1" >&2
	exit 1
fi

src_dir=$PWD
build_dir=$1

function call_conan_install() {
	cd "${src_dir}" || return
	cd "$1" || return

	rm -f ./*.cmake
	rm -f CMakePresets.json
	rm -f conan*.sh
	rm -f deactivate_conan*.sh
	conan install "${src_dir}" --output-folder=. --profile:build "$2" --profile:host "$3" --build=missing
}

# shellcheck source=/dev/null
source .venv/bin/activate

call_conan_install "${build_dir}/gcc-debug" gcc-release gcc-debug
call_conan_install "${build_dir}/gcc-debug-san" gcc-release gcc-debug
call_conan_install "${build_dir}/gcc-release" gcc-release gcc-release
call_conan_install "${build_dir}/gcc-reldeb" gcc-release gcc-reldeb

call_conan_install "${build_dir}/clang-debug" clang-release clang-debug
call_conan_install "${build_dir}/clang-debug-libstd" clang-release clang-debug-libstd
call_conan_install "${build_dir}/clang-debug-san" clang-release clang-debug
call_conan_install "${build_dir}/clang-release" clang-release clang-release
call_conan_install "${build_dir}/clang-reldeb" clang-release clang-reldeb

