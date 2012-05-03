# not tested:

# multilog handles TERM
# multilog handles ALRM
# multilog handles out-of-memory
# multilog handles log directories
# multilog matches only first 1000 characters of long lines
# multilog t produces the right time
# multilog closes descriptors properly

echo '--- multilog prints nothing with no actions'
( echo one; echo two ) | multilog; echo $?

echo '--- multilog e prints to stderr'
( echo one; echo two ) | multilog e 2>&1; echo $?

echo '--- multilog inserts newline after partial final line'
( echo one; echo two | tr -d '\012' ) | multilog e 2>&1; echo $?

echo '--- multilog handles multiple actions'
( echo one; echo two ) | multilog e e 2>&1; echo $?

echo '--- multilog handles wildcard -'
( echo one; echo two ) | multilog '-*' e 2>&1; echo $?

echo '--- multilog handles literal +'
( echo one; echo two ) | multilog '-*' '+one' e 2>&1; echo $?

echo '--- multilog handles long lines for stderr'
echo 0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678 \
| multilog e 2>&1; echo $?
echo 01234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789 \
| multilog e 2>&1; echo $?
echo 012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890 \
| multilog e 2>&1; echo $?

echo '--- multilog handles status files'
rm -f test.status
( echo one; echo two ) | multilog =test.status; echo $?
uniq -c < test.status | sed 's/[ 	]*[ 	]/_/g'

echo '--- multilog t has the right format'
( echo ONE; echo TWO ) | multilog t e 2>&1 | sed 's/[0-9a-f]/x/g'

echo '--- multilog T has the right format'
# This test does not work only between
# <Sun Sep 9 10:46:40 2001> and <Sun Nov 21 02:46:39 2286>.
# (time(3): 1000000000 and 9999999999)
( echo ONE; echo TWO ) | multilog T e 2>&1 | sed 's/^[0-9]\{10\}\.[0-9]\{6\}/xxxxxxxxxx.xxxxxx/g'

echo '--- multilog h has the right format'
( echo ONE; echo TWO ) | multilog h e 2>&1 | sed 's/^[0-9]\{8\}T[0-9]\{6\}\.[0-9]\{6\}/YYYYMMDDThhmmss.SSSSSS/g'

