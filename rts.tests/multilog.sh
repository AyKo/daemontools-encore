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

echo '--- multilog output file name'
/bin/rm -rf multilog_output_default multilog_output_fT multilog_output_fh multilog_output_ft
( for v in `seq 1 500`; do
echo 0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678
done ) > multilog_test.txt
( for v in `seq 1 11`; do
cat multilog_test.txt
done ) > multilog_test_11.txt
cat multilog_test_11.txt | multilog ./multilog_output_default fT ./multilog_output_fT fh ./multilog_output_fh ft ./multilog_output_ft
[ `/bin/ls multilog_output_default/*.s | /bin/grep -E '^multilog_output_default\/@[0-9a-f]{24}\.s$' | wc -l` -eq 9 ]          && echo default_ok
[ `/bin/ls multilog_output_ft/*.s      | /bin/grep -E '^multilog_output_ft\/@[0-9a-f]{24}\.s$' | wc -l` -eq 9 ]               && echo ft_ok
[ `/bin/ls multilog_output_fT/*.s      | /bin/grep -E '^multilog_output_fT\/[0-9]{10}\.[0-9]{6}\.s$' | wc -l` -eq 9 ]         && echo fT_ok
[ `/bin/ls multilog_output_fh/*.s      | /bin/grep -E '^multilog_output_fh\/[0-9]{8}T[0-9]{6}\.[0-9]{6}\.s$' | wc -l` -eq 9 ] && echo fh_ok

