#include <QApplication>
#include "mainwindow.h"

int main(int argc, char **argv)
{
    QApplication app(argc,argv);

    // create an OpenGL format specifier
    QSurfaceFormat format;
    // set the number of samples for multisampling
    // will need to enable glEnable(GL_MULTISAMPLE); once we have a context
    format.setSamples(4);
    #if defined( __APPLE__)
      // at present mac osx Mountain Lion only supports GL3.2
      // the new mavericks will have GL 4.x so can change
      format.setMajorVersion(4);
      format.setMinorVersion(1);
    #else
      // with luck we have the latest GL version so set to this
      format.setMajorVersion(4);
      format.setMinorVersion(3);
    #endif
    // now we are going to set to CoreProfile OpenGL so we can't use and old Immediate mode GL
    format.setProfile(QSurfaceFormat::CoreProfile);
    // now set the depth buffer to 24 bits
    format.setDepthBufferSize(24);

    QSurfaceFormat::setDefaultFormat(format);

    MainWindow w;


    // we can now query the version to see if it worked
    std::cout<<"Profile is "<<format.majorVersion()<<" "<<format.minorVersion()<<"\n";
    // set the window size
    w.resize(1024, 720);

    w.show ();
    return app.exec ();
}
