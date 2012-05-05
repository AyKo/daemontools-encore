#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "direntry.h"
#include "alloc.h"
#include "stralloc.h"
#include "buffer.h"
#include "strerr.h"
#include "error.h"
#include "open.h"
#include "lock.h"
#include "scan.h"
#include "str.h"
#include "byte.h"
#include "seek.h"
#include "timestamp.h"
#include "wait.h"
#include "closeonexec.h"
#include "env.h"
#include "fd.h"
#include "sig.h"
#include "match.h"
#include "deepsleep.h"

extern int rename(const char *,const char *);

#define FATAL "multilog: fatal: "
#define WARNING "multilog: warning: "

void pause3(const char *s1,const char *s2,const char *s3)
{
  strerr_warn4(WARNING,s1,s2,s3,&strerr_sys);
  deepsleep(5);
}

void pause5(const char *s1,const char *s2,const char *s3,const char *s4,const char *s5)
{
  strerr_warn6(WARNING,s1,s2,s3,s4,s5,&strerr_sys);
  deepsleep(5);
}

int fdstartdir;

int *f;

void f_init(char **script)
{
  int i;
  int fd;

  for (i = 0;script[i];++i)
    ;

  f = (int *) alloc(i * sizeof(*f));
  if (!f) strerr_die2x(111,FATAL,"out of memory");

  for (i = 0;script[i];++i) {
    fd = -1;
    if (script[i][0] == '=') {
      if (fchdir(fdstartdir) == -1)
        strerr_die2sys(111,FATAL,"unable to switch to starting directory: ");
      fd = open_append(script[i] + 1);
      if (fd == -1)
        strerr_die4sys(111,FATAL,"unable to create ",script[i] + 1,": ");
      close(fd);
      fd = open_write(script[i] + 1);
      if (fd == -1)
        strerr_die4sys(111,FATAL,"unable to write ",script[i] + 1,": ");
      closeonexec(fd);
    }
    f[i] = fd;
  }
}

/* Flag-value of timing to create a log file */
enum logcreate_timing_t {
  LOGCREATE_TIMING_LAST,
  LOGCREATE_TIMING_FIRST,
};

struct cyclog {
  char buf[512];
  buffer ss;
  int fdcurrent;
  unsigned long bytes;
  unsigned long num;
  unsigned long size;
  const char *processor;
  const char *code_finished;
  const char *dir;
  int fddir;
  int fdlock;
  int flagselected;
  enum timestamp_kind_t flag_filename;
  enum logcreate_timing_t flag_logcreate_timing;
} *c;
int cnum;

stralloc fn = {0};

/* Initialize a temporary variable for logfile-name comparison */
void init_filename_cmp(const struct cyclog *d)
{
  switch (d->flag_filename) {
    case FILENAME_FLAG_ACCUSTAMP:
      str_copy(fn.s, "9999999999.999999");
      break;
    case FILENAME_FLAG_HUMAN_READABLE:
      str_copy(fn.s, "99999999T999999.999999");
      break;
    case FILENAME_FLAG_TAI64N: /* FALLTHROUGH */
    default:
      fn.s[0] = '@';
      fn.s[1] = 'z';
      fn.s[2] = 0;
      break;
  }
}

/* Check format string in forward match */
int checkname(const char* filter, const char* target)
{
  for (; *filter && *target ; ++filter, ++target) {
    switch (*filter) {
      case '9':
        if ('0' > *target || *target > '9')
          return 0;
        break;
      default:
        if (*filter != *target)
          return 0;
        break;
    }
  }
  return 1;
}

/* Check logfile-name in d->flag_filename */ 
int check_filename(const struct cyclog *d, const direntry* x)
{
  switch (d->flag_filename) {
    case FILENAME_FLAG_ACCUSTAMP:
      return checkname("9999999999.999999", x->d_name);
    case FILENAME_FLAG_HUMAN_READABLE:
      return checkname("99999999T999999.999999", x->d_name);
    case FILENAME_FLAG_TAI64N: /* FALLTHROUGH */
    default:
      if (x->d_name[0] == '@') {
        if (str_len(x->d_name) >= 25) {
          return 1;
        }
      }
      break;
  }
  return 0;
}

