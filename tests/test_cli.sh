#!/bin/sh
# Tests for the example command-line tool. Usage: tests/test_cli.sh ./typeid
set -u
bin=$1
fail=0
check() { if [ "$2" != "$3" ]; then echo "FAIL $1: got '$2', want '$3'"; fail=1; fi; }

check encode "$($bin encode prefix 01890a5d-ac96-774b-bcce-b302099a8057)" prefix_01h455vb4pex5vsknk084sn02q
check decode "$($bin decode prefix_01h455vb4pex5vsknk084sn02q | sed -n 2p)" "uuid: 01890a5d-ac96-774b-bcce-b302099a8057"
check count "$($bin new user 1000 | sort -u | wc -l | tr -d ' ')" 1000
check sorted "$($bin new user 1000 | sort -c 2>&1 && echo ok)" ok
check no-prefix "$($bin new | tr -d '\n' | wc -c | tr -d ' ')" 26
$bin new user abc >/dev/null 2>&1; check bad-count "$?" 2
$bin new User >/dev/null 2>&1; check bad-prefix "$?" 1
$bin decode bad >/dev/null 2>&1; check bad-decode "$?" 1
$bin >/dev/null 2>&1; check usage "$?" 2
if [ -w /dev/full ]; then
    $bin new user 10 >/dev/full 2>/dev/null; check write-error "$?" 1
fi

[ $fail -eq 0 ] && echo "cli: all checks passed"
exit $fail
