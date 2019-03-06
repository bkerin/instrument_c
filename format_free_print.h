// Print things without having to give their printf-style format code
//
// WARNING: I'm still experimenting with this stuff myself.  Buggy debugging
// code is the worst.  I haven't shot myself in the foot with this stuff
// yet but maybe the possibility is there, proceed with caution.
//
// IMPROVEME: the C11 _Generic keyword and associated functionality
// might be useful to make all this cleaner and more portable.  I think
// all that would really be different is the syntax, since _Generic on
// GCC is almost certainly implemented in terms of the GCC extensions
// used here.  The syntax might get lots better though.  See here:
// http://www.robertgamble.net/2012/01/c11-generic-selections.html

#ifndef FORMAT_FREE_FREE_PRINT_H
#define FORMAT_FREE_FREE_PRINT_H

#ifndef __GNUC__
#  error GCC extensions are required but __GNUC__ is not defined
#endif

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <wchar.h>

// Some might preferr to #define this such that output goes to stderr
#ifndef FORMAT_FREE_FREE_PRINT_STREAM
#  define FORMAT_FREE_FREE_PRINT_STREAM stdout
#endif // FORMAT_FREE_FREE_PRINT_STREAM

// Default to use 6 significant digits (the default for %g format output
// in printf()).
#ifndef FORMAT_FREE_FREE_PRINT_FLOAT_SIGNIFICANT_DIGITS
#  define FORMAT_FREE_FREE_PRINT_FLOAT_SIGNIFICANT_DIGITS "6"
#endif // FORMAT_FREE_FREE_PRINT_FLOAT_SIGNIFICANT_DIGITS 

// To use the optional format_free_print_pt_extensions.h a -D must be used
#ifdef HAVE_FORMAT_FREE_PRINT_PT_EXTENSIONS_H
#  define INSIDE_FORMAT_FREE_PRINT_H
#  include "format_free_print_pt_extensions.h"
#  undef  INSIDE_FORMAT_FREE_PRINT_H
#  ifndef FORMAT_FREE_PRINT_PT_ADDITIONAL_WIMCUPSMCS
#    error FORMAT_FREE_PRINT_PT_ADDITIONAL_WIMCUPSMCS not defined
#  endif
#else
#  define FORMAT_FREE_PRINT_PT_ADDITIONAL_WIMCUPSMCS
#endif

// File-Line-Function Tuple
#define FLFT __FILE__, __LINE__, __func__

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
            fprintf (stderr, "\n"),                                      \
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
        fprintf (                                                        \
          FORMAT_FREE_FREE_PRINT_STREAM,                                 \
          CEDOTTM (XxX_et_, type, (format), "%s"),                       \
          CEDOTTM (XxX_et_, type, XxX_et_, "i_am_never_seen") ),         \
        XxX_already_matched_ = true                                      \
      ),                                                                 \
      ((void) 0) )

// Convenience alias.  Causes a name clash if clients name something FFPFSD :)
#define FFPFSD FORMAT_FREE_FREE_PRINT_FLOAT_SIGNIFICANT_DIGITS

// FIXME: these FIXME apply to the stuff in cduino as well
// FIXME: why is whchar_t [] using %s?  Looks wrong, check.  But I though I
// went over all this
// FIXME: probably want const versions of everything in the below list?
// C11 _Generic approach would require that at least, I think, may need it
// here as well, actually I have a vague recollection of having checked

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
// to achieve a reasonably approximation of a hygenic macro in C: It fails
// at compile-time when any shadowing happens even if it isn't relevant to
// the PT() being done, but at least it can't silently screw up.
#define PT(thing)                                                           \
  do {                                                                      \
    _Pragma ("GCC diagnostic push");                                        \
    _Pragma ("GCC diagnostic error \"-Wshadow\"");                          \
    bool XxX_already_matched_ = false;                                      \
    typeof (thing) XxX_et_ = thing;   /* thing evaluted only here */        \
    _Pragma ("GCC diagnostic pop");                                         \
    WIMCUPSMC ( char                  , "%c"              );                \
    WIMCUPSMC ( char *                , "%s"              );                \
    WIMCUPSMC ( char const *          , "%s"              );                \
    WIMCUPSMC ( char []               , "%s"              );                \
    /* It seems that wchar_t is not distinct from int32_t, so it's out */   \
    /*WIMCUPSMC ( wchar_t               , "%lc"      );*/                   \
    WIMCUPSMC ( wchar_t *             , "%ls"             );                \
    WIMCUPSMC ( wchar_t const *       , "%ls"             );                \
    WIMCUPSMC ( wchar_t []            , "%s"              );                \
    WIMCUPSMC ( int8_t                , "%" PRIi8         );                \
    WIMCUPSMC ( int16_t               , "%" PRIi16        );                \
    WIMCUPSMC ( int32_t               , "%" PRIi32        );                \
    WIMCUPSMC ( int64_t               , "%" PRIi64        );                \
    WIMCUPSMC ( uint8_t               , "%" PRIu8         );                \
    WIMCUPSMC ( uint16_t              , "%" PRIu16        );                \
    WIMCUPSMC ( uint32_t              , "%" PRIu32        );                \
    WIMCUPSMC ( uint64_t              , "%" PRIu64        );                \
    /* Unlike the shorter integer types, the long long integer types are */ \
    /* distinct from int64_t/uint64_t, even though they are the same */     \
    /* length on my platform at least.  */                                  \
    WIMCUPSMC ( long long int         , "%lli"            );                \
    WIMCUPSMC ( long long unsigned int, "%llu"            );                \
    WIMCUPSMC ( bool                  , "%i"              );                \
    WIMCUPSMC ( float                 , "%." FFPFSD "g"   );                \
    WIMCUPSMC ( double                , "%." FFPFSD "g"   );                \
    WIMCUPSMC ( long double           , "%." FFPFSD "Lg"  );                \
    WIMCUPSMC ( void *                , "%p"              );                \
    FORMAT_FREE_PRINT_PT_ADDITIONAL_WIMCUPSMCS;                             \
    if ( ! XxX_already_matched_ ) {                                         \
      printf ("\n");            /* Flush and existing stdout output */      \
      fprintf (stderr, "\n");   /* Flush and existing stderr output */      \
      fprintf (                                                             \
          stderr,                                                           \
          "%s:%i:%s: "                                                      \
          "error: PT() macro doesn't know how to print things of type "     \
          "typeof (" #thing ")\n",                                          \
          FLFT );                                                           \
      abort ();                                                             \
    }                                                                       \
  } while ( 0 )

