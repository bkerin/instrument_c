#define _GNU_SOURCE

// FIXME: need a version of this file that is free-standing and can be
// built with just a single gcc command, to show how the non-shared-lib
// part of it works.

#include <assert.h>
#include <dlfcn.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "test.h"

// GCC options required for library *AND* client program compilation for
// stuff in this file to fully work:
//
//   -Wall -Wextra -g -fpic
//
// Ok -Wall might not be required, but you want it.  Use -Wextra too if
// possible.
//
// For client program compilation -O0 helps prevent the optimizer from
// confusing matters by inlining functions out of existence.  For shared
// libraries it shouldn't be needed since exposed symbols shouldn't be
// optimized away.  Because some compiler warnings don't happen with -O0,
// I like to compile with -O2 first as well.  With at least some gcc versions
// it's also possible for -O2 to mask warning or errors that show up with -O0.
// Just build it both ways and save yourself headaches.
//
// For linking of the client program -ldl is also required.
//
// For some shared or dynamically loaded library applications the
// -export-dynamic linker option is also required.

// FIXME: move these next three into the source library header

// Check Point: prints source location when hit
#define CP() printf ("%s:%i:%s: checkpoint\n", __FILE__, __LINE__, __func__)

// Trace Value: given var name and unquoted format code, print source
// location and var value when hit, e.g. TV (my_int, %i)
#define TV(var_name, format_code) TS (#var_name ": " #format_code, var_name)

