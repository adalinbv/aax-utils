
set(THIRDPARTY_HEADERS
  gain_analysis.h
  MurmurHash3.h
)

set(THIRDPARTY_OBJS
  gain_analysis.c
  MurmurHash3.c
)

set(LIBTYPE STATIC)

set(LIBTHIRDPARTY thirdparty)
ADD_LIBRARY(${LIBTHIRDPARTY} ${LIBTYPE} ${THIRDPARTY_OBJS})
