#!/bin/sh
# Installs Ubuntu packages on a GitHub runner without depending on its Azure apt mirror.
# The runners fetch from azure.archive.ubuntu.com through /etc/apt/apt-mirrors.txt; when that
# name stops resolving, every Linux job fails before it builds anything. This points apt at
# Ubuntu's own archive instead, lets apt retry and time out, and tries the whole install three
# times. It installs exactly the packages it is given.
# usage: sh .github/apt-install.sh package...
set -eu
for f in /etc/apt/sources.list /etc/apt/sources.list.d/*.sources /etc/apt/sources.list.d/*.list; do
    [ -f "$f" ] || continue
    sudo sed -i -E \
        -e 's#mirror\+file:/etc/apt/apt-mirrors\.txt#http://archive.ubuntu.com/ubuntu/#g' \
        -e 's#https?://azure\.archive\.ubuntu\.com/ubuntu/?#http://archive.ubuntu.com/ubuntu/#g' \
        "$f"
done
opts="-o Acquire::Retries=5 -o Acquire::http::Timeout=30 -o Acquire::https::Timeout=30"
attempt=1
while :; do
    # shellcheck disable=SC2086
    if sudo apt-get $opts update -q && sudo apt-get $opts install -y -q "$@"; then exit 0; fi
    [ "$attempt" -ge 3 ] && break
    echo "apt failed (attempt $attempt of 3); trying again in $((attempt * 20)) s"
    sleep $((attempt * 20))
    attempt=$((attempt + 1))
done
echo "apt could not install: $*"
exit 1
