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
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

// Some might preferr to #define this such that output goes to stderr or
// some other open FILE pointer.
#ifndef FORMAT_FREE_PRINT_STREAM
#  define FORMAT_FREE_PRINT_STREAM stdout
#endif

// Default to use 6 significant digits (the default for %g format output
// in printf()).
#ifndef FORMAT_FREE_FREE_PRINT_FLOAT_SIGNIFICANT_DIGITS
#  define FORMAT_FREE_FREE_PRINT_FLOAT_SIGNIFICANT_DIGITS "6"
#endif

// Some users might like to redefine death itself :)
#ifndef FORMAT_FREE_PRINT_DIE
#  define FORMAT_FREE_PRINT_DIE() exit (EXIT_FAILURE)
#endif

// To use the optional format_free_print_pt_extensions.h a -D must be used
#ifdef HAVE_FORMAT_FREE_PRINT_PT_EXTENSIONS_H
#  define INSIDE_FORMAT_FREE_PRINT_H
#  include "format_free_print_pt_extensions.h"
#  undef  INSIDE_FORMAT_FREE_PRINT_H
#  ifndef FORMAT_FREE_PRINT_PT_ADDITIONAL_WIMCUPSMCS
#    error FORMAT_FREE_PRINT_PT_ADDITIONAL_WIMCUPSMCS not defined
#  endif
#else
#  define FORMAT_FREE_PRINT_PT_ADDITIONAL_WIMCUPSMCS(thing) (void) 0
#endif

// File-Line-Function Tuple
#define FORMAT_FREE_PRINT_FLFT __FILE__, __LINE__, __func__

// All the format-free stuff is GCC-specific
#ifdef __GNUC__

// Choose Expression Depending On Thing Type Match.  See the use context.
#define CEDOTTM(thing, type, exp_if_thing_of_type, exp_if_thing_not_of_type) \
  __builtin_choose_expr (                                                    \
      __builtin_types_compatible_p (                                         \
          typeof (thing),                                                    \
          type ),                                                            \
      (exp_if_thing_of_type),                                                \
      (exp_if_thing_not_of_type) )

// Not for independent use (see the existing call context).  Stands for Weird
// If Matched Check Unmatched Print Set Matched Chunk :) The outer CEDOTTM()
// evaluates to an only-one-match-allowed printf() if thing is of type, or a
// void expression otherwise.  The inner CEDOTTM() calls are needed because
// currently GCC still syntax-checks the non-selected expression arguments
// of __builtin_choose_expr(), so we have to make sure a valid format-thing
// pair get fed to printf() regardless of the type-ness of thing even at
// the point where we already know (because of the surrounding CEDOTTM()
// call) that thing is of type (the pair being format-thing for the case
// that actually happens, or "%s"-"i_am_never_seen" for the other).  Why do
// all this?  Because it gets us a warning if one of the entries in the list
// in PT() has a type-format mismatch that GCC would normally warn about.
// There are some simpler solutions that don't have this property but it
// seems worth it since bugs in debugging code are especially annoying.
//
// IMPROVEME: this could probably be generalized to take a "printer" parameter
// so users could pass down their own printer handlers to be used in PT() and
// PTX().  This would likely require deferred expansion using this technique:
//
//   #define EMPTY()
//   #define DEFER(x) x EMPTY()
//
//   N_ ()           // Expands to 0
//   DEFER (N_) ()   // Expands N_ ()
//
// This me be an easier way to allow PT() to be truly generic and capable
// of rendering arbitrary non-aliasing types, compared to the approach in
// format_free_print_pt_extensions.h.
//
#define WIMCUPSMC(thing, type, format)                                   \
  CEDOTTM (                                                              \
      (thing),                                                           \
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
                FORMAT_FREE_PRINT_FLFT ),                                \
            abort ()                                                     \
          )                                                              \
          :                                                              \
          (void) 0                                                       \
        ),                                                               \
        fprintf (                                                        \
          FORMAT_FREE_PRINT_STREAM,                                      \
          CEDOTTM ((thing), type, (format), "%s"),                       \
          CEDOTTM ((thing), type, (thing), "i_am_never_seen") ),         \
        XxX_already_matched_ = true                                      \
      ),                                                                 \
      ((void) 0) )

// Convenience alias.  Causes a name clash if clients name something FFPFSD :)
#define FFPFSD FORMAT_FREE_FREE_PRINT_FLOAT_SIGNIFICANT_DIGITS

// FIXMELATER: Seems like we would want const versions of everything in
// the below list.  C11 _Generic approach sounds like it would require
// it at least.  However for the current approach it isn't necessary and
// doesn't work for most types because the const types are apparently viewed
// as compatible (the exception being char const * which seems to need the
// const version to exist).  Seems like silly compiler buggers but whatever.

