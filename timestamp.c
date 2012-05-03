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
      nowtm.tm_year + 1900, nowtm.tm_mon, nowtm.tm_mday,
      nowtm.tm_hour, nowtm.tm_min, nowtm.tm_sec, now.tv_usec);

  return len;
}

