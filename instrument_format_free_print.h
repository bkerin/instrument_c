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

#include <assert.h> // FIXME: remove this if end up not using assert
#include <inttypes.h>
#include <stddef.h>
#include <wchar.h>

// FIXME: remove the lines for INSTRUMENT_PT_ADDITIONAL_ELSE_CLAUSES once
// we're sure we're done with that approach
#ifdef HAVE_INSTRUMENT_PT_EXTENSIONS_H
#  define INSTRUMENT_INSIDE_INSTRUMENT_FORMAT_FREE_PRINT_H
#  include "instrument_pt_extensions.h"
#  undef  INSTRUMENT_INSIDE_INSTRUMENT_FORMAT_FREE_PRINT_H
#  ifndef INSTRUMENT_PT_ADDITIONAL_ELSE_CLAUSES
#    error INSTRUMENT_PT_ADDITIONAL_ELSE_CLAUSES not defined
#  endif
#else
#  define INSTRUMENT_PT_ADDITIONAL_ELSE_CLAUSES
#endif
    
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

// FIXME: should we be parenizing macro arguments anywhere?  I think it caused
// a problem to do it for the type argument in particular so I don't know

// Choose Expression Depending On Thing Type Match.  See the use context.
#define CEDOTTM(thing, type, exp_if_thing_of_type, exp_if_thing_not_of_type) \
  __builtin_choose_expr (                                                    \
      __builtin_types_compatible_p (                                         \
          typeof (thing),                                                    \
          type ),                                                            \
      exp_if_thing_of_type,                                                  \
      exp_if_thing_not_of_type )

// FIXME: remember to give already_matched a namespaced and pseudo-hygenic
// name

// Not for independent use (see the existing call context).  Stands for Weird
// If Matched Check Unmatched Print Set Matched Chunk :) The outer CEDOTTM()
// evaluates to a only-one-match allowed printf() if thing is of type, or a
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
#define WIMCUPSMC(thing, type, format)                                   \
  CEDOTTM (                                                              \
      thing,                                                             \
      type,                                                              \
      (                                                                  \
        (                                                                \
          already_matched ?                                              \
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
          CEDOTTM (thing, type, format, "%s"),                           \
          CEDOTTM (thing, type, thing, "i_am_never_seen") ),             \
        already_matched = true                                           \
      ),                                                                 \
      ((void) 0) )

// Try to Print Thing (which must be of one of the known types).  This is
// somewhat adventurous code.  Note that if two types on this list are
// synonyms a run-time error is triggered if an attempt is made to print
// a thing of either type.  This means it isn't possible to treat aliased
// types differently via printf format codes.  The format code for each
// type printed by this interface is explicitly here and cannot be changed
// (in fact not having to specify it every time is the point), so a seperate
// version of this macro would be required to e.g. render ints in hex.
#define PT2(thing)                                                    \
  do {                                                                \
    bool already_matched = false;                                     \
    WIMCUPSMC ( thing , char            , "%c"       );               \
    WIMCUPSMC ( thing , wchar_t         , "%lc"      );               \
    WIMCUPSMC ( thing , char *          , "%s"       );               \
    WIMCUPSMC ( thing , char const *    , "%s"       );               \
    WIMCUPSMC ( thing , char []         , "%s"       );               \
    WIMCUPSMC ( thing , wchar_t *       , "%ls"      );               \
    WIMCUPSMC ( thing , wchar_t const * , "%ls"      );               \
    WIMCUPSMC ( thing , wchar_t []      , "%s"       );               \
    WIMCUPSMC ( thing , int8_t          , "%" PRIi8  );               \
    WIMCUPSMC ( thing , int16_t         , "%" PRIi16 );               \
    WIMCUPSMC ( thing , int32_t         , "%" PRIi32 );               \
    WIMCUPSMC ( thing , int64_t         , "%" PRIi64 );               \
    WIMCUPSMC ( thing , uint8_t         , "%" PRIu8  );               \
    WIMCUPSMC ( thing , uint16_t        , "%" PRIu16 );               \
    WIMCUPSMC ( thing , uint32_t        , "%" PRIu32 );               \
    WIMCUPSMC ( thing , uint64_t        , "%" PRIu64 );               \
    WIMCUPSMC ( thing , float           , "%g"       );               \
    WIMCUPSMC ( thing , double          , "%g"       );               \
    WIMCUPSMC ( thing , long double     , "%g"       );               \
    WIMCUPSMC ( thing , void *          , "%p"       );               \
    INSTRUMENT_PT_ADDITIONAL_WIMCUPSMCS                               \
    if ( ! already_matched ) {                                        \
      printf ("\n");                                                  \
      fprintf (                                                       \
          stderr,                                                     \
          "%s:%i:%s: "                                                \
          "error: PT() macro don't know how to print things of type " \
          "typeof (" #thing ")\n",                                    \
          FLFT );                                                     \
      abort ();                                                       \
    }                                                                 \
  } while ( false );