int filesfit(struct cyclog *d)
{
  DIR *dir;
  direntry *x;
  int count;
  int i;
  const unsigned int format_length = fmt_length(d->flag_filename);

  dir = opendir(".");
  if (!dir) return -1;

  init_filename_cmp(d);

  count = 0;
  for (;;) {
    errno = 0;
    x = readdir(dir);
    if (!x) break;
    if (check_filename(d, x)) {
      ++count;
      if (str_diff(x->d_name,fn.s) < 0) {
        for (i = 0;i < format_length;++i)
          fn.s[i] = x->d_name[i];
        fn.s[format_length] = 0;
      }
    }
  }
  if (errno) { closedir(dir); return -1; }
  closedir(dir);

  if (count < d->num) return 1;

  dir = opendir(".");
  if (!dir) return -1;

  for (;;) {
    errno = 0;
    x = readdir(dir);
    if (!x) break;
    if (str_len(x->d_name) >= format_length)
      if (str_start(x->d_name,fn.s)) {
        unlink(x->d_name);
        break;
      }
  }
  if (errno) { closedir(dir); return -1; }
  closedir(dir);
  return 0;
}

void create_link(struct cyclog *d,const char *file,const char *code)
{
  int fnlen;

  for (;;) {
    fnlen = fmt_timestamp(fn.s, d->flag_filename);
    fn.s[fnlen++] = '.';
    do {
      fn.s[fnlen++] = *code;
    } while (*code++ != 0);

    if (link(file,fn.s) == 0) break;
    pause5("unable to link to ",d->dir,"/",fn.s,", pausing: ");
  }
}

void finish(struct cyclog *d,const char *file,const char *code)
{
  struct stat st;

  for (;;) {
    if (stat(file,&st) == 0) break;
    if (errno == error_noent) return;
    pause5("unable to stat ",d->dir,"/",file,", pausing: ");
  }

  if (st.st_nlink == 1) {
    if (d->flag_logcreate_timing == LOGCREATE_TIMING_LAST) {
      create_link(d,file,code);
    }
  }

  while (unlink(file) == -1)
    pause5("unable to remove ",d->dir,"/",file,", pausing: ");

  for (;;)
    switch(filesfit(d)) {
      case 1: return;
      case -1: pause3("unable to read ",d->dir,", pausing: ");
    }
}

void startprocessor(struct cyclog *d)
{
  const char *args[4];
  int fd;

  sig_uncatch(sig_term);
  sig_uncatch(sig_alarm);
  sig_unblock(sig_term);
  sig_unblock(sig_alarm);

  fd = open_read("previous");
  if (fd == -1) return;
  if (fd_move(0,fd) == -1) return;
  fd = open_trunc("processed");
  if (fd == -1) return;
  if (fd_move(1,fd) == -1) return;
  fd = open_read("state");
  if (fd == -1) return;
  if (fd_move(4,fd) == -1) return;
  fd = open_trunc("newstate");
  if (fd == -1) return;
  if (fd_move(5,fd) == -1) return;

  args[0] = "sh";
  args[1] = "-c";
  args[2] = d->processor;
  args[3] = 0;
  execve("/bin/sh",(char*const*)args,environ);
}

