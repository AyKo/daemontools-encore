# not tested:

# multilog-mod handles TERM
# multilog-mod handles ALRM
# multilog-mod handles out-of-memory
# multilog-mod handles log directories
# multilog-mod matches only first 1000 characters of long lines
# multilog-mod t produces the right time
# multilog-mod closes descriptors properly

echo '--- multilog-mod prints nothing with no actions'
( echo one; echo two ) | multilog-mod; echo $?

echo '--- multilog-mod e prints to stderr'
( echo one; echo two ) | multilog-mod e 2>&1; echo $?

echo '--- multilog-mod inserts newline after partial final line'
( echo one; echo two | tr -d '\012' ) | multilog-mod e 2>&1; echo $?

echo '--- multilog-mod handles multiple actions'
( echo one; echo two ) | multilog-mod e e 2>&1; echo $?

echo '--- multilog-mod handles wildcard -'
( echo one; echo two ) | multilog-mod '-*' e 2>&1; echo $?

echo '--- multilog-mod handles literal +'
( echo one; echo two ) | multilog-mod '-*' '+one' e 2>&1; echo $?

echo '--- multilog-mod handles long lines for stderr'
echo 0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678 \
| multilog-mod e 2>&1; echo $?
echo 01234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789 \
| multilog-mod e 2>&1; echo $?
echo 012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890 \
| multilog-mod e 2>&1; echo $?

echo '--- multilog-mod handles status files'
rm -f test.status
( echo one; echo two ) | multilog-mod =test.status; echo $?
uniq -c < test.status | sed 's/[ 	]*[ 	]/_/g'

echo '--- multilog-mod t has the right format'
( echo ONE; echo TWO ) | multilog-mod t e 2>&1 | sed 's/[0-9a-f]/x/g'

echo '--- multilog-mod T has the right format'
# This test does not work only between
# <Sun Sep 9 10:46:40 2001> and <Sun Nov 21 02:46:39 2286>.
# (time(3): 1000000000 and 9999999999)
( echo ONE; echo TWO ) | multilog-mod T e 2>&1 | sed 's/^[0-9]\{10\}\.[0-9]\{6\}/xxxxxxxxxx.xxxxxx/g'

echo '--- multilog-mod h has the right format'
( echo ONE; echo TWO ) | multilog-mod h e 2>&1 | sed 's/^[0-9]\{8\}T[0-9]\{6\}\.[0-9]\{6\}/YYYYMMDDThhmmss.SSSSSS/g'

echo '--- multilog-mod H has the right format'
( echo ONE; echo TWO ) | multilog-mod H e 2>&1 | sed 's/^[0-9]\{15\}/YYMMDDhhmmssSSS/g'

echo '--- multilog-mod output file name'
/bin/rm -rf multilog_output_default multilog_output_fT multilog_output_fh multilog_output_ft multilog_output_fH
( for v in `seq 1 500`; do
echo 0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678
done ) > multilog_test.txt
( for v in `seq 1 11`; do
cat multilog_test.txt
done ) > multilog_test_11.txt
cat multilog_test_11.txt | multilog-mod ./multilog_output_default fT ./multilog_output_fT fh ./multilog_output_fh ft ./multilog_output_ft fH ./multilog_output_fH
[ `/bin/ls multilog_output_default/*.s | /bin/grep -E '^multilog_output_default\/@[0-9a-f]{24}\.s$' | wc -l` -eq 9 ]          && echo default_ok
[ `/bin/ls multilog_output_ft/*.s      | /bin/grep -E '^multilog_output_ft\/@[0-9a-f]{24}\.s$' | wc -l` -eq 9 ]               && echo ft_ok
[ `/bin/ls multilog_output_fT/*.s      | /bin/grep -E '^multilog_output_fT\/[0-9]{10}\.[0-9]{6}\.s$' | wc -l` -eq 9 ]         && echo fT_ok
[ `/bin/ls multilog_output_fh/*.s      | /bin/grep -E '^multilog_output_fh\/[0-9]{8}T[0-9]{6}\.[0-9]{6}\.s$' | wc -l` -eq 9 ] && echo fh_ok
[ `/bin/ls multilog_output_fH/*.s      | /bin/grep -E '^multilog_output_fH\/[0-9]{15}\.s$' | wc -l` -eq 9 ]                   && echo fH_ok

