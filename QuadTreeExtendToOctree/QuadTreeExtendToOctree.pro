QT += core gui opengl
QT += widgets

# as I want to support 4.8 and 5 this will set a flag for some of the mac stuff
# mainly in the types.h file for the setMacVisual which is native in Qt5
isEqual(QT_MAJOR_VERSION, 5) {
        cache()
        DEFINES +=QT5BUILD
}

TEMPLATE = app

SOURCES += src/*.cpp \

FORMS += \
    mainwindow.ui

HEADERS += include/*.h \

OBJECTS_DIR+=obj

OTHER_FILES+=$$PWD/shaders/*
CONFIG +=c++11


# and add the include dir into the search path for Qt and make
INCLUDEPATH +=./include
# where our exe is going to live (root of project)
DESTDIR=./
# add the glsl shader files
OTHER_FILES+= README.md
# were are going to default to a console app
CONFIG += console
# note each command you add needs a ; as it will be run as a single line
# first check if we are shadow building or not easiest way is to check out against current
#!equals(PWD, $${OUT_PWD}){
#	copydata.commands = echo "creating destination dirs" ;
#	# now make a dir
#	copydata.commands += mkdir -p $$OUT_PWD/shaders ;
#	copydata.commands += echo "copying files" ;
#	# then copy the files
#	copydata.commands += $(COPY_DIR) $$PWD/shaders/* $$OUT_PWD/shaders/ ;
#	# now make sure the first target is built before copy
#	first.depends = $(first) copydata
#	export(first.depends)
#	export(copydata.commands)
#	# now add it as an extra target
#	QMAKE_EXTRA_TARGETS += first copydata
#}
NGLPATH=$$(NGLDIR)
isEmpty(NGLPATH){ # note brace must be here
        message("including $HOME/NGL")
        include($(HOME)/NGL/UseNGL.pri)
}
else{ # note brace must be here
        message("Using custom NGL location")
        include($(NGLDIR)/UseNGL.pri)
}
