// Support for debugging using instrumentation
//
// Certain GCC compilation and linking options should be used:
//
//   * -D_GNU_SOURCE is required at compile time (because GNU libc extensions
//     are required).
//
//   * -g is required at compile-time for backtraces and what_func() (and you
//     must not strip binaries or libraries later)
//
//   * -ldl is always required at link-time because dladdr() needs it, if
//     this annoys you and you don't care about looking up functions in
//     shared libs chop out or edit what_func()
//
//   * -O0 can simplify life by preventing the optimizer from inlining
//     functions out of existance.  It's probably not an issue for shared
//     libraries because exported symbols should be safe.  I like to compile
//     twice, once with -O2 and once with -O0, since some useful warnings
//     only fire with one or the other (on gcc at least).
//
//   * For what_func() to work right when looking up pointers to functions in
//     shared librares, -fPIC is required for library *AND* client
//     compilation.  FIXME: it's only needed for clients because of dladdr()
//     bugs, if dladdr() gets fixed this requirement can be removed
//
//   * -Wall, -Wextra, -Wformat-signedness, and -Werror aren't required but
//     they make life better.  In my old age I've come to believe in
//     -Wconversion as well (after debugging signed*unsigned bugs created by
//     the best C programmer I've ever seen, https://github.com/ldeniau).
//     The C integer type promotion rules are so counter-intuitive with
//     respect to the containment of the sets they model that latent bugs
//     preventable by -Wconversion are guaranteed to happen eventually.

#ifndef INSTRUMENT_H
#define INSTRUMENT_H

// Note that _GNU_SOURCE must be defined when the headers providing the
// extensions we need are included for the *first* time.  This means using the
// -D_GNU_SOURCE gcc option or maybe config.h (if it's always included first).
#ifndef _GNU_SOURCE
#  error GNU libc extensions are required but _GNU_SOURCE is not defined
#endif

#include <assert.h>
#include <dlfcn.h>
#include <errno.h>
#include <execinfo.h>
#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// The stuff in this header is the best way to instrument code to see values.
// IMPROVEME: should we change format_free_print.h to use only C11 _Generic
// it could then be included here unconditionally.
#ifdef __GNUC__
#  include "format_free_print.h"
#endif

// File-Line-Function Tuple
#define FLFT __FILE__, __LINE__, __func__

// Check Point: prints source location followed by a newline
#define CP() printf ("%s:%i:%s: checkpoint\n", FLFT)

// Die Point.  Change a CP() to this when you don't want to see more after it.
#define DP()                                                   \
  do {                                                         \
    printf ("%s:%i:%s: is a die point, will now die\n", FLFT); \
    exit (EXIT_FAILURE);                                       \
  } while ( 0 )

// If for some reason format_free_print.h doesn't do what you need these
// next three might be useful:

