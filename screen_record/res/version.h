#pragma once

#define STR(x)                           #x
#define VERSION(a, b, c, d)              STR(a) "." STR(b) "." STR(c) "." STR(d)

#define MAJOR                            1
#define MINOR                            0
#define BUILD                            59
#define PATCH                            0

#define VERSION_NUM                      MAJOR, MINOR, BUILD, PATCH
#define VERSION_STR                      VERSION(MAJOR, MINOR, BUILD, PATCH)

#define PRODUCT_NAME                     "Â¼ÆÁÖúÊÖ"
#define FILE_DESCRIPTION                 "ÆÁÄ»Â¼ÖÆ"
