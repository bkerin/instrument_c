
# Variables and targets for producing call and callee graphs using GNU
# cflow, and an HTML viewer for those graphs and the other functions and
# sources using GNU global.  This file is meant to be included from the
# main Makefile which defines the sources, cpp flags etc.

# GNU global expects posix format.  The rest of these avoid warning from gcc.
CFLOW_FLAGS = --format=posix                   \
              --symbol __inline:=inline        \
              --symbol __inline__:=inline      \
              --symbol __const__:=const        \
              --symbol __const:=const          \
              --symbol __restrict:=restrict    \
              --symbol __extension__:qualifier \
              --symbol __attribute__:wrapper   \
              --symbol __asm__:wrapper         \
              --symbol __nonnull:wrapper       \
              --symbol __wur:wrapper

call_graph callee_graph: $(SOURCES) $(HEADERS) Makefile
	# See the cflow manual for a more complete make/automake approach
	cflow $(if $(filter callee_graph,$@),--reverse) \
              $(CFLOW_FLAGS)                            \
              $(CPPFLAGS)                               \
              $(SOURCES)                                \
              -o $@

# The point of this target is htags.HTML dir, because it's a dir we use a stamp
htags.HTML.stamp: call_graph callee_graph
	htags --version || (echo This recipe requires GNU global 2>&1 && false)
	rm -f $@
	rm -rf $(patsubst %.stamp,%,$@)
	# Some might like --tree-view=treeview-red and maybe --frame as well
	htags --suggest --call-tree=call_graph --callee-tree=callee_graph
	mv HTML $(patsubst %.stamp,%,$@)
	touch $@

CFLOW_AND_GLOBAL_CLEANFILES = \
  call_graph callee_graph HTML htags.HTML htags.HTML.stamp GPATH GRTAGS GTAGS
