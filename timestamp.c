#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include "fmt.h"
#include "taia.h"
#include "timestamp.h"

static char hex[16] = "0123456789abcdef";

int fmt_tai64nstamp(char s[TIMESTAMP])
{
  struct taia now;
  char nowpack[TAIA_PACK];
  int i;

  taia_now(&now);
  taia_pack(nowpack,&now);

  s[0] = '@';
  for (i = 0;i < 12;++i) {
    s[i * 2 + 1] = hex[(nowpack[i] >> 4) & 15];
    s[i * 2 + 2] = hex[nowpack[i] & 15];
  }
  return 25;
}

int fmt_accustamp(char s[TIMESTAMP])
{
  struct taia now;
  int len;

  taia_now(&now);

  len = fmt_ulong(s,(unsigned long)tai_tounix(&now.sec));
  s[len++] = '.';
  len += fmt_uint0(s+len,now.nano / 1000,6);
  return len;
}

/* Timestamp format - Human readable (ISO 8601) */
int fmt_human_readable_stamp(char s[TIMESTAMP])
{
  int len;

  struct timeval now;
  struct tm nowtm;
  
  gettimeofday(&now,(struct timezone *) 0);
  localtime_r(&now.tv_sec, &nowtm);

  /* 01234567890123456789012345 */
  /* YYYYMMDDThhmmss.SSSSSS     */
  /* 20120430T064033.232342     */
  len = sprintf(s, "%04d%02d%02dT%02d%02d%02d.%06ld",
      nowtm.tm_year + 1900, nowtm.tm_mon + 1, nowtm.tm_mday,
      nowtm.tm_hour, nowtm.tm_min, nowtm.tm_sec, now.tv_usec);

  return len;
}

/* Timestamp format - Human readable No2 */
int fmt_human_readable_2_stamp(char s[TIMESTAMP])
{
  int len;

  struct timeval now;
  struct tm nowtm;
  
  gettimeofday(&now,(struct timezone *) 0);
  localtime_r(&now.tv_sec, &nowtm);

  /* 01234567890123456789012345 */
  /* YYYYMMDDhhmmss.SSS         */
  /* 20120430064033.223         */
  len = sprintf(s, "%02d%02d%02d%02d%02d%02d%03ld",
      nowtm.tm_year % 100, nowtm.tm_mon + 1, nowtm.tm_mday,
      nowtm.tm_hour, nowtm.tm_min, nowtm.tm_sec, now.tv_usec / 1000);

  return len;
}

/* Timestamp format (selectable) */
int fmt_timestamp(char s[TIMESTAMP], enum timestamp_kind_t kind)
{
  int len = 0;
  switch (kind) {
    case FILENAME_FLAG_ACCUSTAMP:
      len = fmt_accustamp(s);
      break;
    case FILENAME_FLAG_HUMAN_READABLE:
      len = fmt_human_readable_stamp(s);
      break;
    case FILENAME_FLAG_HUMAN_READABLE_2:
      len = fmt_human_readable_2_stamp(s);
      break;
    case FILENAME_FLAG_TAI64N: /* FALLTHROUGH */
    default:
      len = fmt_tai64nstamp(s);
      break;
  }
  return len;
}

/* Get length of timestamp format */
int fmt_length(enum timestamp_kind_t kind)
{
  static const int len[FILENAME_FLAG_MAX_] = {
    /*                                    0123456789012345678901234 */
    25, /* FILENAME_FLAG_TAI64N           : @400000004f9ef20e283a0ebc */
    17, /* FILENAME_FLAG_ACCUSTAMP        : 1335817091.063697         */
    22, /* FILENAME_FLAG_HUMAN_READABLE   : 20120401T055440.123155    */
    15, /* FILENAME_FLAG_HUMAN_READABLE_2 : 120401055440123         */
  };

  if (FILENAME_FLAG_TAI64N <= kind && kind <= FILENAME_FLAG_HUMAN_READABLE_2) {
    return len[kind];
  }
  return len[FILENAME_FLAG_TAI64N];
}


