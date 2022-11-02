
set(THIRDPARTY_HEADERS
  gain_analysis.h
  MurmurHash3.h
  ebur128.h
  pffft.h
)

set(THIRDPARTY_OBJS
  gain_analysis.c
  MurmurHash3.c
  ebur128.c
  pffft.c
)

set(LIBTYPE STATIC)

set(LIBTHIRDPARTY thirdparty)
ADD_LIBRARY(${LIBTHIRDPARTY} ${LIBTYPE} ${THIRDPARTY_OBJS})
