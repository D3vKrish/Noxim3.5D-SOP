##### LIBRARIES CONFIGURATION #####

SYSTEMC := $(SYSTEMC_HOME)
YAML    := $(YAML_HOME)


##### DIRECTORIES #####

SYSTEMC_LIBS := $(wildcard $(SYSTEMC)/lib-*)

SRCDIR  := ../src
SUBDIRS := $(filter %/,$(wildcard $(SRCDIR)/*/))
OBJDIR  := ./build

SRCS := $(wildcard $(SRCDIR)/*.cpp) $(wildcard $(addsuffix *.cpp,$(SUBDIRS)))
OBJS := $(subst $(SRCDIR),$(OBJDIR),$(SRCS:.cpp=.o))

MODULE := noxim


##### COMPILER OPTIONS #####

CXX       := g++
ifdef DEBUG
OPT_DEBUG := -g -DDEBUG -DATOMIC_FLOW_CONTROL_CHECK -DDEBUG_VC_UTILIZATION
else
OPT       := -O3
endif
#		OTHER     := -Wall -DSC_NO_WRITE_CHECK --std=c++14
OTHER     := -Wall -DSC_NO_WRITE_CHECK -DSC_DISABLE_API_VERSION_CHECK --std=c++14
CXXFLAGS  := $(OPT) $(OTHER) $(OPT_DEBUG)

INCDIR := -I$(SRCDIR) -I$(SYSTEMC)/include -I$(YAML)/include
LIBDIR := -L$(SRCDIR) -L$(SYSTEMC_LIBS) -L$(YAML)/lib -Wl,-rpath,$(SYSTEMC_LIBS)

LIBS := -lsystemc -lm -lyaml-cpp 

SPACE := $(subst ,, )
VPATH := $(SRCDIR):$(subst $(SPACE),:,$(SUBDIRS))

##### RULES #####

$(MODULE): $(OBJS)
	$(CXX) $(LIBDIR) $^ $(LIBS) -o $@ 2>&1 | c++filt

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCDIR) -c $< -o $@

clean:
	rm -f $(OBJS) $(MODULE) *~

depend: 
	makedepend $(SRCS) -Y -f- 2>/dev/null | sed 's_$(SRCDIR)_$(OBJDIR)_' > Makefile.deps

-include Makefile.deps