void fullcurrent(struct cyclog *d)
{
  int fd;
  int pid;
  int wstat;

  while (fchdir(d->fddir) == -1)
    pause3("unable to switch to ",d->dir,", pausing: ");

  while (fsync(d->fdcurrent) == -1)
    pause3("unable to write ",d->dir,"/current to disk, pausing: ");
  close(d->fdcurrent);

  while (rename("current","previous") == -1)
    pause3("unable to rename current to previous in directory ",d->dir,", pausing: ");
  while ((d->fdcurrent = open_append("current")) == -1)
    pause3("unable to create ",d->dir,"/current, pausing: ");
  if (d->flag_logcreate_timing == LOGCREATE_TIMING_FIRST) {
    create_link(d, "current", "s");
  }
  closeonexec(d->fdcurrent);
  d->bytes = 0;
  while (fchmod(d->fdcurrent,0644) == -1)
    pause3("unable to set mode of ",d->dir,"/current, pausing: ");

  while (chmod("previous",0744) == -1)
    pause3("unable to set mode of ",d->dir,"/previous, pausing: ");

  if (!d->processor)
    finish(d,"previous",d->code_finished);
  else {
    for (;;) {
      while ((pid = fork()) == -1)
        pause3("unable to fork for processor in ",d->dir,", pausing: ");
      if (!pid) {
        startprocessor(d);
        strerr_die4sys(111,FATAL,"unable to run ",d->processor,": ");
      }
      if (wait_pid(&wstat,pid) == -1)
        pause3("wait failed for processor in ",d->dir,", pausing: ");
      else if (wait_crashed(wstat))
        pause3("processor crashed in ",d->dir,", pausing: ");
      else if (!wait_exitcode(wstat))
        break;
      strerr_warn4(WARNING,"processor failed in ",d->dir,", pausing",0);
      deepsleep(5);
    }

    while ((fd = open_append("processed")) == -1)
      pause3("unable to create ",d->dir,"/processed, pausing: ");
    while (fsync(fd) == -1)
      pause3("unable to write ",d->dir,"/processed to disk, pausing: ");
    while (fchmod(fd,0744) == -1)
      pause3("unable to set mode of ",d->dir,"/processed, pausing: ");
    close(fd);

    while ((fd = open_append("newstate")) == -1)
      pause3("unable to create ",d->dir,"/newstate, pausing: ");
    while (fsync(fd) == -1)
      pause3("unable to write ",d->dir,"/newstate to disk, pausing: ");
    close(fd);

    while (unlink("previous") == -1)
      pause3("unable to remove ",d->dir,"/previous, pausing: ");
    while (rename("newstate","state") == -1)
      pause3("unable to rename newstate to state in directory ",d->dir,", pausing: ");
    finish(d,"processed",d->code_finished);
  }
}

int c_write(int pos,char *buf,int len)
{
  struct cyclog *d;
  int w;

  d = c + pos;

  if (d->bytes >= d->size)
    fullcurrent(d);

  if (len >= d->size - d->bytes)
    len = d->size - d->bytes;

  if (d->bytes + len >= d->size - 2000) {
    w = byte_rchr(buf,len,'\n');
    if (w < len)
      len = w + 1;
  }

  for (;;) {
    w = write(d->fdcurrent,buf,len);
    if (w > 0) break;
    pause3("unable to write to ",d->dir,"/current, pausing: ");
  }

  d->bytes += w;
  if (d->bytes >= d->size - 2000)
    if (buf[w - 1] == '\n')
      fullcurrent(d);

  return w;
}

/* Aftertreatment of an outage for the case of "create log file in first".
 * 
 * Change ".u" extension of the log file with the i-node equal to the "current" file.
 */
void aftertreat_create_of_first(const struct cyclog *d, const struct stat* current_st)
{
  DIR *dir;
  direntry *x;

  if (current_st->st_nlink <= 1)
    return;

  dir = opendir(".");
  if (!dir) return;

  for (;;) {
    x = readdir(dir);
    if (!x) break;
    if (check_filename(d, x)) {
      if (current_st->st_ino == x->d_ino) {
        unsigned int len = str_len(x->d_name);
        if ((len > 1) && (x->d_name[len-1] == 's')) { 
          str_copy(fn.s, x->d_name);
          fn.s[len-1] = 'u';
          rename(x->d_name, fn.s);
        }
      }
    }
  }

  closedir(dir);
}

