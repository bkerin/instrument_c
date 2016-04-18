// Print things without having to give their printf-style format code
//
// WARNING: I'm still experimenting with this stuff myself.  Buggy debugging
// code is the worst.  I haven't shot myself in the foot with this stuff
// yet but maybe the possibility is there, proceed with caution.

// Require the expected context.  Because there is no .c implementation
// file associated with this header we don't need to allow this file to
// ever be used outside it's containing header's context (i.e. no need to
// allow compilation if INSTRUMENT_COMPILATION or so defined).
#ifndef INSTRUMENT_INSIDE_INSTRUMENT_H
#  error included from somewhere other than instrument_.h
#endif

#ifndef INSTRUMENT_FORMAT_FREE_PRINT_H
#define INSTRUMENT_FORMAT_FREE_PRINT_H

#ifndef __GNUC__
#  error GCC extensions are required but __GNUC__ is not defined
#endif

#include <inttypes.h>
#include <stddef.h>
#include <wchar.h>

// This shouldn't be needed for format-free printing of most basic
// types since they should already be on the list.  See the sample
// instrument_pt_extensions.h for an example of how this works.
#ifdef HAVE_INSTRUMENT_PT_EXTENSIONS_H
#  define INSTRUMENT_INSIDE_INSTRUMENT_FORMAT_FREE_PRINT_H
#  include "instrument_pt_extensions.h"
#  undef  INSTRUMENT_INSIDE_INSTRUMENT_FORMAT_FREE_PRINT_H
#  ifndef INSTRUMENT_PT_ADDITIONAL_WIMCUPSMCS
#    error INSTRUMENT_PT_ADDITIONAL_WIMCUPSMCS not defined
#  endif
#else
#  define INSTRUMENT_PT_ADDITIONAL_WIMCUPSMCS
#endif

// Choose Expression Depending On Thing Type Match.  See the use context.
#define CEDOTTM(thing, type, exp_if_thing_of_type, exp_if_thing_not_of_type) \
  __builtin_choose_expr (                                                    \
      __builtin_types_compatible_p (                                         \
          typeof (thing),                                                    \
          type ),                                                            \
      (exp_if_thing_of_type),                                                \
      (exp_if_thing_not_of_type) )

// Not for independent use (see the existing call context).  Stands for
// Weird If Matched Check Unmatched Print Set Matched Chunk :) The outer
// CEDOTTM() evaluates to an only-one-match-allowed printf() if XxX_et_
// (Evaluated Thing) is of type, or a void expression otherwise.  The inner
// CEDOTTM() calls are needed because currently GCC still syntax-checks
// the non-selected expression arguments of __builtin_choose_expr(), so
// we have to make sure a valid format-XxX_et_ pair get fed to printf()
// regardless of the type-ness of XxX_et_ even at the point where we
// already know (because of the surrounding CEDOTTM() call) that XxX_et_
// is of type (the pair being format-XxX_et_ for the case that actually
// happens, or "%s"-"i_am_never_seen" for the other).  Why do all this?
// Because it gets us a warning if one of the entries in the list in PT()
// has a type-format mismatch that GCC would normally warn about.  There are
// some simpler solutions that don't have this property but it seems worth
// it since bugs in debugging code are especially annoying.

#define WIMCUPSMC(type, format)                                          \
  CEDOTTM (                                                              \
      XxX_et_,                                                           \
      type,                                                              \
      (                                                                  \
        (                                                                \
          XxX_already_matched_ ?                                         \
          (                                                              \
            printf ("\n"),                                               \
            fprintf (                                                    \
                stderr,                                                  \
                "%s:%i:%s: "                                             \
                "bug in PT() macro: a synonym for type '" #type "' was " \
                "already supplied earlier in the list (second synonyn "  \
                "handler will never fire)\n",                            \
                FLFT ),                                                  \
            abort ()                                                     \
          )                                                              \
          :                                                              \
          (void) 0                                                       \
        ),                                                               \
        printf (                                                         \
          CEDOTTM (XxX_et_, type, (format), "%s"),                       \
          CEDOTTM (XxX_et_, type, XxX_et_, "i_am_never_seen") ),         \
        XxX_already_matched_ = true                                      \
      ),                                                                 \
      ((void) 0) )