echo '--- multilog-mod flag-value of timing to create a log file'
cat multilog_test.txt | multilog-mod ftR ./multilog_output_timing_ftR ftr ./multilog_output_timing_ftr
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

( cat multilog_test.txt ; sleep 3 ) | multilog-mod ftR ./multilog_output_timing_ftR ftr ./multilog_output_timing_ftr &
MULTILOG_PID=$!
sleep 1
exec 2>/dev/null
kill -KILL $MULTILOG_PID
while ( kill -0 $MULTILOG_PID 2> /dev/null) ; do sleep 0.1 ; done
echo a | multilog-mod ftR ./multilog_output_timing_ftR ftr ./multilog_output_timing_ftr
diff ./multilog_output_timing_ftR/*.u ./multilog_output_timing_ftr/*.u  && echo ftR_ftr_dot_u_diff_ok
sleep 3
exec 2>&1

echo '--- multilog-mod processor ---'
( echo '#!/bin/sh'; echo 'tee multilog_processor_test_out.txt' ) > multilog_processor.sh
chmod +x multilog_processor.sh
cat multilog_test.txt | multilog-mod !${PWD}/multilog_processor.sh ./multilog_processor_test
diff multilog_processor_test/multilog_processor_test_out.txt `ls multilog_processor_test/@*` && echo processor_test_ok

echo '--- multilog-mod prefix/postfix ---'
cat multilog_test_11.txt | multilog-mod \
  Pprefix_ ./xxfix-1 \
  p_postfix ./xxfix-2 \
  Pprefix_ p_postfix ./xxfix-3 \
  Pprefix_ p_postfix ftr ./xxfix-tai64n-last \
  Pprefix_ p_postfix ftR ./xxfix-tai64n-first \
  Pprefix_ p_postfix fTr ./xxfix-accustamp-last \
  Pprefix_ p_postfix fTR ./xxfix-accustamp-first \
  Pprefix_ p_postfix fhr ./xxfix-humanreadable-last \
  Pprefix_ p_postfix fhR ./xxfix-humanreadable-first \
  Pprefix_ p_postfix fHr ./xxfix-humanreadable2-last \
  Pprefix_ p_postfix fHR ./xxfix-humanreadable2-first
[ `/bin/ls xxfix-1/*.s | /bin/grep -E '^xxfix-1/prefix_\@[0-9a-f]{24}\.s$'         | wc -l` -eq 9 ] && echo xxfix-1_ok
[ `/bin/ls xxfix-2/*.s | /bin/grep -E '^xxfix-2/\@[0-9a-f]{24}_postfix\.s$'        | wc -l` -eq 9 ] && echo xxfix-2_ok
[ `/bin/ls xxfix-3/*.s | /bin/grep -E '^xxfix-3/prefix_\@[0-9a-f]{24}_postfix\.s$' | wc -l` -eq 9 ] && echo xxfix-3_ok
[ `/bin/ls xxfix-tai64n-last/*.s  | /bin/grep -E '^xxfix-tai64n-last/prefix_\@[0-9a-f]{24}_postfix\.s$'  | wc -l` -eq 9 ] && echo xxfix-tai64n-last_ok
[ `/bin/ls xxfix-tai64n-first/*.s | /bin/grep -E '^xxfix-tai64n-first/prefix_\@[0-9a-f]{24}_postfix\.s$' | wc -l` -eq 9 ] && echo xxfix-tai64n-first_ok
[ `/bin/ls xxfix-accustamp-last/*.s  | /bin/grep -E '^xxfix-accustamp-last/prefix_[0-9]{10}\.[0-9]{6}_postfix\.s$'  | wc -l` -eq 9 ] && echo xxfix-accustamp-last_ok
[ `/bin/ls xxfix-accustamp-first/*.s | /bin/grep -E '^xxfix-accustamp-first/prefix_[0-9]{10}\.[0-9]{6}_postfix\.s$' | wc -l` -eq 9 ] && echo xxfix-accustamp-first_ok
[ `/bin/ls xxfix-humanreadable-last/*.s | /bin/grep -E '^xxfix-humanreadable-last/prefix_[0-9]{8}T[0-9]{6}\.[0-9]{6}_postfix\.s$' | wc -l` -eq 9 ] && echo xxfix-humanreadable-last_ok
[ `/bin/ls xxfix-humanreadable-first/*.s | /bin/grep -E '^xxfix-humanreadable-first/prefix_[0-9]{8}T[0-9]{6}\.[0-9]{6}_postfix\.s$' | wc -l` -eq 9 ] && echo xxfix-humanreadable-first_ok
[ `/bin/ls xxfix-humanreadable2-last/*.s  | /bin/grep -E '^xxfix-humanreadable2-last/prefix_[0-9]{15}_postfix\.s$'  | wc -l` -eq 9 ] && echo xxfix-humanreadable2-last_ok
[ `/bin/ls xxfix-humanreadable2-first/*.s | /bin/grep -E '^xxfix-humanreadable2-first/prefix_[0-9]{15}_postfix\.s$' | wc -l` -eq 9 ] && echo xxfix-humanreadable2-first_ok

echo '--- multilog-mod "safely written" code'
cat multilog_test_11.txt | multilog-mod cOK ./safely_written
[ `/bin/ls safely_written/ | /bin/grep -E '^\@[0-9a-f]{24}\.OK$' | wc -l` -eq 9 ] && echo safely written code ok

echo '--- multilog-mod "unsafely written" code'
( cat multilog_test.txt ; sleep 1 ) | multilog-mod cOK CNG ./unsafely_written &
MULTILOG_PID=$!
sleep 0.3
exec 2>/dev/null
kill -KILL $MULTILOG_PID
while ( kill -0 $MULTILOG_PID 2> /dev/null) ; do sleep 0.1 ; done
sleep 1
exec 2>&1
echo a | multilog-mod cOK CNG ./unsafely_written 
[ `/bin/ls unsafely_written/*.NG | /bin/grep -E '^unsafely_written/\@[0-9a-f]{24}\.NG$' | wc -l` -eq 1 ] && echo unsafely written code rename ok

( cat multilog_test_11.txt ; sleep 1 ) | multilog-mod cOK CNG fR ./unsafely_written_2 &
MULTILOG_PID=$!
sleep 0.3
exec 2>/dev/null
kill -KILL $MULTILOG_PID
while ( kill -0 $MULTILOG_PID 2> /dev/null) ; do sleep 0.1 ; done
sleep 1
exec 2>&1
INDOE1=`/bin/ls -i unsafely_written_2/*.OK | tail -1 | cut -d ' ' -f 1`
INDOE2=`/bin/ls -i unsafely_written_2/current | cut -d ' ' -f 1`
[ $INDOE1 -eq $INDOE2 ] && echo unsafely written code inode ok
echo a | multilog-mod cOK CNG fR ./unsafely_written_2 
[ `/bin/ls unsafely_written_2/*.NG | /bin/grep -E '^unsafely_written_2/\@[0-9a-f]{24}\.NG$' | wc -l` -eq 1 ] && echo unsafely written code with fR rename ok with fR
INDOE2=`/bin/ls -i unsafely_written_2/*.NG | cut -d ' ' -f 1`
[ $INDOE1 -eq $INDOE2 ] && echo unsafely written code with fR inode ok

