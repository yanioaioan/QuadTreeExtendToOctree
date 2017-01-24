#include <QMouseEvent>
#include <QGuiApplication>
#include "nglscene.h"
#include <iostream>
#include <ngl/NGLInit.h>
#include <ngl/VAOPrimitives.h>
#include <ngl/ShaderLib.h>
#include <ngl/Random.h>
#include <QTime>


//Initialize QuadTree
QuadTree tree(0,0,0,totalCollisionObjects,totalCollisionObjects,totalCollisionObjects);


//----------------------------------------------------------------------------------------------------------------------
/// @brief the increment for x/y translation with mouse movement
//----------------------------------------------------------------------------------------------------------------------
const static float INCREMENT=0.1;
//----------------------------------------------------------------------------------------------------------------------
/// @brief the increment for the wheel zoom
//----------------------------------------------------------------------------------------------------------------------
const static float ZOOM=5;
static int m_frames=0;
static bool paused=false;

QTime currenttime;
double tmpTimeElapsed=0;
int fps=0;

NGLScene::NGLScene()
{
    setFocusPolicy(Qt::StrongFocus);//to make the keyevents respond
//     this->resize(_parent->size ());

    // re-size the widget to that of the parent (in this case the GLFrame passed in on construction)
    m_rotate=false;
    // mouse rotation values set to 0
    m_spinXFace=0;
    m_spinYFace=0;
    updateTimer=startTimer(0);
    m_animate=true;

}

NGLScene::~NGLScene()
{
    std::cout<<"Shutting down NGL, removing VAO's and Shaders\n";
}

void NGLScene::initializeGL ()
{
    ngl::NGLInit::instance();
    glClearColor (0.4,0.4,0.4,1);
    std::cout<<"Initializing NGL\n";

    ngl::Vec3 from(100,200,200);ngl::Vec3 to(0,0,0);ngl::Vec3 up(0,1,0);
    m_cam = new ngl::Camera(from,to,up);
    m_cam->setShape(45,(float)720/576,0.05,350);

    m_text=new ngl::Text(QFont("Arial",14));
    m_text->setScreenSize (width (),height ());

    // now to load the shader and set the values
    // grab an instance of shader manager
    ngl::ShaderLib *shader=ngl::ShaderLib::instance();
    (*shader)["nglDiffuseShader"]->use();

    shader->setShaderParam4f("Colour",1,0,0,1);
    shader->setShaderParam3f("lightPos",1,1,1);
    shader->setShaderParam4f("lightDiffuse",1,1,1,1);

    glEnable(GL_DEPTH_TEST);
    // enable multisampling for smoother drawing
    glEnable(GL_MULTISAMPLE);

    // as re-size is not explicitly called we need to do this.
   glViewport(0,0,width(),height());

    //Fill random 2D Values to Quatree
    for(int i=0;i<totalCollisionObjects;i++)
    {
       ngl::Random *rng=ngl::Random::instance ();
       int x = (int)rng->randomPositiveNumber(100);
       int y = (int)rng->randomPositiveNumber (100);
       int z = (int)rng->randomPositiveNumber (100);

       //save positions
       Point t(x,y,z);
       treePositions.push_back (t);

       Point tempPoint(x,y,z);//or insert x,y instead of i,i to create some randomness
       tree.addPoint(tempPoint);
    }


    //find & get the collision neighbours of Point a(8,8), if (8,8) is in the tree
//    Point a(3,3);
    //Point a(2530,7399);    
//    getPointCollisions(a,&tree);

    currenttime.start ();
    tmpTimeElapsed = 0;
    fps= 0;
}

void NGLScene::resizeGL (QResizeEvent *_event )
{
    int w=_event->size().width();
    int h=_event->size().height();
    // set the viewport for openGL
    glViewport(0,0,w,h);
    // now set the camera size values as the screen size has changed
    m_cam->setShape(45,(float)w/h,0.05,350);
    update ();
}

ngl::Mat4 MV;
ngl::Mat4 MVP;
ngl::Mat3 normalMatrix;
ngl::Mat4 M;
///////////////////////////////////
void NGLScene::loadMatricesToShader(ngl::Transformation &_transform, const ngl::Mat4 &_globalTx, ngl::Camera *_cam, ngl::Colour &c)
{
    ngl::ShaderLib *shader=ngl::ShaderLib::instance();
//    (*shader)["nglDiffuseShader"]->use();

    shader->setRegisteredUniformFromColour ("Colour",c);


    M=m_transform.getMatrix()*m_mouseGlobalTX;
    MV=  M*m_cam->getViewMatrix();
    MVP=  MV*m_cam->getProjectionMatrix();
    normalMatrix=MV;
    normalMatrix.inverse();
//    shader->setShaderParamFromMat4("MV",MV);
    shader->setShaderParamFromMat4("MVP",MVP);
    shader->setShaderParamFromMat3("normalMatrix",normalMatrix);
//    shader->setShaderParamFromMat4("M",M);
}


