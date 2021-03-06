#include <sys/syscall.h>
#include <sys/defs.h>
#include <sys/utils.h>
#include <sys/stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

static char write_buf[1024];

int printf(const char *str, ...){

    va_list ap;
    const char *ptr = NULL;
    char istr[100];
    int32_t len = 0;

    va_start(ap, str); 
    for (ptr = str; *ptr; ptr++) {
        if (*ptr == '%') {
            switch (*++ptr) {
                case 'd':
             	{
                    int32_t isNegative = 0;
                    int32_t ival = va_arg(ap, int32_t);
                    char *dstr;

                    if (ival < 0) {
                        isNegative = 1;
                        ival = -ival;
                    }
                    dstr = itoa(ival, istr+99, 10);
                    if (isNegative) {
                        *--dstr = '-';
                    }
                    memcpy((void *)(write_buf + len), (void *)dstr, str_len(dstr));
                    len += str_len(dstr);
                    break;
                }
                case 'p':
                {
                    char *pstr = itoa(va_arg(ap, uint64_t), istr+99, 16);
                    *--pstr = 'x';
                    *--pstr = '0';
                    memcpy((void *)(write_buf + len), (void *)pstr, str_len(pstr));
                    len += str_len(pstr);
                    break;
                }
                case 'x':
                {
                    char *xtr;
                    xtr = itoa(va_arg(ap, uint64_t), istr+99, 16);
                    memcpy((void *)(write_buf + len), (void *)xtr , str_len(xtr));
                    len += str_len(xtr);
                    break;
                }
                case 'c':
                {
                    write_buf[len] = va_arg(ap, uint32_t);
                    len += 1;
                    break;
                }
                case 's':
                {
                    char *str;
                    str = va_arg(ap, char *);
                    memcpy((void *)(write_buf + len), (void *)str , str_len(str));
                    len += str_len(str);
                    break;
                }
                case '\0':
                    ptr--;
                    break;
                default:
                {
                    memcpy((void *)(write_buf + len), (void *) ptr ,1 );
                    len += 1;
                    break;
                }
            }
         } else {
            memcpy((void *)(write_buf + len), (void *) ptr ,1 );
            len += 1;
       }
    }
    va_end(ap); 
    write_buf[len] = '\0';
	return write(1, write_buf, str_len(write_buf));
}
