
set(BASE_HEADERS
  geometry.h
  logging.h
  random.h
  memory.h
  timer.h
  types.h
)

set(BASE_OBJS
  logging.c
  memory.c
  random.c
  timer.c
  types.c
)

set(LIBTYPE STATIC)

set(LIBBASE base)
ADD_LIBRARY(${LIBBASE} ${LIBTYPE} ${BASE_OBJS})