void NGLScene::detectAndResolveCollisions(Point &a, std::vector<Point> *collisionAreaPoints , const float & width, const float & height)
{
    for(size_t i=0;i<collisionAreaPoints->size ();i++)
    {
       if(a.x < (*collisionAreaPoints)[i].x+width &&
          a.x+width > (*collisionAreaPoints)[i].x &&
          a.y < (*collisionAreaPoints)[i].y+height &&
          a.y+height > (*collisionAreaPoints)[i].y)
       {
//          a.x=0.0001;
//          a.y=0.0001;

           //calculate x intersection
           float xOffset=/*abs*/(a.x-(*collisionAreaPoints)[i].x);
           float yOffset=/*abs*/(a.y-(*collisionAreaPoints)[i].y);

           //this point make resovles collisions, and makes it run fast
           //(IF commented out it runs slower)

           //move point / push away
           a.x+=xOffset;
           a.y+=yOffset;

//           (*collisionAreaPoints)[i].x+=xOffset;
//           (*collisionAreaPoints)[i].y+=yOffset;
       }


    }

}

// Determine which octant of the tree would contain 'point'
int NGLScene::getOctantContainingPoint( Point& point, QuadTree *tree) const {
            int oct = 0;
            if(point.x >= tree->x) oct |= 4; //tree->x,y,z is the origin of this tree/subtree
            if(point.y >= tree->y) oct |= 2;
            if(point.z >= tree->z) oct |= 1;
            return oct;
        }


//This method used to be a member function of Quatree Structure
void NGLScene::getPointCollisions(Point &a, QuadTree *tree)
{
    //if tree node is a leaf node
    if ( (tree->top_dl==NULL && tree->top_dr==NULL && tree->top_ul==NULL && tree->top_ur==NULL)
         && (tree->bottom_dl==NULL && tree->bottom_dr==NULL && tree->bottom_ul==NULL && tree->bottom_ur==NULL)
          )
    {
        //found element in root tree
        std::vector<Point>::iterator element=std::find(tree->container.begin(), tree->container.end(), a);

        if( element !=tree->container.end ())//found element
        {
            /////////////////////       !!!!!!!!!!!!!!!!!
            /// \Section Added - For each element  of the tree that we search, we print the whole container using a another colour.
            /// Each neighbourhood should now have a different separate colour
            ///
            ngl::Colour collisionAreaColour(ngl::Random::instance ()->randomPositiveNumber (), ngl::Random::instance ()->randomPositiveNumber (), ngl::Random::instance ()->randomPositiveNumber (), 1);

            //**THIS could be the vector to test against (in ex. for neighbour search)**
            std::vector<Point> *collisionAreaPoints = &tree->container;

//            std::cout<<"neighbours of point at position("<<a.x<<","<<a.y<<") are="<<collisionAreaPoints->size()<<" out of all "<<totalCollisionObjects<<'\n';

            //Check collisions and push cube that collide with eachother further apart
//            detectAndResolveCollisions( (*element), collisionAreaPoints, tree->width, tree->height);

            for(size_t i=0;i<collisionAreaPoints->size ();i++)
            {
                m_transform.setPosition ( (*collisionAreaPoints)[i].x/*/totalCollisionObjects*/ ,(*collisionAreaPoints)[i].y/*/totalCollisionObjects*/, (*collisionAreaPoints)[i].z);
//                m_transform.setScale (0.01, 0.01, 1);

                loadMatricesToShader (m_transform,m_mouseGlobalTX, m_cam, collisionAreaColour);
                ngl::VAOPrimitives *prim=ngl::VAOPrimitives::instance ();
                prim->draw ("cube");

            }

             return;
        }

    }
    else//if it's a branch , find if one of the 8 sub-areas intersects with the searched area / and if yes..then draw it there in that area
    {

        //find the octant the point is in , instead of checking all 8 individual split pieces of space
        int octant=getOctantContainingPoint(a, tree);
        if (octant!=7)
            std::cout<<"octant="<<octant<<'\n';

        //TOP

        if (a.x >= tree->top_dl->x && a.x <= tree->top_dl->x+tree->top_dl->width &&
            a.y >= tree->top_dl->y && a.y <= tree->top_dl->y+tree->top_dl->height &&
            a.z >= tree->top_dl->z && a.z <= tree->top_dl->z+tree->top_dl->depth
            )
        {
            getPointCollisions (a,(tree->top_dl));
        }

        if (a.x >= tree->top_dr->x && a.x <= tree->top_dr->x+tree->top_dr->width &&
            a.y >= tree->top_dr->y && a.y <= tree->top_dr->y+tree->top_dr->height &&
            a.z >= tree->top_dr->z && a.z <= tree->top_dr->z+tree->top_dr->depth
           )
        {
            getPointCollisions (a,(tree->top_dr));
        }

        if (a.x >= tree->top_ul->x && a.x <= tree->top_ul->x+tree->top_ul->width &&
            a.y >= tree->top_ul->y && a.y <= tree->top_ul->y+tree->top_ul->height &&
            a.z >= tree->top_ul->z && a.z <= tree->top_ul->z+tree->top_ul->depth
            )
        {
            getPointCollisions (a,(tree->top_ul));
        }

        if (a.x >= tree->top_ur->x && a.x <= tree->top_ur->x+tree->top_ur->width &&
            a.y >= tree->top_ur->y && a.y <= tree->top_ur->y+tree->top_ur->height &&
            a.z >= tree->top_ur->z && a.z <= tree->top_ur->z+tree->top_ur->depth
            )
        {
            getPointCollisions (a,(tree->top_ur));
        }

        //BOTTOM

        if (a.x >= tree->top_dl->x && a.x <= tree->top_dl->x+tree->top_dl->width &&
            a.y >= tree->top_dl->y && a.y <= tree->top_dl->y+tree->top_dl->height &&
            a.z >= tree->top_dl->z && a.z <= tree->top_dl->z+tree->top_dl->depth
            )
        {
            getPointCollisions (a,(tree->top_dl));
        }

        if (a.x >= tree->bottom_dr->x && a.x <= tree->bottom_dr->x+tree->bottom_dr->width &&
            a.y >= tree->bottom_dr->y && a.y <= tree->bottom_dr->y+tree->bottom_dr->height &&
            a.z >= tree->bottom_dr->z && a.z <= tree->bottom_dr->z+tree->bottom_dr->depth
           )
        {
            getPointCollisions (a,(tree->bottom_dr));
        }

        if (a.x >= tree->bottom_ul->x && a.x <= tree->bottom_ul->x+tree->bottom_ul->width &&
            a.y >= tree->bottom_ul->y && a.y <= tree->bottom_ul->y+tree->bottom_ul->height &&
            a.z >= tree->bottom_ul->z && a.z <= tree->bottom_ul->z+tree->bottom_ul->depth
            )
        {
            getPointCollisions (a,(tree->bottom_ul));
        }

        if (a.x >= tree->bottom_ur->x && a.x <= tree->bottom_ur->x+tree->bottom_ur->width &&
            a.y >= tree->bottom_ur->y && a.y <= tree->bottom_ur->y+tree->bottom_ur->height &&
            a.z >= tree->bottom_ur->z && a.z <= tree->bottom_ur->z+tree->bottom_ur->depth
            )
        {
            getPointCollisions (a,(tree->bottom_ur));
        }



    }

}


