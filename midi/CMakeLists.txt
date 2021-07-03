
set( MIDI_OBJS
     instrument.cpp
     driver.cpp
     stream.cpp
     file.cpp
     xgmidi.cpp
   )

set(LIBTYPE STATIC)

set(LIBMIDI midi)
ADD_LIBRARY(${LIBMIDI} ${LIBTYPE} ${MIDI_OBJS})

