// Implementation of the interface described in instrument.h

#include <assert.h>
#include <dlfcn.h>
#include <execinfo.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "instrument.h"

static void
get_executable_name (char *executable_name)
{
  // Set the string executable_name to the name of the current executable,
  // as determined by using readlink() on the "/proc/self/exe" magical link.
  // The executable_name pointer must point to PATH_MAX + 1 or more bytes
  // of memory.

  ssize_t bytes_read; 

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
    __attribute__ ((format (printf, format_pos, first_other_pos)));
#else
#  define MAYBE_PRINTF_ATTRIBUTE(format, first_other)
#endif

#define EXPANDED_SYSTEM_COMMAND_MAX_LENGTH 4042

// Expand the string in format a la snprintf(), then pass it to system()
// and return the result of that function.  If the expanded string would
// come out longer than EXPANDED_SYSTEM_COMMAND_MAX_LENGTH an assertion
// violation is triggered.
int
expand_system (char const *format, ...) MAYBE_PRINTF_ATTRIBUTE (1, 2);

int
expand_system (char const *format, ...)
{
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

char *
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
  int ii;      // Index variable
  struct stat stat_buf;
  ssize_t bytes_read;
  char *uresult, *result;   // Unbeautified result, result to be returned
  int line_number;          // Line number of backtrace (for beautification)
  ssize_t cci;              // Current Character Index

  get_executable_name (exe_name);   // What executable are we really?
  
  // Get the actual backtrace
  btrace_size = backtrace (btrace_array, BT_MAX_STACK);
  assert (btrace_size < BT_MAX_STACK);

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
  for ( ii = 1 ; ii < btrace_size ; ii++ ) {   // Skip 0 because that's us
    // - 1 because we want the call site, not the return address.  This is a 
    // heuristic and the caller could defeat it with a synthesized address.
    fprintf (tfp, "%p\n", btrace_array[ii] - 1);
  } 
  return_code = fclose (tfp);
  assert (return_code == 0);

  // FIXMELATER: Instead of just using addr2line on all the addresses, we
  // could use what_func() on each of them to try to work over shared libs
  // and such.
  return_code
    = expand_system ("addr2line -e %s -f -i <%s >%s", exe_name, tfba, tfbt);
  assert (return_code == 0);

  // Get the size of the result
  return_code = stat (tfbt, &stat_buf);
  assert (return_code == 0);
  
  // Allocate storage for Unbeautified Result and actual result
  uresult = malloc (stat_buf.st_size + 1);   // +1 for trailing null byte
  assert (uresult != NULL);
  uresult[0] = '\0';
  // We're going to add some spaces, to keep life simple just use 2 * space.
  // To make sure this string is always null-byte terminated as we grow it we
  // use calloc and double check that null bytes are actully zeros :)
  assert ((char) 0 == '\0');
  result = calloc (1, 2 * stat_buf.st_size + 1);   // +1 for trailing null byte
  assert (result != NULL);
  result[0] = '\0';

  // Read the func, file, line form back in
  tfp = fopen (tfbt, "r");
  assert (tfp != NULL);
  bytes_read = fread (uresult, 1, stat_buf.st_size, tfp);
  assert (bytes_read == stat_buf.st_size);
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

  free (uresult);

  // Remove the temporary files
  return_code = unlink (tfbt);
  assert (return_code == 0);
  return_code = unlink (tfba);
  assert (return_code == 0);

  return result;
}

void
what_func (void *func_addr)
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

  get_executable_name (exe_name);   // What executable are we really?

  // If the executable has been stripped, abort().  The user should recompile.
  // Careful: testing for grep no-match return code would need WEXITSTATUS()
  executable_not_stripped
    = !(expand_system ("file %s | grep --quiet 'not stripped$'", exe_name));
  assert (executable_not_stripped);

  // Create new temporary file and close it's file descriptor
  strcpy (temp_file, "what_func_XXXXXX");
  temp_file_fd = open_tmp_file (temp_file);
  return_code = close (temp_file_fd);
  assert (return_code == 0);

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
        func_addr,
        temp_file );
  // Careful: testing for grep no-match return code needs WEXITSTATUS()
  assert (return_code == 0);
 
  // If we found a match using nm on the executable, we're done.
  if ( expand_system ("test -z \"`cat %s`\"", temp_file) != 0 ) {
    return_code = unlink (temp_file);
    assert (return_code == 0);
    return;
  }

  // Try using dladdr() to find the function in a shared library.
  // FIXMELATER: there's also dladdr1() in recent GNU libc that can give a lot
  // more information about the found symbol or file, and therefore could
  // probably be made to do a better job reporting on anything found.
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
    // FIXMELATER: are there cases in which we don't get an exact match,
    // but the reported name is right?  I'm uncertain on this even after
    // asking on the libc mailing list, so for now we're being paranoid.
    printf (
        "  No exact address match: function location unknown, and the\n"
        "  reported name might be wrong.");
  }

  return_code = unlink (temp_file);
  assert (return_code == 0);

  return;
}