// Try to Print Thing (which must be of one of the known types).  This is
// somewhat adventurous code.  Note that if two types on this list are
// synonyms a run-time error is triggered if an attempt is made to print
// a thing of either type.  This means it isn't possible to treat aliased
// types differently via printf format codes.  The format code for each type
// printed by this interface is explicitly set here and cannot be changed
// (in fact not having to specify it every time is the point), so a separate
// version of this macro would be required to e.g. render ints in hex.
// It might be better to use __auto_type rather than typeof() here, but
// it doesn't work from C++ and would only help in the very minority case
// of a variably modified type that can't tolerate being evaluated more
// than once.  We use weird "XxX_" prefix and "_" postfix and _Pragma()
// to achieve a reasonably approximation of a hygenic macro in C: it fails
// at compile-time when any shadowing happens even if it isn't relevant to
// the PT() being done, but at least it can't silently screw up.
#define PT(thing)                                                       \
  do {                                                                  \
    _Pragma ("GCC diagnostic push");                                    \
    _Pragma ("GCC diagnostic error \"-Wshadow\"");                      \
    bool XxX_already_matched_ = false;                                  \
    typeof (thing) XxX_et_ = thing;   /* thing evaluted only here */    \
    _Pragma ("GCC diagnostic pop");                                     \
    WIMCUPSMC ( char            , "%c"       );                         \
    WIMCUPSMC ( wchar_t         , "%lc"      );                         \
    WIMCUPSMC ( char *          , "%s"       );                         \
    WIMCUPSMC ( char const *    , "%s"       );                         \
    WIMCUPSMC ( char []         , "%s"       );                         \
    WIMCUPSMC ( wchar_t *       , "%ls"      );                         \
    WIMCUPSMC ( wchar_t const * , "%ls"      );                         \
    WIMCUPSMC ( wchar_t []      , "%s"       );                         \
    WIMCUPSMC ( int8_t          , "%" PRIi8  );                         \
    WIMCUPSMC ( int16_t         , "%" PRIi16 );                         \
    WIMCUPSMC ( int32_t         , "%" PRIi32 );                         \
    WIMCUPSMC ( int64_t         , "%" PRIi64 );                         \
    WIMCUPSMC ( uint8_t         , "%" PRIu8  );                         \
    WIMCUPSMC ( uint16_t        , "%" PRIu16 );                         \
    WIMCUPSMC ( uint32_t        , "%" PRIu32 );                         \
    WIMCUPSMC ( uint64_t        , "%" PRIu64 );                         \
    WIMCUPSMC ( float           , "%g"       );                         \
    WIMCUPSMC ( double          , "%g"       );                         \
    WIMCUPSMC ( long double     , "%Lg"      );                         \
    WIMCUPSMC ( void *          , "%p"       );                         \
    INSTRUMENT_PT_ADDITIONAL_WIMCUPSMCS;                                \
    if ( ! XxX_already_matched_ ) {                                     \
      printf ("\n");                                                    \
      fprintf (                                                         \
          stderr,                                                       \
          "%s:%i:%s: "                                                  \
          "error: PT() macro doesn't know how to print things of type " \
          "typeof (" #thing ")\n",                                      \
          FLFT );                                                       \
      abort ();                                                         \
    }                                                                   \
  } while ( 0 ) 

// Trace Thing.  This just puts the source location, thing text, and a
// newline around PT().
#define TT(thing)                            \
  do {                                       \
    printf ("%s:%i:%s: " #thing ": ", FLFT); \
    PT (thing);                              \
    printf ("\n");                           \
  } while ( 0 );

#endif   // INSTRUMENT_FORMAT_FREE_PRINT_H