// Try to Print Thing (which must be of one of the known types).  This is
// somewhat adventurous code.  Note that if two types on this list are
// synonyms a run-time error is triggered if an attempt is made to print
// a thing of either type.  This means it isn't possible to treat aliased
// types differently via printf format codes.  The format code for each
// type printed by this interface is explicitly set here and cannot be
// changed (in fact not having to specify it every time is the point), so
// a separate version of this macro is required to e.g. render ints in hex.
// FIXXME: make use of XxX_already_matched_ more hygenic by pushing a shadow
// warning like we do in cduino version of this stuff?
#define PT(thing)                                                           \
  do {                                                                      \
    bool XxX_already_matched_ = false;                                      \
    WIMCUPSMC ( thing , char                   , "%c"             );        \
    WIMCUPSMC ( thing , char *                 , "%s"             );        \
    WIMCUPSMC ( thing , char const *           , "%s"             );        \
    WIMCUPSMC ( thing , char []                , "%s"             );        \
    WIMCUPSMC ( thing , int8_t                 , "%" PRIi8        );        \
    WIMCUPSMC ( thing , int16_t                , "%" PRIi16       );        \
    WIMCUPSMC ( thing , int32_t                , "%" PRIi32       );        \
    WIMCUPSMC ( thing , int64_t                , "%" PRIi64       );        \
    WIMCUPSMC ( thing , uint8_t                , "%" PRIu8        );        \
    WIMCUPSMC ( thing , uint16_t               , "%" PRIu16       );        \
    WIMCUPSMC ( thing , uint32_t               , "%" PRIu32       );        \
    WIMCUPSMC ( thing , uint64_t               , "%" PRIu64       );        \
    /* Unlike the shorter integer types, the long long integer types are */ \
    /* distinct from int64_t/uint64_t, even though they are the same */     \
    /* length on my platform at least.  */                                  \
    WIMCUPSMC ( thing , long long int          , "%lli"           );        \
    WIMCUPSMC ( thing , long long unsigned int , "%llu"           );        \
    WIMCUPSMC ( thing , bool                   , "%i"             );        \
    /* Note that C often promotes float to double, but this *is* tested: */ \
    WIMCUPSMC ( thing , float                  , "%." FFPFSD "g"  );        \
    WIMCUPSMC ( thing , double                 , "%." FFPFSD "g"  );        \
    WIMCUPSMC ( thing , long double            , "%." FFPFSD "Lg" );        \
    WIMCUPSMC ( thing , void *                 , "%p"             );        \
    WIMCUPSMC ( thing , void const *           , "%p"             );        \
    FORMAT_FREE_PRINT_PT_ADDITIONAL_WIMCUPSMCS(thing);                      \
    if ( ! XxX_already_matched_ ) {                                         \
      printf ("\n");            /* Flush any existing stdout output */      \
      fprintf (stderr, "\n");   /* Flush any existing stderr output */      \
      fprintf (                                                             \
          stderr,                                                           \
          "%s:%i:%s: "                                                      \
          "error: PT() macro doesn't know how to print things of type "     \
          "typeof (" #thing ")\n",                                          \
          FORMAT_FREE_PRINT_FLFT );                                         \
      abort ();                                                             \
    }                                                                       \
  } while ( 0 )

