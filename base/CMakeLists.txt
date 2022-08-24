
set(BASE_HEADERS
  geometry.h
  logging.h
  memory.h
  timer.h
  types.h
)

set(BASE_OBJS
  logging.c
  memory.c
  timer.c
  types.c
)

set(LIBTYPE STATIC)

set(LIBBASE base)
ADD_LIBRARY(${LIBBASE} ${LIBTYPE} ${BASE_OBJS})