void NGLScene::deleteAreaByAreaElements(QuadTree &tree)
{
    ngl::ShaderLib *shader = ngl::ShaderLib::instance ();
     (*shader)["nglDiffuseShader"]->use();

    if (tree.container.size ()!=0)
    {
        //std::vector<Point> collisionAreaPoints(tree.container);

        ngl::Colour collisionAreaColour(ngl::Random::instance ()->randomPositiveNumber (), ngl::Random::instance ()->randomPositiveNumber (), ngl::Random::instance ()->randomPositiveNumber (), 1);
//        for(int i=0;i<tree.container.size ();i++)
        {

            //clear all tree area elements
//            tree.container.clear ();

            loadMatricesToShader (m_transform,m_mouseGlobalTX, m_cam, collisionAreaColour);
            ngl::VAOPrimitives *prim=ngl::VAOPrimitives::instance ();
            prim->draw ("cube");
        }

        update ();

    }
    else
    {
        return;
    }



    //Comment out some of the following quadtree collision regions and PushButton on the Gui, to delete some of the 1st level quatree splits (1 of the 4 first quadrants of the quadtree)
    if (tree.top_dl!=NULL)
    {
        update ();
    }
    if (tree.top_dr!=NULL)
    {
        update ();
    }
    if (tree.top_ul!=NULL)
    {
        update ();
    }
    if (tree.top_ur!=NULL)
    {
        update ();
    }

    //Bottom
    if (tree.bottom_dl!=NULL)
    {
        update ();
    }
    if (tree.bottom_dr!=NULL)
    {
        update ();
    }
    if (tree.bottom_ul!=NULL)
    {
        update ();
    }
    if (tree.bottom_ur!=NULL)
    {
        update ();
    }

}

