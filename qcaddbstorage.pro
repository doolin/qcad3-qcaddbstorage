TEMPLATE = lib
DESTDIR = lib
CONFIG -= qt
CONFIG += staticlib warn_on

HEADERS = \
    ./src/rs_dbsentity.h \
    ./src/rs_dbsline.h \
    ./src/rs_dbsentityregistry.h \
    ./src/rs_dbstorage.h
SOURCES = \
    ./src/rs_dbsentity.cpp \
    ./src/rs_dbsline.cpp \
    ./src/rs_dbsentityregistry.cpp \
    ./src/rs_dbstorage.cpp

TARGET = qcaddbstorage
OBJECTS_DIR = .obj
MOC_DIR = .moc

CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,_d)
    OBJECTS_DIR = $$join(OBJECTS_DIR,,,_d)
}

exists( ../mkspecs/defs.pro ):include( ../mkspecs/defs.pro )