// Try to Print Labeled thing.  Like PT(), but precedes the value with
// "thing: ".
#define PL(thing)                                             \
   do {                                                       \
     fprintf (FORMAT_FREE_FREE_PRINT_STREAM, "%s: ", #thing); \
     PT (thing);                                              \
   } while ( 0 )

// Try to Dump Thing.  Like PL(), but also outputs a newline after the value.
#define DT(thing)                                   \
   do {                                             \
     PL (thing);                                    \
     fprintf (FORMAT_FREE_FREE_PRINT_STREAM, "\n"); \
   } while ( 0 )

// Try to Trace Thing.  Like DT(), but also add the source location as
// a prefix.
// FIXME: in this func and it's ilk, do we want to manually expand the macros
// to avoid an extra expansion and potential resulting confusion?  But then
// they have to be manually synced if changed, which is sort of sad.  Or I
// suppose there's always DEFER() or whatever but god
#define TT(thing)                                                 \
   do {                                                           \
     fprintf (FORMAT_FREE_FREE_PRINT_STREAM, "%s:%i:%s: ", FLFT); \
     DT (thing);                                                  \
   } while ( 0 );

// Try to Trace thing then Die.
#define TD(thing)        \
  do {                   \
    TT (thing);          \
    exit (EXIT_FAILURE); \
  } while ( 0 )

// Try to Print Thing in Hex.  Like PT(), but only works for unsigned
// integer types and outputs the value in hex.
#define PTX(thing)                                                        \
   do {                                                                   \
     _Pragma ("GCC diagnostic push");                                     \
     _Pragma ("GCC diagnostic error \"-Wshadow\"");                       \
     bool XxX_already_matched_ = false;                                   \
     typeof (thing) XxX_et_ = thing;   /* thing evaluted only here */     \
     _Pragma ("GCC diagnostic pop");                                      \
     WIMCUPSMC ( uint8_t         , "0x%02" PRIx8  );                      \
     WIMCUPSMC ( uint16_t        , "0x%04" PRIx16 );                      \
     WIMCUPSMC ( uint32_t        , "0x%08" PRIx32 );                      \
     if ( ! XxX_already_matched_ ) {                                      \
       printf ("\n");            /* Flush and existing stdout output */   \
       fprintf (stderr, "\n");   /* Flush and existing stderr output */   \
       fprintf (                                                          \
           stderr,                                                        \
           "%s:%i:%s: "                                                   \
           "error: PTX() macro doesn't know how to print things of type " \
           "typeof (" #thing ")\n",                                       \
           FLFT );                                                        \
       abort ();                                                          \
     }                                                                    \
   } while ( 0 )

// Try to Print Labeled thing in Hex.  Like PL(), but only works for
// unsigned integer types and outputs the value in hex.
#define PLX(thing)                                            \
   do {                                                       \
     fprintf (FORMAT_FREE_FREE_PRINT_STREAM, "%s: ", #thing); \
     PTX (thing);                                             \
   } while ( 0 )

// Try to Dump thing in Hex.  Like DT(), but only works for unsigned integer
// types and outputs the value in hex.
#define DTX(thing)                                  \
   do {                                             \
     PLX (thing);                                   \
     fprintf (FORMAT_FREE_FREE_PRINT_STREAM, "\n"); \
   } while ( 0 )

// Try to Trace Thing in Hex.  Like TT(), but only works for unsigned
// integer types and outputs the value in hex.
#define TTX(thing)                                                \
   do {                                                           \
     fprintf (FORMAT_FREE_FREE_PRINT_STREAM, "%s:%i:%s: ", FLFT); \
     DTX (thing);                                                 \
   } while ( 0 );

// Try to Trace thing (in Hex) then Die.  Like TD(), but only works for
// unsigned integer types and outputs the value in hex.
#define TDX(thing)       \
  do {                   \
    TTX (thing);         \
    exit (EXIT_FAILURE); \
  } while ( 0 )

#endif   // FORMAT_FREE_FREE_PRINT_H
