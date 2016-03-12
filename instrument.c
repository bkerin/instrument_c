// Implementation of the interface described in instrument.h

#define _GNU_SOURCE

#include <assert.h>
#include <dlfcn.h>
#include <execinfo.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "instrument.h"

// FIXME: make a wrapper for the readlink() proc magic thingy

// FIXME: use this function
static int
file_open_tmp (char *template)
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
  strcpy (template, strcat (dir_part, template));

  // Fill in the "XXXXXX" part
  temp_file_fd = mkstemp (template);
  assert (temp_file_fd != -1);

  return temp_file_fd;   
}

char *
backtrace_with_line_numbers (void)
{
  char executable_name[PATH_MAX + 1];
  ssize_t bytes_read;
  int btrace_size;                 // Number of addresses
  int return_code;
  char temp_ba[PATH_MAX + 1] = "backtrace_addresses_XXXXXX";
  int temp_ba_fd;   // File Descriptor for temp_ba
  char temp_bt[PATH_MAX + 1] = "backtrace_text_XXXXXX";
  int temp_bt_fd;   // File Descriptor for temp_bt
  // FIXME: these next two are going away
  // Backtrace Addresses (temporary file, initialized to template)
  char ba[] = "/tmp/baXXXXXX";  
  // Most Recent Backtrace Text (temporary file, initialized to template)
  char bt[] = "/tmp/btXXXXXX";   // Most Recent Backtrace Text
  int tfd;     // Temporary File Descriptor (reused for different files)
  FILE *tfp;   // Trmporary FILE Pointer (reused for different things)
#define BT_MAX_STACK 242
  void *btrace_array[BT_MAX_STACK];   // Actual addressess
  int ii;      //  Index variable
#define ADDR2LINE_COMMAND_MAX_LENGTH 242
  char addr2line_command[ADDR2LINE_COMMAND_MAX_LENGTH + 1];
  int bytes_printed;
  struct stat stat_buf;
  char *result;

  // Use /proc magic to find which executable we really are
  bytes_read = readlink ("/proc/self/exe", executable_name, PATH_MAX + 1);
  assert (bytes_read != -1);
  assert (bytes_read <= PATH_MAX);      // Systems don't always honor PATH_MAX
  executable_name[bytes_read] = '\0';   // Readlink doesn't do this for us
  
  // Get the actual backtrace
  btrace_size = backtrace (btrace_array, BT_MAX_STACK);
  assert (btrace_size < BT_MAX_STACK);

  // Create full temp file names and temp files
  temp_ba_fd = file_open_tmp (temp_ba);
  return_code = close (temp_ba_fd);
  assert (return_code == 0);
  temp_bt_fd = file_open_tmp (temp_bt);
  return_code = close (temp_bt_fd);
  assert (return_code == 0);

  printf ("temp_ba: %s\n", temp_ba);
  printf ("temp_bt: %s\n", temp_bt);

  // FIXME: this next block is getting replaced
  // Create temp files (filling in their actual names), close file descriptors
  tfd = mkstemp (ba);
  assert (tfd != -1);
  return_code = close (tfd);
  assert (return_code == 0);
  tfd = mkstemp (bt);
  assert (tfd != -1);
  return_code = close (tfd);
  assert (return_code == 0);

  // Print the addresses to the address file
  tfp = fopen (ba, "w");
  assert (tfp != NULL);
  for ( ii = 1 ; ii < btrace_size ; ii++ ) {   // Skip 0 because that's us
    fprintf (tfp, "%p\n", btrace_array[ii]);
  } 
  return_code = fclose (tfp);
  assert (return_code == 0);

  // Run addr2line to convert addresses to show func, file, line
  bytes_printed
    = snprintf (
        addr2line_command,
        ADDR2LINE_COMMAND_MAX_LENGTH + 1,
        "addr2line --exe %s -f -i <%s >%s",
        executable_name,
        ba,
        bt );
  assert (bytes_printed <= ADDR2LINE_COMMAND_MAX_LENGTH);
  return_code = system (addr2line_command);
  assert (return_code == 0);

  // Get the size of the result
  return_code = stat (bt, &stat_buf);
  assert (return_code == 0);
  
  // Allocate storage for result
  result = malloc (stat_buf.st_size + 1);   // +1 for trailing null byte
  assert (result != NULL);

  // Read the func, file, line form back in
  tfp = fopen (bt, "r");
  assert (tfp != NULL);
  bytes_read = fread (result, 1, stat_buf.st_size, tfp);
  assert (bytes_read == stat_buf.st_size);
  result[stat_buf.st_size] = '\0';
  return_code = fclose (tfp);
  assert (return_code == 0);

  // Remove the temporary files
  return_code = unlink (bt);
  assert (return_code == 0);
  return_code = unlink (ba);
  assert (return_code == 0);

  return result;
}

void
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

  // Use /proc magic to find which executable we really are
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
  // required, but to be conservative and not lie to the user it's required.
  // FIXME: research this point
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