void restart(struct cyclog *d)
{
  struct stat st;
  int fd;
  int flagprocessed;

  if (fchdir(fdstartdir) == -1)
    strerr_die2sys(111,FATAL,"unable to switch to starting directory: ");

  mkdir(d->dir,0700);
  d->fddir = open_read(d->dir);
  if ((d->fddir == -1) || (fchdir(d->fddir) == -1))
    strerr_die4sys(111,FATAL,"unable to open directory ",d->dir,": ");
  closeonexec(d->fddir);

  d->fdlock = open_append("lock");
  if ((d->fdlock == -1) || (lock_exnb(d->fdlock) == -1))
    strerr_die4sys(111,FATAL,"unable to lock directory ",d->dir,": ");
  closeonexec(d->fdlock);

  if (stat("current",&st) == -1) {
    if (errno != error_noent)
      strerr_die4sys(111,FATAL,"unable to stat ",d->dir,"/current: ");
  }
  else {
    if (st.st_mode & 0100) {
      fd = open_append("current");
      if (fd == -1)
        strerr_die4sys(111,FATAL,"unable to append to ",d->dir,"/current: ");
      if (fchmod(fd,0644) == -1)
        strerr_die4sys(111,FATAL,"unable to set mode of ",d->dir,"/current: ");
      closeonexec(fd);
      if (d->flag_logcreate_timing == LOGCREATE_TIMING_FIRST) {
        if (st.st_nlink == 1)
          create_link(d, "current", "s");
      }
      d->fdcurrent = fd;
      d->bytes = st.st_size;
      return;
    }
    if (d->flag_logcreate_timing == LOGCREATE_TIMING_FIRST) {
      aftertreat_create_of_first(d, &st);
    }
  }

  unlink("state");
  unlink("newstate");

  flagprocessed = 0;
  if (stat("processed",&st) == -1) {
    if (errno != error_noent)
      strerr_die4sys(111,FATAL,"unable to stat ",d->dir,"/processed: ");
  }
  else if (st.st_mode & 0100)
    flagprocessed = 1;

  if (flagprocessed) {
    unlink("previous");
    finish(d,"processed",d->code_finished);
  }
  else {
    unlink("processed");
    finish(d,"previous","u");
  }

  finish(d,"current","u");

  fd = open_trunc("state");
  if (fd == -1)
    strerr_die4sys(111,FATAL,"unable to write to ",d->dir,"/state: ");
  close(fd);
  fd = open_append("current");
  if (d->flag_logcreate_timing == LOGCREATE_TIMING_FIRST) {
    create_link(d, "current", "s");
  }
  if (fd == -1)
    strerr_die4sys(111,FATAL,"unable to write to ",d->dir,"/current: ");
  if (fchmod(fd,0644) == -1)
    strerr_die4sys(111,FATAL,"unable to set mode of ",d->dir,"/current: ");
  closeonexec(fd);
  d->fdcurrent = fd;
  d->bytes = 0;
}

/* Analyse output-file flag (for c_init)
 *
 * <Filename flag>
 * t:tai64n stamp (Default)
 * T:accustamp
 * h:human-readable stamp (YYYYMMDD-HHMMSS.ssssss)
 * <Generate timing flag>
 * r:generate logfile last
 * R:generate logfile first */
void c_init_filename(
  const char* script,
  enum timestamp_kind_t* flag_filename,
  enum logcreate_timing_t* flag_logcreate_timing)
{
  const char* head = script;
  for (++script; *script; ++script) {
    switch (*script) {
      case 'T':
        *flag_filename = FILENAME_FLAG_ACCUSTAMP;
        break;
      case 'h':
        *flag_filename = FILENAME_FLAG_HUMAN_READABLE;
        break;
      case 't':
        *flag_filename = FILENAME_FLAG_TAI64N;
        break;
      case 'R':
        *flag_logcreate_timing = LOGCREATE_TIMING_FIRST;
        break;
      case 'r':
        *flag_logcreate_timing = LOGCREATE_TIMING_LAST;
        break;
      default:
        strerr_warn3(WARNING,"output file flag error in ",head,0);
        break;
    }
  }
}

