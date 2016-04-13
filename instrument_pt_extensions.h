
// Require the expected context.  Because there is no .c implementation
// file associated with this header we don't need to allow this file to
// ever be used outside it's containing header's context (i.e. no need to
// allow compilation if INSTRUMENT_COMPILATION or so defined).
// FIXME: test this insire requirement
#ifndef INSTRUMENT_INSIDE_INSTRUMENT_FORMAT_FREE_PRINT_H
#  error included from somewhere other than instrument_format_free_print.h
#endif
