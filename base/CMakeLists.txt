
set(BASE_HEADERS
  geometry.h
  logging.h
  timer.h
  types.h
)

set(BASE_OBJS
  types.c
  timer.c
)

set(LIBTYPE STATIC)

set(LIBBASE base)
ADD_LIBRARY(${LIBBASE} ${LIBTYPE} ${BASE_OBJS})