// Print Labeled thing, i.e. print stringified argument then do PT()
#define PL(thing)                                        \
   do {                                                  \
     fprintf (FORMAT_FREE_PRINT_STREAM, "%s: ", #thing); \
     PT (thing);                                         \
   } while ( 0 )

// Dump Thing, i.e. do PL() followed by a newline
#define DT(thing)                              \
   do {                                        \
     PL (thing);                               \
     fprintf (FORMAT_FREE_PRINT_STREAM, "\n"); \
   } while ( 0 )

// Trace Thing, i.e. print source location then do DT()
#define TT(thing)                  \
   do {                            \
     fprintf (                     \
         FORMAT_FREE_PRINT_STREAM, \
         "%s:%i:%s: ",             \
         FORMAT_FREE_PRINT_FLFT ); \
     DT (thing);                   \
   } while ( 0 );

// Do TT() then Die
#define TD(thing)               \
  do {                          \
    TT (thing);                 \
    do {                        \
      FORMAT_FREE_PRINT_DIE (); \
    } while ( 0 );              \
  } while ( 0 )

// Try to Print Thing in hex.  Like PT(), but only works for unsigned
// integer types and outputs the value in hex with a "0x" prefix.
#define PTX(thing)                                                        \
   do {                                                                   \
     bool XxX_already_matched_ = false;                                   \
     /* Note that these handle e.g. unsigned short, unsigned int, etc: */ \
     WIMCUPSMC ( thing, uint8_t  , "0x%02"  PRIx8  );                     \
     WIMCUPSMC ( thing, uint16_t , "0x%04"  PRIx16 );                     \
     WIMCUPSMC ( thing, uint32_t , "0x%08"  PRIx32 );                     \
     WIMCUPSMC ( thing, uint64_t , "0x%016" PRIx64 );                     \
     if ( ! XxX_already_matched_ ) {                                      \
       printf ("\n");            /* Flush any existing stdout output */   \
       fprintf (stderr, "\n");   /* Flush any existing stderr output */   \
       fprintf (                                                          \
           stderr,                                                        \
           "%s:%i:%s: "                                                   \
           "error: PTX() macro doesn't know how to print things of type " \
           "typeof (" #thing ")\n",                                       \
           FORMAT_FREE_PRINT_FLFT );                                      \
       abort ();                                                          \
     }                                                                    \
   } while ( 0 )

// Print Labeled thing in heX, i.e. print stringified argument then do PTX()
#define PLX(thing)                                       \
   do {                                                  \
     fprintf (FORMAT_FREE_PRINT_STREAM, "%s: ", #thing); \
     PTX (thing);                                        \
   } while ( 0 )

// Dump thing in heX, i.e. do PLX() then output a newline
#define DTX(thing)                             \
   do {                                        \
     PLX (thing);                              \
     fprintf (FORMAT_FREE_PRINT_STREAM, "\n"); \
   } while ( 0 )

// Trace Thing in heX, i.e. print source location then do DTX()
#define TTX(thing)                  \
   do {                             \
     fprintf (                      \
         FORMAT_FREE_PRINT_STREAM,  \
         "%s:%i:%s: ",              \
         FORMAT_FREE_PRINT_FLFT);   \
     DTX (thing);                   \
   } while ( 0 );

// Do TTX() then Die
#define TDX(thing)              \
  do {                          \
    TTX (thing);                \
    do {                        \
      FORMAT_FREE_PRINT_DIE (); \
    } while ( 0 );              \
  } while ( 0 )

#endif // __GNUC__

// FIXME: WORK POINT: make sure the tests for these next four are in the
// right place (the die ones might not have tests I'm not sure):

// Trace Value: given an expression expr and an unquoted format code, print
// the source location and expression text and value followed by a newline,
// e.g. TV (my_int, %i), TV (my_sub_returning_int (), %i).
#define TV(expr, format_code)                    \
  printf (                                       \
      "%s:%i:%s: " #expr ": " #format_code "\n", \
      FORMAT_FREE_PRINT_FLFT,                    \
      expr )

// Like TV(), but die after.
#define TVD(expr, format_code) \
  do {                         \
    TV (expr, format_code);    \
    FORMAT_FREE_PRINT_DIE ();  \
  } while ( 0 )

// Trace Stuff: print expanded format string in first argument, using values
// given in remaining arguments, tagged with source location and added newline
#ifdef __GNUC__   // This one needs GNU comma-swallowing __VA_ARGS__ extension
#  define TS(fmt, ...)                                                      \
     printf ("%s:%i:%s: " fmt "\n", FORMAT_FREE_PRINT_FLFT, ## __VA_ARGS__)
#endif

// Like TS() then die.
#ifdef __GNUC__   // This one needs GNU comma-swallowing __VA_ARGS__ extension
#  define TSD(fmt, ...)          \
     do {                        \
       TS (fmt, __VA_ARGS__);    \
       FORMAT_FREE_PRINT_DIE (); \
     } while ( 0 )
#endif

// CheckPoint (output e.g. "Hit my_file.c:42:my_func\n")
#define CP()                       \
   do {                            \
     fprintf (                     \
         FORMAT_FREE_PRINT_STREAM, \
         "Hit %s:%i:%s\n",         \
         FORMAT_FREE_PRINT_FLFT ); \
   } while ( 0 )

// Named CheckPoint (e.g. NCP(my cp) will output "my cp\n")
#define NCP(name)                                   \
  do {                                              \
    fprintf (FORMAT_FREE_PRINT_STREAM, #name "\n"); \
  } while ( 0 )

// Die Point: output (e.g. "Dying at my_file.c:42:m_func\n" then die)
#define DP()                       \
   do {                            \
     fprintf (                     \
         FORMAT_FREE_PRINT_STREAM, \
         "Dying at %s:%i:%s\n",    \
         FORMAT_FREE_PRINT_FLFT ); \
     do {                          \
       FORMAT_FREE_PRINT_DIE ();   \
     } while ( 0 );                \
   } while ( 0 )

// Named Die Point (e.g. NDP(my dp) will output "my dp\n" then die)
#define NDP(name)                                    \
   do {                                              \
     fprintf (FORMAT_FREE_PRINT_STREAM, #name "\n"); \
     do {                                            \
       FORMAT_FREE_PRINT_DIE ();                     \
     } while ( 0 );                                  \
   } while ( 0 )

#endif   // FORMAT_FREE_FREE_PRINT_H