// Trace Value: given an expression expr and an unquoted format code, print
// the source location and expression text and value followed by a newline,
// e.g. TV (my_int, %i), TV (my_sub_returning_int (), %i).
#define TV(expr, format_code)                                    \
  printf ("%s:%i:%s: " #expr ": " #format_code "\n", FLFT, expr)

// Like TV(), but die after.  To easily avoid seeing additional garbage output.
#define TVD(expr, format_code) \
  do {                         \
    TV (expr, format_code);    \
    exit (EXIT_FAILURE);       \
  } while ( 0 )

// Trace Stuff: print expanded format string in first argument, using values
// given in remaining arguments, tagged with source location and added newline
#ifdef __GNUC__   // This one needs GNU comma-swallowing __VA_ARGS__ extension
#  define TS(fmt, ...) printf ("%s:%i:%s: " fmt "\n", FLFT, ## __VA_ARGS__)
#endif

// Like TS() then Die.  To easily avoid seeing additional garbage output.
#ifdef __GNUC__   // This one needs GNU comma-swallowing __VA_ARGS__ extension
#  define TSD(fmt, ...)       \
     do {                     \
       TS (fmt, __VA_ARGS__); \
       exit (EXIT_FAILURE);   \
     } while ( 0 )
#endif

static void
get_executable_name (char *executable_name)
{
  // Set the string executable_name to the name of the current executable,
  // as determined by using readlink() on the "/proc/self/exe" magical link.
  // The executable_name pointer must point to PATH_MAX + 1 or more bytes
  // of memory.

  ssize_t bytes_read;

  // IMPROVEME: is there a more portable way to do this?
  bytes_read = readlink ("/proc/self/exe", executable_name, PATH_MAX + 1);
  assert (bytes_read != -1);
  assert (bytes_read <= PATH_MAX);      // Systems don't always honor PATH_MAX
  executable_name[bytes_read] = '\0';   // readlink() doesn't do this for us
}

static int
open_tmp_file (char *template)
{
  // Open a new temporary file in our best guess at the system temporary
  // directory.  The template must contain the file base name with "XXXXXX"
  // section as used by mkstemp().  It is modified to include the directory
  // part and replace the "XXXXXX" string.  It is assumed it has length at
  // least PATH_MAX + 1.  The file descriptor of the open file is returned.
  // On error an assertion violation is triggered.

  char dir_part[PATH_MAX + 1] = "";
  char *tmpdir_ev = NULL;   // TMPDIR Environment Variable value
  int temp_file_fd;

  // Figure out appropriate temporary directory name to use
  tmpdir_ev = getenv ("TMPDIR");
  if ( tmpdir_ev != NULL ) {
    assert (strlen (tmpdir_ev) <= PATH_MAX);   // PATH_MAX isn't dependable
    strcpy (dir_part, tmpdir_ev);
  }
  if ( strlen (dir_part) == 0 ) {
#ifdef P_tmpdir
    strcpy (dir_part, P_tmpdir);
#endif
  }
  if ( strlen (dir_part) == 0 ) {
    strcpy (dir_part, "/tmp");
  }

  // Assemble the full template path (still with "XXXXXX" at this stage)
  assert (strlen (dir_part) + strlen (template) <= PATH_MAX);
  strcpy (template, strcat (strcat (dir_part, "/"), template));

  // Fill in the "XXXXXX" part
  temp_file_fd = mkstemp (template);
  assert (temp_file_fd != -1);

  return temp_file_fd;
}

#ifdef __GNUC__
#  define MAYBE_PRINTF_ATTRIBUTE(format_pos, first_other_pos) \
    __attribute__ ((format (printf, format_pos, first_other_pos)))
#else
#  define MAYBE_PRINTF_ATTRIBUTE(format, first_other)
#endif

#define EXPANDED_SYSTEM_COMMAND_MAX_LENGTH 4042


MAYBE_PRINTF_ATTRIBUTE (1, 2)
static int
expand_system (char const *format, ...)
{
  // Expand the string in format a la snprintf(), then pass it to system()
  // and return the result of that function.  If the expanded string would
  // come out longer than EXPANDED_SYSTEM_COMMAND_MAX_LENGTH an assertion
  // violation is triggered.

  char ec[EXPANDED_SYSTEM_COMMAND_MAX_LENGTH + 1];
  int bytes_printed;

  va_list ap;
  va_start (ap, format);
  bytes_printed
    = vsnprintf (ec, EXPANDED_SYSTEM_COMMAND_MAX_LENGTH + 1, format, ap);
  va_end (ap);

  assert (bytes_printed <= EXPANDED_SYSTEM_COMMAND_MAX_LENGTH);

  return system (ec);
}

// Because we will be converting from off_t to size_t:
#if (__STDC_VERSION__ >= 201112L)
_Static_assert (
    sizeof(off_t) <= sizeof(size_t), "off_t larger than expected");
#endif

static void *
get_base_address (void)
{
  // Get the base address for the current process from the (kernel-maintained)
  // /proc/self/maps.  This is necessary to handle ASLR which is usually
  // enabled by default nowadays, and also to take care of the fixed offset
  // that generally exists even when ASLR is turned off.

  FILE *map_file;   // Map File
  void *result;
  int match_count, return_code;
  char *line = NULL;
  size_t line_len;
  ssize_t chars_read;
  char *map_file_file_name;   // Rename
  char exe_name[PATH_MAX + 1];    // Executable Name (name of calling program)

  map_file = fopen ("/proc/self/maps", "r");
  assert (map_file != NULL);
  // The first number in the first range in the file is the one we want
  match_count = fscanf (map_file, "%p-", &result);
  assert (match_count == 1);

  // Everything that follows in this function is to look at the file name
  // that's also listed on the first line of the file, and make sure it's
  // the name of executable file as we expect.

  chars_read = getline (&line, &line_len, map_file);
  assert (chars_read != -1);

  return_code = fclose (map_file);
  assert (return_code == 0);

  // map_file_file_name is actually a pointer into line so there's still
  // a following '\n' at least.
  map_file_file_name = strstr (line, "/");
  assert (map_file_file_name != NULL);
  assert (line != NULL);

  get_executable_name (exe_name);   // What executable are we really?

  assert (strncmp (map_file_file_name, exe_name, strlen (exe_name)) == 0);

  free (line);

  return result;
}

// Convert real_address to an address relative to base_address
#define REAL2RELATIVE_ADDR(real_address, base_address)             \
  ((void *) ((uint8_t *) real_address - (uint8_t *) base_address))

// Get a new string containing a backtrace for the call point.  This is
// done using the GNU libc backtrace() function and the addr2line program.
// See the notes above about required compiler options.  On error an
// assertion violation is triggered.
//
// Caveats:
//
//   * Stack unwinding involves heuristics.  Callers can do things that defeat
//     it.  If it looks like this routine is lying to you it probably is.
//
//   * This function doesn't make any effort to backtrace through separate
//     (shared or dynamically loaded) libraries.  FIXME: In theory it could,
//     either by using what_func on all the frames or maybe by taking
//     advantage of all the info in the map file as described in another
//     fixme in this file, either way -rdynamic would then likely be required
//     (though it might be required anyway per the current backtrace() docs)
//
//     in which case -rdynamic would need to be used when compiling
//     shared libs, and since what_func() would be used per-frame its
//     requirements would need to be met as well.
//
//   * Use from signal handlers probably doesn't work.
//
static char *
backtrace_with_line_numbers (void)
{
  char exe_name[PATH_MAX + 1];    // Executable Name (name of calling program)
  int btrace_size;                // Number of addresses
#define BT_MAX_STACK 242
  void *btrace_array[BT_MAX_STACK];   // Actual addressess
  int return_code;   // Return code for functions and executed programs
  char tfba[PATH_MAX + 1] = "";   // Temporary file Backtrace Addresses
  char tfbt[PATH_MAX + 1] = "";   // Temporary file Backtrace Text
  int tfd;     // Temporary File Descriptor (reused for different files)
  FILE *tfp;   // Trmporary FILE Pointer (reused for different things)
  unsigned int ii;   // Index variable
  // FIXXME: is this "base address" term that we use here and elsewhere reallyh
  // the most correct and descriptive name?
  void *base_address;   // Base address of the process
  void *call_site_with_offset;   // Address from which func called with offset
  void *call_site;               // Address from which func called w/o offset
  struct stat stat_buf;
  size_t bytes_read;
  char *uresult, *result;     // Unbeautified result, result to be returned
  char *soml, *sonl, *sonnl;   // Start of main()/Next/Next Next Line
  unsigned int line_number;   // Line number of backtrace (for beautification)
  size_t cci;                 // Current Character Index

  get_executable_name (exe_name);   // What executable are we really?

  // Get the actual backtrace
  btrace_size = backtrace (btrace_array, BT_MAX_STACK);
  assert (btrace_size < BT_MAX_STACK);

  base_address = get_base_address ();

  // Create full temp file names and temp files
  strcpy (tfba, "backtrace_addresses_XXXXXX");
  tfd = open_tmp_file (tfba);
  return_code = close (tfd);
  assert (return_code == 0);
  strcpy (tfbt, "backtrace_text_XXXXXX");
  tfd = open_tmp_file (tfbt);
  return_code = close (tfd);
  assert (return_code == 0);

  // Print the addresses to the address file
  tfp = fopen (tfba, "w");
  assert (tfp != NULL);
  assert (btrace_size > 0);
  // Skip 0 because that's us:
  for ( ii = 1 ; ii < (unsigned int) btrace_size ; ii++ ) {
    // - 1 because we want the call site, not the return address.  This is a
    // heuristic and the caller could defeat it with a synthesized address.
    call_site_with_offset = ((void *) ((uintptr_t) btrace_array[ii] - 1));
    call_site = REAL2RELATIVE_ADDR (call_site_with_offset, base_address);
    //fprintf (tfp, "%p\n", ((void *) ((uintptr_t) btrace_array[ii] - 1)));
    fprintf (tfp, "%p\n", call_site);
  }
  return_code = fclose (tfp);
  assert (return_code == 0);

  // FIXME: Instead of just using addr2line on all the addresses, we could use
  // what_func() on each of them to try to work over shared libs and such.
  // Or maybe if we look at the entire map file and use addr2line with the
  // correct file name as listed in the map file addr2line can sort things
  // out correctly these days?
  return_code
    = expand_system ("addr2line -e %s -f -i <%s >%s", exe_name, tfba, tfbt);
  assert (return_code == 0);

  // Get the size of the result
  return_code = stat (tfbt, &stat_buf);
  assert (return_code == 0);

  // Allocate storage for Unbeautified Result and actual result
  assert (stat_buf.st_size >= 0);
  uresult = malloc ((size_t) stat_buf.st_size + 1);   // +1 for trailing null
  assert (uresult != NULL);
  uresult[0] = '\0';
  // We're going to add some spaces and a line indicating that we're snipping
  // off some magic function frames, to keep life simple just use 2 * space + 80.
  // To make sure this string is always null-byte terminated as we grow it we
  // use calloc and double check that null bytes are actully zeros :)
  assert ((char) 0 == '\0');
  assert (stat_buf.st_size >= 0);
  // +1 for trailing null:
  assert ((size_t) stat_buf.st_size <= SIZE_MAX / 2 + 80 + 1);
  result = calloc (1, 2 * (size_t) stat_buf.st_size + 80 + 1);
  assert (result != NULL);
  result[0] = '\0';

  // Read the func, file, line form back in
  tfp = fopen (tfbt, "r");
  assert (tfp != NULL);
  assert (stat_buf.st_size >= 0);
  bytes_read = fread (uresult, 1, (size_t) stat_buf.st_size, tfp);
  assert (bytes_read == (size_t) stat_buf.st_size);
  result[stat_buf.st_size] = '\0';
  return_code = fclose (tfp);
  assert (return_code == 0);

  // Slight beautification: indent the location lines (every other line)
  line_number = 0;
  for ( cci = 0 ; cci < bytes_read ; cci++ ) {
    if ( uresult[cci] == '\n' ) {
      line_number++;
      if ( line_number % 2 == 1 ) {
        strcat (result, "\n  ");
        continue;
      }
    }
    result[cci + 2 * ((line_number + 1) / 2)] = uresult[cci];
  }

  // Trim off callers above main() since they are generally magical hidden
  // stuff that's not interesting to users and we aren't set up to deal with
  // them, and replace them with a message.  FIXXME: the map file does look
  // like it has all the info we would need to sort these out, but we would
  // have to parse a lot more of it to get the file names and different base
  // addresses of e.g. /usr/lib/x86_64-linux-gnu/libc.so.6 just to report
  // e.g. __libc_start_main which doesn't seem very worthwhile.
  soml = strstr (result, "main\n");   // Start Of main() Line
  assert (soml != NULL);
  while (! ((soml == result) || (*(soml - 1) == '\n')) ) {
    soml = strstr (soml, "main\n");
    assert (soml != NULL);
  }
  sonl = strstr (soml + 1, "\n") + 1;   // Start Of Next Line
  sonnl = strstr (sonl, "\n") + 1;
  sprintf (
      sonnl,
      "[Magical callers above main() (e.g. _libc_statrt_main) not shown]\n");

  free (uresult);

  // Remove the temporary files
  return_code = unlink (tfbt);
  assert (return_code == 0);
  return_code = unlink (tfba);
  assert (return_code == 0);

  return result;
}

// Convenience wrapper that prints "Backtrace:\n" and the result of
// backtrace_with_line_numbers() to stderr.
#define BACKTRACE()                                       \
  do {                                                    \
    char *XxX_backtrace = backtrace_with_line_numbers (); \
    fprintf (stderr, "Backtrace:\n%s", XxX_backtrace);    \
    free (XxX_backtrace);                                 \
  } while (0)

// Like assert(), but prints a full backtrace (if NDEBUG isn't defined).
// All caveats of backtrace_with_line_numbers() apply.  */
#ifndef NDEBUG

// Use GCC branch-predicting test for assertions if possible
#  ifdef __GNUC__
#    define INSTRUMENT_MAYBE_EXPECT_FALSE(cond) (__builtin_expect ((cond), 0))
#  else
#    define INSTRUMENT_MAYBE_EXPECT_FALSE(cond) (cond)
#  endif

#  define ASSERT_BT(cond)                                                   \
    do {                                                                    \
      if ( INSTRUMENT_MAYBE_EXPECT_FALSE (!(cond)) ) {                      \
        fprintf (                                                           \
            stderr,                                                         \
            "%s: %s:%i:%s: Assertion ` " #cond "' failed.  Backtrace:\n%s", \
            program_invocation_short_name,                                  \
            FLFT,                                                           \
            backtrace_with_line_numbers() );                                \
        abort ();                                                           \
      }                                                                     \
    } while ( 0 )

#else

#  define ASSERT_BT(COND)

#endif

// Print to stdout a best guess about the function name and source code
// location corresponding to func_addr.  See the notes above about required
// compiler options.  First an attempt is made to look up the address
// in the current binary using the nm program, and if that doesn't find
// anything dladdr() is used to find the correct shared library and nm is
// used on that.  If things appear to have been compiled or stripped such
// that this function cannot succeed an assertion violation is triggered.
// On error an assertion violation is triggered.
__attribute__ ((__unused__))
static void
what_func_func (void *func_addr)
{
  // I look long but I'm all simple stuff

  char exe_name[PATH_MAX + 1] = "";    // Name of current executable file
  char temp_file[PATH_MAX + 42] = "";  // 42 > strlen("/what_func_XXXXXX")
  int temp_file_fd;
  int return_code;
  bool executable_not_stripped, library_not_stripped;
  bool exact_address_match;
  char real_path_dli_fname[PATH_MAX], real_path_executable[PATH_MAX];
  char *real_path_return;
  bool dli_fname_is_not_executable;
  Dl_info dl_info;
  void *base_address;

  base_address = get_base_address ();

  get_executable_name (exe_name);   // What executable are we really?

  // If the executable has been stripped, abort().  The user should recompile.
  // Careful: testing for grep no-match return code would need WEXITSTATUS()
  executable_not_stripped
    = !(expand_system ("file %s | grep --quiet 'not stripped$'", exe_name));
  assert (executable_not_stripped);

  // Create new temporary file and close it's file descriptor
  strcpy (temp_file, "what_func_func_XXXXXX");
  temp_file_fd = open_tmp_file (temp_file);
  return_code = close (temp_file_fd);
  assert (return_code == 0);

  // FIXME: I think we would do better to use addr2line here, since it's
  // entire purpose is to do this sort of thing (while nm and readelf,
  // both of which also seem to be able to get us what we want, have other
  // primary purposes).
  // Assemble and run the nm-based command to look up the function name and
  // location in the executable.
  return_code
    = expand_system (
        "nm --line-numbers --print-file-name --format=posix %s | "
        "grep `echo %p | perl -p -e 's/^0x//'` | "
        "perl -p -e 's/\\s+/ /g' | "
        "cut --delimiter=' ' --fields=2,6 | "
        "tee %s",
        exe_name,
        REAL2RELATIVE_ADDR (func_addr, base_address),
        temp_file );
  // Careful: testing for grep no-match return code needs WEXITSTATUS()
  // FIXME: this is only testing that tee worked, could do better with
  // set -o pipefail on bash at least
  assert (return_code == 0);

  // If we found a match using nm on the executable, we're done.
  if ( expand_system ("test -z \"`cat %s`\"", temp_file) != 0 ) {
    return_code = unlink (temp_file);
    assert (return_code == 0);
    return;
  }

  // Try using dladdr() to find the function in a shared library.
  // FIXME: there's also dladdr1() in recent GNU libc that can give a lot
  // more information about the found symbol or file, and therefore could
  // probably be made to do a better job reporting on anything found.
  printf (
      "Didn't find a function with that address in executable, will now try\n"
      "dladdr().  See the caveats in the dladdr() man page.\n" );
  return_code = dladdr (func_addr, &dl_info);
  assert (return_code != 0);
  exact_address_match = (dl_info.dli_saddr == func_addr);
  printf (
      "  dli_fname: %s\n"
      "  dli_fbase: %p\n"
      "  dli_sname: %s\n"
      "  dli_saddr: %p (%sexact address match)\n",
      dl_info.dli_fname,
      dl_info.dli_fbase,
      dl_info.dli_sname,
      dl_info.dli_saddr,
      exact_address_match ? "" : "not " );

  // Check that the file name returned by dladdr() doesn't point back to the
  // executable itself.  This can happen when client code is compiled without
  // -fPIC: see the dladdr() man page.
  real_path_return  = realpath (dl_info.dli_fname, real_path_dli_fname);
  assert (real_path_return != NULL);
  real_path_return  = realpath (exe_name, real_path_executable);
  assert (real_path_return != NULL);
  dli_fname_is_not_executable
    = strcmp (real_path_dli_fname, real_path_executable);
  assert (dli_fname_is_not_executable);

  // If the library is stripped we're done.  Maybe the user can recompile.
  return_code
    = expand_system (
        "file `realpath %s` | grep --quiet 'not stripped$'",
        dl_info.dli_fname );
  library_not_stripped = ! return_code;
  assert (library_not_stripped);

  // If we found an exact match for the given address, assemble and run the
  // nm-based command to look up the function location in the source file
  // for the shared library.
  if ( exact_address_match ) {
    return_code
      = expand_system (
          "echo -n \"  Possible function and source location:\n    \" && "
          "nm --line-numbers --print-file-name --format=posix %s | "
          "grep ': %s ' | "
          "perl -p -e 's/\\s+/ /g' | "
          "cut --delimiter=' ' --fields=2,6 | "
          "tee %s",
          dl_info.dli_fname,
          dl_info.dli_sname,
          temp_file );
    // Careful: testing for grep no-match return code needs WEXITSTATUS()
    assert (return_code == 0);
  }
  else {
    // FIXME: are there cases in which we don't get an exact match, but the
    // reported name is right?  I'm uncertain on this even after asking on
    // the libc mailing list, so for now we're being paranoid.
    printf (
        "  No exact address match: function location unknown, and the\n"
        "  reported name might be wrong.");
  }

  return_code = unlink (temp_file);
  assert (return_code == 0);

  return;
}

// Because what_func_func doesn't work with -Wpedantic because function
// pointer cast to void *. IMPROVEME: equivalent workaround for e.g. clang
// equivalt of -Wpedantic, if any?
#ifdef __GNUC__
  #define what_func(func_addr)                           \
    do {                                                 \
      _Pragma ("GCC diagnostic push");                   \
      _Pragma ("GCC diagnostic ignored \"-Wpedantic\""); \
      what_func_func (func_addr);                        \
      _Pragma ("GCC diagnostic pop");                    \
    } while ( 0 )
#else
  #define what_func(func_addr)                           \
    do {                                                 \
      what_func_func (func_addr);                        \
    } while ( 0 )
#endif


#endif   // INSTRUMENT_H