// Trace Stuff: print expanded format string in first argument, using values
// given in remaining arguments, tagged with source location and added newline
#define TS(fmt, ...) \
  printf ("%s:%i:%s: " fmt "\n", __FILE__, __LINE__, __func__, ## __VA_ARGS__)

static void
what_func (void *func_addr)
{
  // I look long but I'm all simple stuff

  char executable_name[PATH_MAX + 1] = "";
  ssize_t bytes_read;
  int bytes_printed;
#define SYSTEM_COMMAND_MAX_LENGTH 442
  char system_command[SYSTEM_COMMAND_MAX_LENGTH + 1] = "";
  char temp_file[PATH_MAX + 42] = "";  // 42 > strlen("/what_func_XXXXXX")
  int temp_file_fd;
  int return_code;
  char *tmpdir_ev = NULL;   // TMPDIR Environment Variable value
  bool executable_not_stripped, library_not_stripped;
  bool exact_address_match;

  // Use /proc magic to find which binary we really are
  bytes_read = readlink ("/proc/self/exe", executable_name, PATH_MAX + 1);
  assert (bytes_read != -1);
  assert (bytes_read <= PATH_MAX);      // Systems don't always honor PATH_MAX
  executable_name[bytes_read] = '\0';   // Readlink doesn't do this for us

  // If the executable has been stripped, abort().  The user should recompile.
  bytes_printed
    = snprintf (
        system_command,
        SYSTEM_COMMAND_MAX_LENGTH + 1,
        "file %s | grep --quiet 'not stripped$'",
        executable_name );
  assert (bytes_printed <= SYSTEM_COMMAND_MAX_LENGTH);
  // Careful: testing for grep no-match return code needs WEXITSTATUS()
  executable_not_stripped = ! system (system_command);
  assert (executable_not_stripped);

  // Figure out appropriate temporary directory name to use
  tmpdir_ev = getenv ("TMPDIR");
  if ( tmpdir_ev != NULL ) {
    assert (strlen (tmpdir_ev) <= PATH_MAX);   // PATH_MAX isn't dependable
    strcpy (temp_file, tmpdir_ev);
  }
  if ( strlen (temp_file) == 0 ) {
#ifdef P_tmpdir
    strcpy (temp_file, P_tmpdir);
#endif
  }
  if ( strlen (temp_file) == 0 ) {
    strcpy (temp_file, "/tmp");
  }

  // Fill out the rest of the temporary file name
  temp_file_fd = mkstemp (strcat (temp_file, "/what_func_XXXXXX"));
  if ( temp_file_fd == -1 ) {
    perror ("mkstemp failed");
  }
  assert (temp_file_fd != -1);
  return_code = close (temp_file_fd);
  assert (return_code == 0);

  // FIXME: move these demos of how to print individual values somewhere else
  CP ();
  TV (executable_not_stripped, %i);
  TS ("executable_not_stripped: %i", executable_not_stripped);

  // Assemble and run the nm-based command to look up the function name and
  // location in the executable.
  bytes_printed
    = snprintf (
        system_command,
        SYSTEM_COMMAND_MAX_LENGTH + 1,
        "nm --line-numbers --print-file-name --format=posix %s | "
        "grep `echo %p | perl -p -e 's/^0x//'` | "
        "perl -p -e 's/\\s+/ /g' | "
        "cut --delimiter=' ' --fields=2,6 | "
        "tee %s",
        executable_name,
        func_addr,
        temp_file );
  assert (bytes_printed <= SYSTEM_COMMAND_MAX_LENGTH);
  return_code = system (system_command);
  // Careful: testing for grep no-match return code needs WEXITSTATUS()
  assert (return_code == 0);
 
  // If we found a match using nm on the executable, we're done.
  bytes_printed
    = snprintf (
        system_command,
        SYSTEM_COMMAND_MAX_LENGTH + 1,
        "test -z \"`cat %s`\"",
        temp_file );
  assert (bytes_printed <= SYSTEM_COMMAND_MAX_LENGTH);
  if ( system (system_command) != 0 ) {
    return_code = unlink (temp_file);
    assert (return_code == 0);
    return;
  }

  // Try using dladdr() to find the function in a shared library.
  printf (
      "Didn't find a function with that address in executable, will now try\n"
      "dladdr().  See the caveats in the dladdr() man page.\n" );
  Dl_info dl_info;
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
  
  // If the library has been stripped, abort().  Maybe the user can recompile.
  bytes_printed
    = snprintf (
        system_command,
        SYSTEM_COMMAND_MAX_LENGTH + 1,
        "file `realpath %s` | grep --quiet 'not stripped$'",
        dl_info.dli_fname );
  assert (bytes_printed <= SYSTEM_COMMAND_MAX_LENGTH);
  // Careful: testing for grep no-match return code needs WEXITSTATUS()
  library_not_stripped = ! system (system_command);
  assert (library_not_stripped);

  // If we found an exact match for the given address, assemble and run the
  // nm-based command to look up the function location in the source file for
  // shared library.  Note that I'm not actually sure an exact match is even
  // requires, but to be conservative and not lie to the user it's required.
  if ( exact_address_match ) {
    bytes_printed
      = snprintf (
          system_command,
          SYSTEM_COMMAND_MAX_LENGTH + 1,
          "echo -n \"  Possible function and source location:\n    \" && "
          "nm --line-numbers --print-file-name --format=posix %s | "
          "grep ': %s ' | "
          "perl -p -e 's/\\s+/ /g' | "
          "cut --delimiter=' ' --fields=2,6 | "
          "tee %s",
          dl_info.dli_fname,
          dl_info.dli_sname,
          temp_file );
    assert (bytes_printed <= SYSTEM_COMMAND_MAX_LENGTH);
    return_code = system (system_command);
    // Careful: testing for grep no-match return code needs WEXITSTATUS()
    assert (return_code == 0);
  }
    
  return_code = unlink (temp_file);
  assert (return_code == 0);
  return;
}

void
basic_func (void);

void
basic_func (void)
{
  printf ("in function %s\n", __func__);
}

static void
static_func (void)
{
  printf ("in function %s\n", __func__);
}

int
main (void)
{
  printf ("\n");

  // Announce ourselves and call the functions being tested
  printf ("I'm a client program\n");
  basic_func ();
  static_func ();
  shared_lib_func ();
  printf ("\n");

  printf ("Looking up name for function basic_func()...\n");
  what_func (basic_func);
  printf ("\n");

  // Note that the optimizer tends to inline static functions especially often
  printf ("Looking up name for static function static_func()...\n");
  what_func (static_func);
  printf ("\n");

  printf ("Looking up name for shared library function shared_lib_func()...\n");
  what_func (shared_lib_func);
  printf ("\n");

  return 0;
}