void c_init(char **script)
{
  int i;
  struct cyclog *d;
  const char *processor;
  unsigned long num;
  unsigned long size;
  const char *code_finished = "s";
  enum timestamp_kind_t flag_filename = FILENAME_FLAG_TAI64N;
  enum logcreate_timing_t flag_logcreate_timing = LOGCREATE_TIMING_LAST;

  cnum = 0;
  for (i = 0;script[i];++i)
    if ((script[i][0] == '.') || (script[i][0] == '/'))
      ++cnum;

  c = (struct cyclog *) alloc(cnum * sizeof(*c));
  if (!c) strerr_die2x(111,FATAL,"out of memory");

  d = c;
  processor = 0;
  num = 10;
  size = 99999;
  flag_filename = FILENAME_FLAG_TAI64N;
  flag_logcreate_timing = LOGCREATE_TIMING_LAST;

  for (i = 0;script[i];++i)
    if (script[i][0] == 's') {
      scan_ulong(script[i] + 1,&size);
      if (size < 4096) size = 4096;
      if (size > 16777215) size = 16777215;
    }
    else if (script[i][0] == 'n') {
      scan_ulong(script[i] + 1,&num);
      if (num < 2) num = 2;
    }
    else if (script[i][0] == '!') {
      processor = script[i] + 1;
    }
    else if (script[i][0] == '+') {
      code_finished = script[i] + 1;
      if (!stralloc_ready(&fn,str_len(code_finished)+TIMESTAMP+1))
	strerr_die2sys(111,FATAL,"unable to allocate memory: ");
    }
    else if (script[i][0] == 'f') {
      c_init_filename(script[i], &flag_filename, &flag_logcreate_timing);
    } 
    else if ((script[i][0] == '.') || (script[i][0] == '/')) {
      d->num = num;
      d->size = size;
      d->processor = processor;
      d->code_finished = code_finished;
      d->dir = script[i];
      d->flag_filename = flag_filename;
      d->flag_logcreate_timing = flag_logcreate_timing;
      buffer_init(&d->ss,c_write,d - c,d->buf,sizeof d->buf);
      restart(d);
      ++d;
      flag_filename = FILENAME_FLAG_TAI64N;
      flag_logcreate_timing = LOGCREATE_TIMING_LAST;
    }
}

void c_quit(void)
{
  int j;

  for (j = 0;j < cnum;++j) {
    buffer_flush(&c[j].ss);
    while (fsync(c[j].fdcurrent) == -1)
      pause3("unable to write ",c[j].dir,"/current to disk, pausing: ");
    while (fchmod(c[j].fdcurrent,0744) == -1)
      pause3("unable to set mode of ",c[j].dir,"/current, pausing: ");
  }
}

int flagexitasap = 0;
int flagforcerotate = 0;
int flagnewline = 1;

void exitasap(void)
{
  flagexitasap = 1;
}

void forcerotate(void)
{
  flagforcerotate = 1;
}

int flushread(int fd,char *buf,int len)
{
  int j;

  for (j = 0;j < cnum;++j)
    buffer_flush(&c[j].ss);
  if (flagforcerotate) {
    for (j = 0;j < cnum;++j)
      if (c[j].bytes > 0)
	fullcurrent(&c[j]);
    flagforcerotate = 0;
  }

  if (!len) return 0;
  if (flagexitasap) {
    if (flagnewline) return 0;
    len = 1;
  }

  sig_unblock(sig_term);
  sig_unblock(sig_alarm);

  len = read(fd,buf,len);

  sig_block(sig_term);
  sig_block(sig_alarm);

  if (len <= 0) return len;
  flagnewline = (buf[len - 1] == '\n');
  return len;
}