// FIXME: really use the new-age while ( false ) form?

// Trace Thing.  This just puts the source location and a newline around PT().
#define TT2(thing)                \
  do {                           \
    printf ("%s:%i:%s: ", FLFT); \
    PT2 (thing);                  \
    printf ("\n");               \
  } while ( 0 );

// FIXME: remove this old way and make a commit with good message

// Not meant for stand-along use.  See the context.  In theory
// __builtin_choose_expr coudl be used as well for compile-time conditional
// printing, but the syntax is gross and we don't care about speed or a
// compile-time message in this case (it's nuts to use this for anything but
// debug output during active development).  It might be worth it to avoid
// the -Wformat suppression pragma that's needed to prevent warnings from the
// non-selected type clauses, but as of this writing __builtin_choose_expr
// doesn't guarantee that the non-selected expressions don't generate syntax
// errors anyway.
#define THING_OF_TYPE_PRINT_USING_FORMAT(thing, type, format) \
  ( __builtin_types_compatible_p (typeof (thing), type) ) {   \
    printf (format, thing);                                   \
  }                                                           \

// Acronym fun :)
#define TOTPUF THING_OF_TYPE_PRINT_USING_FORMAT

// Try to Print Thing (which must be of one of the known types).  This is
// somewhat adventurous code.  Note for example that the case for size_t
// never fires, because one of the integer type cases will happen first,
// which would make a difference if printf() used the format code for
// size_t ("%zi") to mean that the integer should be rendered differently.
// Pointers can be printed, but if they aren't already of void pointer type
// they must be explicitly cast to that type (or else a hook macro added
// to instrument_user_printf_extension_registration.h).  If the type isn't
// known an error message is produced and abort() is called.
//
// Other fun possibilities:
//
//   * output ints in hex (PRIx8, PRIx16 etc.).  But this is mainly a
//     microcontroller thing so really just clone and modify
//
//   * use a block-local (namespace-dirty) variable and run all the clauses to
//     detect and error out if two or more types match, rather than just the
//     in-comment warning about type synonyms and the size_t example
//
#define PT(thing)                                                         \
  do {                                                                    \
    _Pragma ("GCC diagnostic push");                                      \
    _Pragma ("GCC diagnostic ignored \"-Wformat\"");                      \
    if      TOTPUF ( thing , char            , "%c"                 )     \
    else if TOTPUF ( thing , wchar_t         , "%lc"                )     \
    else if TOTPUF ( thing , char *          , "%s"                 )     \
    else if TOTPUF ( thing , char const *    , "%s"                 )     \
    else if TOTPUF ( thing , char []         , "%s"                 )     \
    else if TOTPUF ( thing , wchar_t *       , "%ls"                )     \
    else if TOTPUF ( thing , wchar_t const * , "%ls"                )     \
    else if TOTPUF ( thing , wchar_t []      , "%s"                 )     \
    else if TOTPUF ( thing , int8_t          , "%" PRIi8            )     \
    else if TOTPUF ( thing , int16_t         , "%" PRIi16           )     \
    else if TOTPUF ( thing , int32_t         , "%" PRIi32           )     \
    else if TOTPUF ( thing , int64_t         , "%" PRIi64           )     \
    else if TOTPUF ( thing , uint8_t         , "%" PRIu8            )     \
    else if TOTPUF ( thing , uint16_t        , "%" PRIu16           )     \
    else if TOTPUF ( thing , uint32_t        , "%" PRIu32           )     \
    else if TOTPUF ( thing , uint64_t        , "%" PRIu64           )     \
    else if TOTPUF ( thing , float           , "%g"                 )     \
    else if TOTPUF ( thing , double          , "%g"                 )     \
    else if TOTPUF ( thing , long double     , "%g"                 )     \
    else if TOTPUF ( thing , void *          , "%p"                 )     \
    else if TOTPUF ( thing , size_t          , "I never happen %zi" )     \
    INSTRUMENT_PT_ADDITIONAL_ELSE_CLAUSES                                 \
    else {                                                                \
      printf ("\n");                                                      \
      fprintf (                                                           \
          stderr,                                                         \
          "%s:%i:%s: "                                                    \
          "don't know how to print things of type typeof (" #thing ")\n", \
          FLFT );                                                         \
      abort ();                                                           \
    }                                                                     \
    _Pragma ("GCC diagnostic pop");                                       \
  } while ( 0 )

// Trace Thing.  This just puts the source location and a newline around PT().
#define TT(thing)                \
  do {                           \
    printf ("%s:%i:%s: ", FLFT); \
    PT (thing);                  \
    printf ("\n");               \
  } while ( 0 );

#endif   // INSTRUMENT_FORMAT_FREE_PRINT_H
