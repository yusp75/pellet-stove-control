
#pragma build_attribute vfpcc_compatible
#pragma build_attribute arm_thumb_compatible
#pragma build_attribute ropi_compatible
// #pragma build_attribute rwpi_compatible

#ifdef __AS_BOUNDS__
  #error "Boundscheck should not be turned on when compiling this file"
#endif

#include <stdint.h>
#include <yfuns.h>

#pragma language=extended

void abort(void);

__interwork void __iar_ReportCheckFailedStdout(void * d);


static char * putstring(char *buf, const char *s)
{ 
  while( *s ) *buf++ = *s++;
  return buf;
}

static char * puthex(char *buf, unsigned long ul)
{
  const char hex[16] = "0123456789ABCDEF";
  buf = putstring(buf, "0x");

  char lbuf[9] = {0};
  int index = 8;

  do {
    --index;
    lbuf[index] = hex[ul & 0xF];
    ul >>= 4;
  } while(ul);

  return putstring(buf, &lbuf[index]);
}


int __iar_ReportCheckFailedStdoutAbort = 0;

__interwork void __iar_ReportCheckFailedStdout(void * d)
{
  char buf[200] = {0};
  char *b = buf;

  uint32_t const *data = (uint32_t const *)d;
  int nr = data[0] >> 24;

  b = putstring(b, "CRUN_ERROR:");
  for (int i = 0; i < nr; ++i)
  {
    *b++ = ' ';
    b = puthex(b, data[i]); 
  }
  *b++ = '\n';

  //__write(_LLIO_STDOUT, (unsigned char const *)buf, (size_t)(b - buf));
  for (int i=0; i<(size_t)(b-buf);i++)
    printf((unsigned char)buf[i]);
      

  if (__iar_ReportCheckFailedStdoutAbort)
    abort();
}
