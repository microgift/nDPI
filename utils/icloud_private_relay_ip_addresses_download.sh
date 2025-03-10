#!/usr/bin/env bash

set -e

cd "$(dirname "${0}")" || exit 1
. ./common.sh || exit 1

DEST=../src/lib/inc_generated/ndpi_icloud_private_relay_match.c.inc
TMP=/tmp/icloud.csv
LIST=/tmp/icloud.list
LIST_MERGED=/tmp/icloud.list_m
ORIGIN="https://mask-api.icloud.com/egress-ip-ranges.csv"


echo "(1) Downloading file... ${ORIGIN}"
http_response=$(curl -s -o "$TMP" -w "%{http_code}" ${ORIGIN})
check_http_response "${http_response}"
is_file_empty "${TMP}"

echo "(2) Processing IP addresses..."

# Note: the "grep -v :" is used to skip IPv6 addresses
cut -d ',' -f 1 $TMP | grep -v ':' > $LIST
is_file_empty "${LIST}"
./mergeipaddrlist.py $LIST > $LIST_MERGED
is_file_empty "${LIST_MERGED}"
./ipaddr2list.py $LIST_MERGED NDPI_ANONYMOUS_SUBSCRIBER "_icloud_private_relay" > $DEST
is_file_empty "${DEST}"
rm -f "${TMP}" "${LIST}" "${LIST_MERGED}"

echo "(3) iCloud Private Relay IPs are available in $DEST"
exit 0