char inbuf[1024];
buffer ssin = BUFFER_INIT(flushread,0,inbuf,sizeof inbuf);

char line[1001];
int linelen; /* 0 <= linelen <= 1000 */

void doit(char **script)
{
  int flageof;
  char ch;
  int j;
  int i;
  const char *action;
  int flagselected;
  int flagtimestamp;

  flagtimestamp = 0;
  if (script[0])
    if (script[0][0] == 't' || script[0][0] == 'T' || script[0][0] == 'h')
      flagtimestamp = script[0][0];

  for (i = 0;i <= 1000;++i) line[i] = '\n';
  linelen = 0;

  flageof = 0;
  for (;;) {
    for (i = 0;i < linelen;++i) line[i] = '\n';
    linelen = 0;

    while (linelen < 1000) {
      if (buffer_GETC(&ssin,&ch) <= 0) {
        if (!linelen) return;
        flageof = 1;
        break;
      }
      if (!linelen)
        if (flagtimestamp) {
          switch (flagtimestamp) {
            case 'T':
              linelen = fmt_accustamp(line);
              break;
            case 'h':
              linelen = fmt_human_readable_stamp(line);
              break;
            case 't':   /* FALLTHROUGH */
            default:
              linelen = fmt_tai64nstamp(line);
              break;
          }
          line[linelen++] = ' ';
        }
      if (ch == '\n')
        break;
      line[linelen++] = ch;
    }

    flagselected = 1;
    j = 0;
    for (i = 0;(action = script[i]) != 0;++i)
      switch(*action) {
        case '+':
          if (!flagselected)
            if (match(action + 1,line,linelen))
              flagselected = 1;
          break;
        case '-':
          if (flagselected)
            if (match(action + 1,line,linelen))
              flagselected = 0;
          break;
        case 'e':
          if (flagselected) {
            if (linelen > 200) {
              buffer_put(buffer_2,line,200);
              buffer_puts(buffer_2,"...\n");
            }
            else {
              buffer_put(buffer_2,line,linelen);
              buffer_puts(buffer_2,"\n");
            }
            buffer_flush(buffer_2);
          }
          break;
        case '=':
          if (flagselected)
            for (;;) {
              while (seek_begin(f[i]) == -1)
                pause3("unable to move to beginning of ",action + 1,", pausing: ");
              if (write(f[i],line,1001) == 1001)
                break;
              pause3("unable to write ",action + 1,", pausing: ");
            }
          break;
        case '.':
        case '/':
          c[j].flagselected = flagselected;
          ++j;
          break;
      }

    for (j = 0;j < cnum;++j)
      if (c[j].flagselected)
        buffer_put(&c[j].ss,line,linelen);
        
    if (linelen == 1000)
      for (;;) {
        if (buffer_GETC(&ssin,&ch) <= 0) {
          flageof = 1;
          break;
        }
        if (ch == '\n')
          break;
        for (j = 0;j < cnum;++j)
          if (c[j].flagselected)
            buffer_PUTC(&c[j].ss,ch);
      }

    for (j = 0;j < cnum;++j)
      if (c[j].flagselected) {
	ch = '\n';
        buffer_PUTC(&c[j].ss,ch);
      }

    if (flageof) return;
  }
}

int main(int argc,char **argv)
{
  umask(022);

  if (!stralloc_ready(&fn,40))
    strerr_die2sys(111,FATAL,"unable to allocate memory: ");

  fdstartdir = open_read(".");
  if (fdstartdir == -1)
    strerr_die2sys(111,FATAL,"unable to switch to current directory: ");
  closeonexec(fdstartdir);

  sig_block(sig_term);
  sig_block(sig_alarm);
  sig_catch(sig_term,exitasap);
  sig_catch(sig_alarm,forcerotate);

  ++argv;
  f_init(argv);
  c_init(argv);
  doit(argv);
  c_quit();
  _exit(0);
}
