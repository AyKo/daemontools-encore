#ifndef TIMESTAMP_H
#define TIMESTAMP_H

#define TIMESTAMP 25

/* Type of timestamp format kind */
enum timestamp_kind_t {
  FILENAME_FLAG_TAI64N,
  FILENAME_FLAG_ACCUSTAMP,
  FILENAME_FLAG_HUMAN_READABLE,
  FILENAME_FLAG_MAX_
};

extern int fmt_tai64nstamp(char *);
extern int fmt_accustamp(char *);
extern int fmt_human_readable_stamp(char *);
extern int fmt_timestamp(char s[TIMESTAMP], enum timestamp_kind_t kind);
extern int fmt_length(enum timestamp_kind_t kind);

#endif
