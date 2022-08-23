#ifndef __AT_H
#define __AT_H

#define STRINGIFY(x) #x
#define TO_STRING(x) STRINGIFY(x)
#define __WHERE__ __FILE__ ": " TO_STRING(__LINE__)
#define WHERE fputs("at " __WHERE__ "\n",stderr)

#endif
//IN GOD WE TRVST.
