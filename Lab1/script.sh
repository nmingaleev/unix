#!/bin/bash -e

function error() {
  BIGWHITETEXT="\033[1;37m"
  BGRED='\033[41m'
  NORMAL="\033[0m"
  echo ""
  printf "${BIGWHITETEXT}${BGRED} %s ${NORMAL}" $1
  echo ""
  exit 1
}

function handle_SIGNALS() {
  rm -rf -- $mktemp_name
  error "User kill process. Temporary folder was removed."
}

file_name="$1"

[ -z "$file_name" ] && error 'First argument must be a file name'

[ ! -e "$file_name" ] && error 'File does not exist'

[ ! -r "$file_name" ] && error 'File can not be read'

OUTPUT_NAME_REGEX="s/^[[:space:]]*\/\/[[:space:]]*Output[[:space:]]*\([^ ]*\)$/\1/p"

executable_file_name=$(sed -n -e "$OUTPUT_NAME_REGEX" "$file_name" | grep -m 1 "")

[ -z "$executable_file_name" ] && error 'Output name is not found'

mktemp_name=$(mktemp -d -t kekXXX) || error 'Failed to create temp folder'

trap handle_SIGNALS HUP INT QUIT PIPE TERM

cp "$file_name" $mktemp_name || error 'Failed to copy file.'

current_path=$(pwd)

cd "$mktemp_name"

g++ -o "$executable_file_name" "$file_name" || error "Failed compiling src file."

cp "$executable_file_name" "$current_path" || error "Failed to copy file"

rm -rf -- "$mktemp_name"