void NGLScene::paintGL ()
{
    ngl::ShaderLib *shader=ngl::ShaderLib::instance();
    (*shader)["nglDiffuseShader"]->use();

    // clear the screen and depth buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // Rotation based on the mouse position for our global
    // transform
    ngl::Mat4 rotX;
    ngl::Mat4 rotY;
    // create the rotation matrices
    rotX.rotateX(m_spinXFace);
    rotY.rotateY(m_spinYFace);
    // multiply the rotations
    m_mouseGlobalTX=rotY*rotX;
    // add the translations
    m_mouseGlobalTX.m_m[3][0] = m_modelPos.m_x;
    m_mouseGlobalTX.m_m[3][1] = m_modelPos.m_y;
    m_mouseGlobalTX.m_m[3][2] = m_modelPos.m_z;


        for(unsigned int i=0;i<treePositions.size ();i++)
        {
           getPointCollisions(treePositions[i],&tree);
        }

        QString text;
        text.sprintf ("Framerate is %d", fps);
        m_text->setColour (0.2,0.6,0.5);
        m_text->renderText (10,10,text);


        m_frames++;
}


void NGLScene::testButtonClicked(bool b)
{
    emit clicked (b);
    std::cout<<"Button Clicked - manual signal-slot connection"<<std::endl;
    // m_rColor=1;

    //delete some of the 1st level nodes of the tree
    deleteAreaByAreaElements(tree);

    update ();
}


void NGLScene::timerEvent( QTimerEvent *_event )
{
    if(_event->timerId() == updateTimer && !paused)
    {
        if(currenttime.elapsed() > 1000)
        {
            fps=m_frames;
            m_frames=0;
            currenttime.restart();
        }


        update ();
    }
}

//----------------------------------------------------------------------------------------------------------------------
void NGLScene::mouseMoveEvent (QMouseEvent * _event)
{
  // note the method buttons() is the button state when event was called
  // this is different from button() which is used to check which button was
  // pressed when the mousePress/Release event is generated
  if(m_rotate && _event->buttons() == Qt::LeftButton)
  {
    int diffx=_event->x()-m_origX;
    int diffy=_event->y()-m_origY;
    m_spinXFace += (float) 0.5f * diffy;
    m_spinYFace += (float) 0.5f * diffx;
    m_origX = _event->x();
    m_origY = _event->y();
    update ();
  }
        // right mouse translate code
  else if(m_translate && _event->buttons() == Qt::RightButton)
  {
    int diffX = (int)(_event->x() - m_origXPos);
    int diffY = (int)(_event->y() - m_origYPos);
    m_origXPos=_event->x();
    m_origYPos=_event->y();
    m_modelPos.m_x += INCREMENT * diffX;
    m_modelPos.m_y -= INCREMENT * diffY;
    update();

   }
}


//----------------------------------------------------------------------------------------------------------------------
void NGLScene::mousePressEvent ( QMouseEvent * _event)
{
  // this method is called when the mouse button is pressed in this case we
  // store the value where the maouse was clicked (x,y) and set the Rotate flag to true
  if(_event->button() == Qt::LeftButton)
  {
    m_origX = _event->x();
    m_origY = _event->y();
    m_rotate =true;
  }
  // right mouse translate mode
  else if(_event->button() == Qt::RightButton)
  {
    m_origXPos = _event->x();
    m_origYPos = _event->y();
    m_translate=true;
  }

}

//----------------------------------------------------------------------------------------------------------------------
void NGLScene::mouseReleaseEvent ( QMouseEvent * _event )
{
  // this event is called when the mouse button is released
  // we then set Rotate to false
  if (_event->button() == Qt::LeftButton)
  {
    m_rotate=false;
  }
        // right mouse translate mode
  if (_event->button() == Qt::RightButton)
  {
    m_translate=false;
  }
}

//----------------------------------------------------------------------------------------------------------------------
void NGLScene::wheelEvent(QWheelEvent *_event)
{

    // check the diff of the wheel position (0 means no change)
    if(_event->delta() > 0)
    {
        m_modelPos.m_z+=ZOOM;
    }
    else if(_event->delta() <0 )
    {
        m_modelPos.m_z-=ZOOM;
    }
    update();
}
//----------------------------------------------------------------------------------------------------------------------

void NGLScene::keyPressEvent(QKeyEvent *_event)
{
  // this method is called every time the main window recives a key event.
  // we then switch on the key value and set the camera in the GLWindow
  switch (_event->key())
  {
  // escape key to quite
  case Qt::Key_Escape : QGuiApplication::exit(EXIT_SUCCESS); break;
//  case Qt::Key_Up    : m_plane->tilt(1.0,1,0); break;
  case Qt::Key_P : paused=!paused; break; //pause app
//  case Qt::Key_Down  : m_plane->tilt(-1.0,1,0); break;
//  case Qt::Key_Left  : m_plane->tilt(-1.0,0,1); break;
//  case Qt::Key_Right : m_plane->tilt(1.0,0,1); break;
  default : break;
  }
  // finally update the GLWindow and re-draw
//  if (isExposed())
    update();
}
