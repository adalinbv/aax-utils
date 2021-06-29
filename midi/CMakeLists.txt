
set( MIDI_OBJS
     instrument.cpp
     driver.cpp
     stream.cpp
     file.cpp
   )

set(LIBTYPE STATIC)

set(LIBMIDI midi)
ADD_LIBRARY(${LIBMIDI} ${LIBTYPE} ${MIDI_OBJS})