echo '--- multilog flag-value of timing to create a log file'
cat multilog_test.txt | multilog ftR ./multilog_output_timing_ftR ftr ./multilog_output_timing_ftr
[ `/bin/ls multilog_output_timing_ftR/*.s | /bin/grep -E '^multilog_output_timing_ftR\/@[0-9a-f]{24}\.s$' | wc -l` -eq 2 ] && echo ftR_num_ok
TARGET_INODE=`ls -i multilog_output_timing_ftR/*.s | tail -1 | cut -d ' ' -f 1`
CURRENT_INODE=`ls -i multilog_output_timing_ftR/current | cut -d ' ' -f 1`
[ $TARGET_INODE -eq $CURRENT_INODE ] && echo ftR_inode_ok
( cat multilog_output_timing_ftR/*.s | diff multilog_test.txt - ) && echo ftR_data_ok

[ `/bin/ls multilog_output_timing_ftr/*.s | /bin/grep -E '^multilog_output_timing_ftr\/@[0-9a-f]{24}\.s$' | wc -l` -eq 1 ] && echo ftR_num_ok
TARGET_INODE=`ls -i multilog_output_timing_ftr/*.s | tail -1 | cut -d ' ' -f 1`
CURRENT_INODE=`ls -i multilog_output_timing_ftr/current | cut -d ' ' -f 1`
[ $TARGET_INODE -ne $CURRENT_INODE ] && echo ftr_inode_ok
( cat multilog_output_timing_ftr/*.s multilog_output_timing_ftr/current | diff multilog_test.txt - ) && echo ftr_data_ok

( cat multilog_test.txt ; sleep 3 ) | multilog ftR ./multilog_output_timing_ftR ftr ./multilog_output_timing_ftr &
MULTILOG_PID=$!
sleep 1
exec 2>/dev/null
kill -KILL $MULTILOG_PID
echo a | multilog ftR ./multilog_output_timing_ftR ftr ./multilog_output_timing_ftr
diff ./multilog_output_timing_ftR/*.u ./multilog_output_timing_ftr/*.u  && echo ftR_ftr_dot_u_diff_ok
sleep 3
exec 2>&1

echo '--- multilog processor ---'
( echo '#!/bin/sh'; echo 'tee multilog_processor_test_out.txt' ) > multilog_processor.sh
chmod +x multilog_processor.sh
cat multilog_test.txt | multilog !${PWD}/multilog_processor.sh ./multilog_processor_test
diff multilog_processor_test/multilog_processor_test_out.txt `ls multilog_processor_test/@*` && echo processor_test_ok

echo '--- multilog prefix/postfix ---'
cat multilog_test_11.txt | multilog \
  Pprefix_ ./xxfix-1 \
  p_postfix ./xxfix-2 \
  Pprefix_ p_postfix ./xxfix-3 \
  Pprefix_ p_postfix ftr ./xxfix-tai64n-last \
  Pprefix_ p_postfix ftR ./xxfix-tai64n-first \
  Pprefix_ p_postfix fTr ./xxfix-accustamp-last \
  Pprefix_ p_postfix fTR ./xxfix-accustamp-first \
  Pprefix_ p_postfix fhr ./xxfix-humanreadable-last \
  Pprefix_ p_postfix fhR ./xxfix-humanreadable-first
[ `/bin/ls xxfix-1/*.s | /bin/grep -E '^xxfix-1/prefix_\@[0-9a-f]{24}\.s$'         | wc -l` -eq 9 ] && echo xxfix-1_ok
[ `/bin/ls xxfix-2/*.s | /bin/grep -E '^xxfix-2/\@[0-9a-f]{24}_postfix\.s$'        | wc -l` -eq 9 ] && echo xxfix-2_ok
[ `/bin/ls xxfix-3/*.s | /bin/grep -E '^xxfix-3/prefix_\@[0-9a-f]{24}_postfix\.s$' | wc -l` -eq 9 ] && echo xxfix-3_ok
[ `/bin/ls xxfix-tai64n-last/*.s  | /bin/grep -E '^xxfix-tai64n-last/prefix_\@[0-9a-f]{24}_postfix\.s$'  | wc -l` -eq 9 ] && echo xxfix-tai64n-last_ok
[ `/bin/ls xxfix-tai64n-first/*.s | /bin/grep -E '^xxfix-tai64n-first/prefix_\@[0-9a-f]{24}_postfix\.s$' | wc -l` -eq 9 ] && echo xxfix-tai64n-first_ok
[ `/bin/ls xxfix-accustamp-last/*.s  | /bin/grep -E '^xxfix-accustamp-last/prefix_[0-9]{10}\.[0-9]{6}_postfix\.s$'  | wc -l` -eq 9 ] && echo xxfix-accustamp-last_ok
[ `/bin/ls xxfix-accustamp-first/*.s | /bin/grep -E '^xxfix-accustamp-first/prefix_[0-9]{10}\.[0-9]{6}_postfix\.s$' | wc -l` -eq 9 ] && echo xxfix-accustamp-first_ok
[ `/bin/ls xxfix-humanreadable-last/*.s  | /bin/grep -E '^xxfix-humanreadable-last/prefix_[0-9]{8}T[0-9]{6}\.[0-9]{6}_postfix\.s$'  | wc -l` -eq 9 ] && echo xxfix-humanreadable-last_ok
[ `/bin/ls xxfix-humanreadable-first/*.s | /bin/grep -E '^xxfix-humanreadable-first/prefix_[0-9]{8}T[0-9]{6}\.[0-9]{6}_postfix\.s$' | wc -l` -eq 9 ] && echo xxfix-humanreadable-first_ok

echo '--- multilog "safely written" code'
cat multilog_test_11.txt | multilog cOK ./safely_written
[ `/bin/ls safely_written/ | /bin/grep -E '^\@[0-9a-f]{24}\.OK$' | wc -l` -eq 9 ] && echo safely written code ok

echo '--- multilog "unsafely written" code'
( cat multilog_test.txt ; sleep 1 ) | multilog cOK CNG ./unsafely_written &
MULTILOG_PID=$!
sleep 0.3
exec 2>/dev/null
kill -KILL $MULTILOG_PID
sleep 1
exec 2>&1
echo a | multilog cOK CNG ./unsafely_written 
[ `/bin/ls unsafely_written/*.NG | /bin/grep -E '^unsafely_written/\@[0-9a-f]{24}\.NG$' | wc -l` -eq 1 ] && echo unsafely written code rename ok

( cat multilog_test_11.txt ; sleep 1 ) | multilog cOK CNG fR ./unsafely_written_2 &
MULTILOG_PID=$!
sleep 0.3
exec 2>/dev/null
kill -KILL $MULTILOG_PID
sleep 1
exec 2>&1
INDOE1=`/bin/ls -i unsafely_written_2/*.OK | tail -1 | cut -d ' ' -f 1`
INDOE2=`/bin/ls -i unsafely_written_2/current | cut -d ' ' -f 1`
[ $INDOE1 -eq $INDOE2 ] && echo unsafely written code inode ok
echo a | multilog cOK CNG fR ./unsafely_written_2 
[ `/bin/ls unsafely_written_2/*.NG | /bin/grep -E '^unsafely_written_2/\@[0-9a-f]{24}\.NG$' | wc -l` -eq 1 ] && echo unsafely written code with fR rename ok with fR
INDOE2=`/bin/ls -i unsafely_written_2/*.NG | cut -d ' ' -f 1`
[ $INDOE1 -eq $INDOE2 ] && echo unsafely written code with fR inode ok

