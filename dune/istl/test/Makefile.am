# $Id$

if MPI
  MPITESTS = vectorcommtest matrixmarkettest
endif

if MPI
  PARMETISTESTS= matrixredisttest
endif

if SUPERLU
  SUPERLUTESTS =  superlutest superluztest superluctest superlustest overlappingschwarztest
endif

if PARDISO
  PARDISOTEST = test_pardiso
endif

# which tests where program to build and run are equal
NORMALTESTS = basearraytest matrixutilstest matrixtest mmtest bvectortest vbvectortest \
	bcrsbuildtest matrixiteratortest mv iotest scaledidmatrixtest seqmatrixmarkettest

# list of tests to run (indicestest is special case)
TESTS = $(NORMALTESTS) $(MPITESTS) $(SUPERLUTESTS) $(PARDISOTEST) $(PARMETISTESTS)

# programs just to build when "make check" is used
check_PROGRAMS = $(NORMALTESTS) $(MPITESTS) $(SUPERLUTESTS) $(PARDISOTEST) $(PARMETISTESTS)

# define the programs

if SUPERLU
  superlutest_SOURCES = superlutest.cc
  superlutest_LDADD= $(SUPERLU_LIBS)
  superlutest_LDFLAGS= $(AM_LDFLAGS) $(SUPERLU_LDFLAGS)
  superlutest_CPPFLAGS=$(AM_CPPFLAGS) $(SUPERLU_CPPFLAGS)

  superlustest_SOURCES = superlutest.cc
  superlustest_LDADD= $(SUPERLU_LIBS)
  superlustest_LDFLAGS= $(AM_LDFLAGS) $(SUPERLU_LDFLAGS)
  superlustest_CPPFLAGS=$(AM_CPPFLAGS) $(SUPERLU_CPPFLAGS) -DSUPERLU_NTYPE=0


  superluctest_SOURCES = superlutest.cc
  superluctest_LDADD= $(SUPERLU_LIBS)
  superluctest_LDFLAGS= $(AM_LDFLAGS) $(SUPERLU_LDFLAGS)
  superluctest_CPPFLAGS=$(AM_CPPFLAGS) $(SUPERLU_CPPFLAGS) -DSUPERLU_NTYPE=2


  superluztest_SOURCES = superlutest.cc
  superluztest_LDADD= $(SUPERLU_LIBS)
  superluztest_LDFLAGS= $(AM_LDFLAGS) $(SUPERLU_LDFLAGS)
  superluztest_CPPFLAGS=$(AM_CPPFLAGS) $(SUPERLU_CPPFLAGS) -DSUPERLU_NTYPE=3

  overlappingschwarztest_SOURCES = overlappingschwarztest.cc
  overlappingschwarztest_LDADD= $(SUPERLU_LIBS)
  overlappingschwarztest_LDFLAGS= $(AM_LDFLAGS) $(SUPERLU_LDFLAGS)
  overlappingschwarztest_CPPFLAGS=$(AM_CPPFLAGS) $(SUPERLU_CPPFLAGS)
endif

if PARDISO
  test_pardiso_SOURCES = test_pardiso.cc
  test_pardiso_LDADD = $(PARDISO_LIBS) $(LAPACK_LIBS) $(BLAS_LIBS) $(LIBS) $(FLIBS)
endif 

basearraytest_SOURCES = basearraytest.cc

bcrsbuildtest_SOURCES = bcrsbuild.cc

bvectortest_SOURCES = bvectortest.cc

vbvectortest_SOURCES = vbvectortest.cc

matrixutilstest_SOURCES = matrixutilstest.cc laplacian.hh

matrixiteratortest_SOURCES = matrixiteratortest.cc

matrixtest_SOURCES = matrixtest.cc

mmtest_SOURCES = mmtest.cc

mv_SOURCES = mv.cc

iotest_SOURCES = iotest.cc

scaledidmatrixtest_SOURCES = scaledidmatrixtest.cc

if MPI
  vectorcommtest_SOURCES = vectorcommtest.cc
  vectorcommtest_CPPFLAGS = $(AM_CPPFLAGS)	\
	$(DUNEMPICPPFLAGS)
  vectorcommtest_LDFLAGS = $(AM_LDFLAGS)	\
	$(DUNEMPILDFLAGS)
  vectorcommtest_LDADD =			\
	$(DUNEMPILIBS)				\
	$(LDADD)
  matrixmarkettest_SOURCES = matrixmarkettest.cc
  matrixmarkettest_CPPFLAGS = $(AM_CPPFLAGS)	\
	$(DUNEMPICPPFLAGS)
  matrixmarkettest_LDFLAGS = $(AM_LDFLAGS)	\
	$(DUNEMPILDFLAGS)
  matrixmarkettest_LDADD =			\
	$(DUNEMPILIBS)				\
	$(LDADD)
endif

seqmatrixmarkettest_SOURCES = matrixmarkettest.cc

if MPI  
  matrixredisttest_SOURCES = matrixredisttest.cc
  matrixredisttest_CPPFLAGS = $(AM_CPPFLAGS)	\
	$(DUNEMPICPPFLAGS)			\
	${PARMETIS_CPPFLAGS}
  matrixredisttest_LDFLAGS = $(AM_LDFLAGS)	\
	$(DUNEMPILDFLAGS)			\
	${PARMETIS_LDFLAGS}
  matrixredisttest_LDADD =			\
	${PARMETIS_LDFLAGS} ${PARMETIS_LIBS}	\
	${DUNEMPILIBS}				\
	$(LDADD)
endif
include $(top_srcdir)/am/global-rules
