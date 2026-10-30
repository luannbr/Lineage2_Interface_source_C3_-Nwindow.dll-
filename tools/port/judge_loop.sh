#!/usr/bin/env bash
# The generate -> compile -> judge -> demote loop of the C3 port (docs/PLANO_C3.md, "Etapa 3/4").
# Each round: gen.py (everything but db/hand_owned.txt), build, then the first judge that fails demotes what it
# can (db/demote.csv) and the next round starts again; it ends when every judge passes.
#
#   bash tools/port/judge_loop.sh [rounds]        (from the repository root; log lines on stdout)
#
# A failure in a hand-owned file cannot be demoted: the loop prints it and keeps going until `rounds` run out,
# so read the log, fix the file, and start again.
cd "$(dirname "$0")/../.." || exit 1
mkdir -p build
for i in $(seq 1 "${1:-16}"); do
  python tools/port/gen.py --cat-c --derived > build/judge_gen.log 2>&1 || { echo "round $i: gen FAILED"; tail -3 build/judge_gen.log; break; }
  u=$(grep "^units" build/judge_gen.log | cut -c1-90)
  cmake --build build --config Release --target NWindowDecomp --parallel 8 > build/judge_cc_$i.log 2>&1
  if grep -q "error C" build/judge_cc_$i.log; then
    echo "round $i: $u | compile: $(grep -c 'error C' build/judge_cc_$i.log) error lines"
    python tools/port/demote.py build/judge_cc_$i.log --stage . 2>&1 | head -1
    continue
  fi
  python tools/cycle.py --game-source --no-install > build/judge_cy_$i.log 2>&1; rc=$?
  h=$(grep "hooks:" build/judge_cy_$i.log | tail -1 | cut -c14-60)
  if [ $rc -ne 0 ]; then
    v=$(grep -E "[0-9]+ ok, [0-9]+ explained" build/judge_cy_$i.log | tail -1)
    if echo "$v" | grep -q " 0 unexplained"; then
      echo "round $i: $u | $h | regcheck -> demote"; python tools/regcheck.py --game-src --demote 2>&1 | tail -1
    else
      echo "round $i: $u | $h | $v -> demote_verify"; python tools/port/demote_verify.py build/judge_cy_$i.log 2>&1 | tail -1
    fi
    continue
  fi
  d=$(python tools/port/verify_data.py 2>&1 | tail -1)
  python tools/port/check_breakers.py > build/judge_br_$i.log 2>&1; brc=$?
  echo "round $i: $h | cycle ok | $d | breakers rc=$brc"
  dok=0; echo "$d" | grep -q " 0 differ" && dok=1
  if [ $dok -eq 1 ] && [ $brc -eq 0 ]; then
    echo "ALL JUDGES PASS"
    # what no judge above sees (the first run in the game found both kinds): the arguments a stub declares against
    # the stock ret, and a void hook whose stock callers read eax
    python tools/port/check_stubs.py | head -1
    python tools/port/check_retval.py | tail -1
    break
  fi
  [ $dok -eq 0 ] && python tools/port/demote_data.py 2>&1 | tail -1
  [ $brc -ne 0 ] && python tools/port/check_breakers.py --demote 2>&1 | tail -1
done
