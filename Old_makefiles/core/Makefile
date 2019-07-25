# Get variables from Makefile.defs
include $(FLEXUS_ROOT)/makefile.defs

QEMU_SOURCES = $(subst $(CORE_DIR)/,,$(shell ls $(CORE_DIR)/qemu/*.cpp $(CORE_DIR)/qemu/aux_/*.cpp))
TEST_SOURCES = $(subst $(CORE_DIR)/,,$(shell ls $(CORE_DIR)/test/*.cpp))
DOC_SOURCES = core_documentation.cpp
PREFILTER_CORE_LIB_SOURCES = $(subst $(CORE_DIR)/,,$(shell ls $(CORE_DIR)/*/*.cpp)) $(subst $(CORE_DIR)/,,$(wildcard $(CORE_DIR)/*.cpp))
#CORE_LIB_SOURCES = $(filter-out $(STANDALONE_SOURCES) $(SIMICS_SOURCES) $(QEMU_SOURCES) $(TEST_SOURCES) $(DOC_SOURCES),$(PREFILTER_CORE_LIB_SOURCES))
CORE_LIB_SOURCES = $(filter-out $(STANDALONE_SOURCES) $(QEMU_SOURCES) $(TEST_SOURCES) $(DOC_SOURCES),$(PREFILTER_CORE_LIB_SOURCES))

# BOOST_DATE_TIME_PATH = $(BOOST_INCLUDE)/libs/date_time/src/gregorian
# BOOST_DATE_TIME_FILES = greg_month greg_weekday date_generators
# BOOST_DATE_TIME_SOURCES = $(addsuffix .cpp, $(BOOST_DATE_TIME_FILES))

# BOOST_REGEX_PATH = $(BOOST_INCLUDE)/libs/regex/src
# BOOST_REGEX_FILES =  c_regex_traits cpp_regex_traits regex_traits_defaults \
# 	cregex fileiter posix_api regex regex_debug w32_regex_traits icu static_mutex \
# 	regex_raw_buffer wide_posix_api instances winstances usinstances wc_regex_traits
# BOOST_REGEX_SOURCES = $(addsuffix .cpp, $(BOOST_REGEX_FILES))

# BOOST_SERIALIZATION_PATH = $(BOOST_INCLUDE)/libs/serialization/src
# BOOST_SERIALIZATION_FILES =      basic_archive basic_archive_impl basic_iarchive \
# 	basic_iserializer basic_oarchive basic_oserializer basic_pointer_iserializer \
# 	basic_pointer_oserializer basic_serializer_map basic_text_iprimitive \
# 	basic_text_oprimitive binary_iarchive  binary_oarchive codecvt_null \
# 	extended_type_info extended_type_info_no_rtti extended_type_info_typeid polymorphic_iarchive \
# 	polymorphic_oarchive stl_port text_iarchive text_oarchive void_cast
# BOOST_SERIALIZATION_SOURCES = $(addsuffix .cpp, $(BOOST_SERIALIZATION_FILES))

# BOOST_IOSTREAMS_PATH = $(BOOST_INCLUDE)/libs/iostreams/src
# BOOST_IOSTREAMS_FILES = zlib bzip2 file_descriptor mapped_file
# BOOST_IOSTREAMS_SOURCES = $(addsuffix .cpp, $(BOOST_IOSTREAMS_FILES))

include $(FLEXUS_ROOT)/makefile.variant

.SUFFIXES:
.SUFFIXES: .cpp .$(OEXT) .$(GCC_DEP_EXT) .$(ICC_DEP_EXT)

.PHONY: default clean clobber qemu test doc core

default: qemu 

# Standalone target

qemu: depend core
	$(MAKE) -f makefile.$(SELECTED_CC) libqemu_$(VARIANT).a $(BUILD_OPTIONS) SOURCES="$(QEMU_SOURCES)"

test: depend core
	$(MAKE) -f makefile.$(SELECTED_CC) test $(BUILD_OPTIONS) SOURCES="$(TEST_SOURCES)"

# core: depend iostreams
# 	$(MAKE) -f makefile.$(SELECTED_CC) libcore_$(VARIANT).a $(BUILD_OPTIONS) SOURCES="$(CORE_LIB_SOURCES)"

#core: 
core:
	$(MAKE) -f makefile.$(SELECTED_CC) libcore_$(VARIANT).a $(BUILD_OPTIONS) SOURCES="$(CORE_LIB_SOURCES)"

boost_libs:
	mkdir -p lib
	cd $(BOOST_INCLUDE) && \
	export PATH=$(GCC_PATH):$(PATH) && \
	./bootstrap.sh --without-icu --with-libraries=date_time,regex,serialization,iostreams,system && \
	echo "Building Boost Libraries: date_time regex serialization iostreams system" && \
	./b2 --build-dir=/tmp/build-boost --stagedir=$(CURRENT_TOOLCHAIN) variant=release link=static,shared stage 

# date_time: depend
# 	$(MAKE) -f makefile.$(SELECTED_CC) libboost_date_time.a $(BUILD_OPTIONS) BOOST_SRC_DIR="$(BOOST_DATE_TIME_PATH)" SOURCES="$(BOOST_DATE_TIME_SOURCES)"

# regex: depend date_time
# 	$(MAKE) -f makefile.$(SELECTED_CC) libboost_regex.a $(BUILD_OPTIONS)  BOOST_SRC_DIR="$(BOOST_REGEX_PATH)" SOURCES="$(BOOST_REGEX_SOURCES)"

# serialization: depend regex
# 	$(MAKE) -f makefile.$(SELECTED_CC) libboost_serialization.a $(BUILD_OPTIONS)  BOOST_SRC_DIR="$(BOOST_SERIALIZATION_PATH)" SOURCES="$(BOOST_SERIALIZATION_SOURCES)"

# iostreams: depend serialization
# 	$(MAKE) -f makefile.$(SELECTED_CC) libboost_iostreams.a $(BUILD_OPTIONS)  BOOST_SRC_DIR="$(BOOST_IOSTREAMS_PATH)" SOURCES="$(BOOST_IOSTREAMS_SOURCES)"


clean:
	echo "Cleaning core"
	find . -name "*.*_o" | xargs rm -f
	find . -name "*.*_dep" | xargs rm -f
	find . -name "Makefile.depend.*cc" | xargs rm -f
	find . -name "core.*.ipo_out.o" | xargs rm -f
	find . -name "qemu.*.ipo_out.o" | xargs rm -f
	find . -name "*.il" | xargs rm -f
	find . -name "core-test.*.ipo_out.o" | xargs rm -f
	find . -name "core-test.*.?cc" | xargs rm -f
	find . -name "*_v9_*_gcc.a" | xargs rm -f

clobber: clean
	echo "Clobbering core"
	find . -name "*cc.a" | xargs rm -f
	find . -name "libboost_*.a" | xargs rm -f
	find . -name "*gcc.a.fresh" | xargs rm -f
	rm -rf ../doc/core

doc:
	echo "Generating core documentation"
	$(DOXYGEN) doxygen.config

# Set up a depend target
###############

depend: Makefile.depend.$(VARIANT)

.cpp.$(GCC_DEP_EXT):
	$(GCC) $(INCLUDES) $(VARIANT_DEFINES) $(GCC_DEPFLAGS) -MF $@ -MT $*.$(GCC_OEXT) $<

Makefile.depend.$(GCC_VARIANT): $(patsubst %.cpp,%.$(GCC_DEP_EXT),$(CORE_LIB_SOURCES) $(STANDALONE_SOURCES) $(SIMICS_SOURCES) $(TEST_SOURCES) $(QEMU_SOURCES))
	cat $^ > Makefile.depend.$(GCC_VARIANT)


$(patsubst %.cpp,%.$(ICC_DEP_EXT),$(CORE_LIB_SOURCES)): %.$(ICC_DEP_EXT): %.cpp
	$(GCC) $(INCLUDES) $(VARIANT_DEFINES) $(GCC_DEPFLAGS) -MF $@ -MT core.$(VARIANT_EXT).ipo_out.o $<

$(patsubst %.cpp,%.$(ICC_DEP_EXT),$(QEMU_SOURCES)): %.$(ICC_DEP_EXT): %.cpp
	$(GCC) $(INCLUDES) $(VARIANT_DEFINES) $(GCC_DEPFLAGS) -MF $@ -MT qemu.$(VARIANT_EXT).ipo_out.o $<

Makefile.depend.$(ICC_VARIANT): $(patsubst %.cpp,%.$(ICC_DEP_EXT),$(CORE_LIB_SOURCES) $(STANDALONE_SOURCES) $(QEMU_SOURCES) $(SIMICS_SOURCES))
	cat $^ > Makefile.depend.$(ICC_VARIANT)
