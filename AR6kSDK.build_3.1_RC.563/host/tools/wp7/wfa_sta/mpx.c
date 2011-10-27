#include <stdio.h>

#include "mpx.h"

void
print_hex_string (char* buf, int len)
{
    int i;

    if (len == 0)
    {
        printf("<empty string>");
        return;
    }

    for (i = 0; i < len; i++)
    {
        printf("%02x ", *((unsigned char *)buf + i));
        if ((i & 0xf) == 15)
        {
            printf("\n   ");
        }
    }

    if ((i & 0xf))
    {
        printf("\n");
    }
}

void mpx (char *m, void *buf_v, int len)
{
    char *buf = buf_v;

    printf("%s   MSG: %s\n   ", m, &buf[44]);

    print_hex_string (buf, len);
}
