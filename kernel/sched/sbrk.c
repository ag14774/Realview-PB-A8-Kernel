#include "sbrk.h"

caddr_t _sbrk( int incr ) {
    return ( caddr_t )( &boh );  
}
