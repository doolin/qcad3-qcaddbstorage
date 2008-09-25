exists( ../mkspecs/defs.pro ):include( ../mkspecs/defs.pro )

TEMPLATE = lib
DESTDIR = lib
CONFIG -= qt
CONFIG += staticlib warn_on

HEADERS = \
    ./src/rs_dbsentitytype.h \
    ./src/rs_dbsobjecttype.h \
    ./src/rs_dbslinetype.h \
    ./src/rs_dbsentitytyperegistry.h \
    ./src/rs_dbstorage.h \
    ./src/rs_dbsucstype.h
SOURCES = \
    ./src/rs_dbsentitytype.cpp \
    ./src/rs_dbsobjecttype.cpp \
    ./src/rs_dbslinetype.cpp \
    ./src/rs_dbsentitytyperegistry.cpp \
    ./src/rs_dbstorage.cpp \
    ./src/rs_dbsucstype.cpp

TARGET = qcaddbstorage
OBJECTS_DIR = .obj
MOC_DIR = .moc

CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,_d)
    OBJECTS_DIR = $$join(OBJECTS_DIR,,,_d)
}
