
# This Makefile requires that the -R/--no-builtin-variables and
# -r/--no-builtin-variables options be used.  Implicit rules and default
# variables cause much more trouble and unreadability than they're worth.
ifeq ($(findstring r,$(MAKEFLAGS)),)
  $(error the -r/--no-builtin-rules make option is required)
endif
ifeq ($(findstring R,$(MAKEFLAGS)),)
   $(error the -R/--no-builtin-variables make option)
endif

# This is sensible stuff for use but could confuse an experienced Make
# programmer, so it's out front here.

# Delete files produced by rules the commands of which return non-zero.
.DELETE_ON_ERROR:

# Don't work with -j.  I seems to work but I haven't proven it.
.NOTPARALLEL:

# Enable a second expsion of variables in the prerequisite parts of rules.
# So $$(OBJS) will give us what we want if we have made a target- or
# pattern-local version of OBJS, for example.
.SECONDEXPANSION:

# Disable old-fashioned suffix rules.
.SUFFIXES:

# Avoid default goal confusion by essentially disabling default goals.
PRINT_DEFAULT_GOAL_TRAP_ERROR_MESSAGE =                                       \
  echo ;                                                                      \
  echo This build system doesn\'t support default goals.  Please explicitly ; \
  echo specify a target. ;                                                    \
  echo

.DEFAULT_GOAL = default_goal_trap
.PHONY: default_goal_trap
default_goal_trap:
	@($(PRINT_DEFAULT_GOAL_TRAP_ERROR_MESSAGE) && false) 1>&2
