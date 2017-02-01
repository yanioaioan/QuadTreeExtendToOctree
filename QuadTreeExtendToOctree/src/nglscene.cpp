#include <QMouseEvent>
#include <QGuiApplication>
#include "nglscene.h"
#include <iostream>
#include <ngl/NGLInit.h>
#include <ngl/VAOPrimitives.h>
#include <ngl/ShaderLib.h>
#include <ngl/Random.h>
#include <QTime>


//Initialize Octree
int treewidth=totalCollisionObjects*4;int treeheight=totalCollisionObjects*4;int treedepth=totalCollisionObjects*4;

int Octree::nexttreeID=0;

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
    //std::cout<<"Shutting down NGL, removing VAO's and Shaders\n";
}

void NGLScene::initializeGL ()
{
    ngl::NGLInit::instance();
    glClearColor (0.4,0.4,0.4,1);
    //std::cout<<"Initializing NGL\n";

    ngl::Vec3 from(50,50,50);ngl::Vec3 to(20,20,0);ngl::Vec3 up(0,1,0);
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

   tree.reset(new Octree(0,0,0,treewidth,treeheight,treedepth));

    //Fill random 2D Values to Octree
    for(int i=0;i<totalCollisionObjects;i++)
    {
       ngl::Random *rng=ngl::Random::instance ();
       float x = (float)rng->randomPositiveNumber(treewidth/4);
       float y = (float)rng->randomPositiveNumber (treeheight/4);
       float z = (float)rng->randomPositiveNumber (treedepth/4);

//       x=i;y=0;z=0;

       //save positions
       Point t(i,x,y,z,0.1,0.1,0.1);
       treePositions.push_back (t);


       Point tempPoint(i,x,y,z,0.1,0.1,0.1);//or insert x,y instead of i,i to create some randomness
       tree->addPoint(tempPoint);
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
int NGLScene::getOctantContainingPoint( Point& point, Octree *tree) const {
            int oct = 0;
            if(point.x >= tree->x) oct |= 4; //tree->x,y,z is the origin of this tree/subtree
            if(point.y >= tree->y) oct |= 2;
            if(point.z >= tree->z) oct |= 1;
            return oct;
        }


//This method used to be a member function of Octree Structure
void NGLScene::getPointCollisions(const Point &a, Octree *tree)
{


//    ngl::Vec3 P, Q, Vp, N, force;
//    float Rp, Rq, dist, speed;
//    BOOST_FOREACH(Particle *currentParticle,node->m_objectList)
//    {
//        P = currentParticle->getPosition();
//        Vp = currentParticle->getCurrentSpeed();
//        speed = Vp.length();
//        Rp = currentParticle->getRadius();
//        force.m_x = force.m_y = force.m_z = 0.0;

//        BOOST_FOREACH(Particle *testParticle,node->m_objectList )
//        {
//            // no need to self test
//            if(testParticle==currentParticle)
//            {
//              // continue
//              continue;
//            }
//            Q = testParticle->getPosition();
//            Rq = testParticle->getRadius();
//            dist = Rq + Rq - (P-Q).length();
//            if(dist > 0) // collision between P and Q
//            {
//              N = P-Q;
//              N.normalize();
//              force += dist*N;
//            }
//        }
//        Vp += force;
//        Vp.normalize();
//        currentParticle->setSpeed(Vp*speed);
//    }



//    m_transform.setPosition (a.x, a.y, a.z);
//    ngl::Colour collisionAreaColour(ngl::Random::instance ()->randomPositiveNumber (), ngl::Random::instance ()->randomPositiveNumber (), ngl::Random::instance ()->randomPositiveNumber (), 1);
//    loadMatricesToShader (m_transform,m_mouseGlobalTX, m_cam, collisionAreaColour);
//    ngl::VAOPrimitives *prim=ngl::VAOPrimitives::instance ();
//    prim->draw ("cube");


    //if tree node is a leaf node (all of its 8 pointers have point to NULL)
    if ( (tree->front_dl==NULL && tree->front_dr==NULL && tree->front_ul==NULL && tree->front_ur==NULL)
         && (tree->back_dl==NULL && tree->back_dr==NULL && tree->back_ul==NULL && tree->back_ur==NULL)
          )
    {
        //std::cout<<"tree->treeID= "<<tree->treeId<<'\n';
        //std::cout<<"tree->container.size() = "<<tree->container.size()<<'\n';
        if(tree->container.size()<=1)
        {
//            tree->container[0].x=0;
//            tree->container[0].y=0;
//            tree->container[0].z=0;

            return;
        }
        else//iterate the particles of this container (n squared ) and perform collision detection and response
        {
            for(int i=0;i<tree->container.size();++i)
            {

//                tree->container[i].x=0;
//                tree->container[i].y=0;
//                tree->container[i].z=0;

                ngl::Vec3 p1(tree->container[i].x,tree->container[i].y,tree->container[i].z);

                for(int j=0;j<tree->container.size();++j)
                {
                    if (tree->container[i].id != tree->container[j].id)//do not check particle with itself
                    {
                        ngl::Vec3 p2(tree->container[j].x,tree->container[j].y,tree->container[j].z);

                        float lengthsquared = (p1-p2).lengthSquared() ;
                        if ( (p1-p2).lengthSquared() < tree->container[i].radius * tree->container[i].radius )
                        {

                            ngl::Vec3 N(p1-p2);
                            N.normalize();
                            //the penetrated distance
                            float dist=tree->container[i].radius + tree->container[j].radius - (p1-p2).length();
                            ngl::Vec3 norm=dist*N;


//                            //collision detected/response, so move each other apart randomly towards all three directions
                            tree->container[i].x+=norm.m_x;
                            tree->container[i].y+=norm.m_y;
                            tree->container[i].z+=norm.m_z;

                            tree->container[j].x-=norm.m_x;
                            tree->container[j].y-=norm.m_y;
                            tree->container[j].z-=norm.m_z;


                        }
                    }

                }


            }
        }


//        // search element a in this tree
//        std::vector<Point>::iterator element=std::find(tree->container.begin(), tree->container.end(), a);


//        //std::cout<<"searching for particle with ID="<<a.id<<'\n';

//        if( element !=tree->container.end ())// 'a' element found!!
//        {

//            //std::cout<<"FOUND to have neighbours!!"<<'\n';

//            //std::cout<<"drawing neighbours of particle_with id="<<a.id<<", at pos:("<<a.x<<", "<<a.y<<", "<<a.z<<")"<<'\n';

//            /////////////////////       !!!!!!!!!!!!!!!!!
//            /// \Section Added - For each element  of the tree that we search, we print the whole container using a another colour.
//            /// Each neighbourhood should now have a different separate colour
//            ///
//            ngl::Colour collisionAreaColour(ngl::Random::instance ()->randomPositiveNumber (), ngl::Random::instance ()->randomPositiveNumber (), ngl::Random::instance ()->randomPositiveNumber (), 1);

//            //**THIS could be the vector to test against (in ex. for neighbour search)**
//            //SO..this &tree->container that contains the 'a'point, is the one to draw with this specifiv collisionAreaColour
//            std::vector<Point> *collisionAreaPoints = &tree->container;

////            //std::cout<<"neighbours of point at position("<<a.x<<","<<a.y<<") are="<<collisionAreaPoints->size()<<" out of all "<<totalCollisionObjects<<'\n';

//            //Check collisions and push cube that collide with eachother further apart
////            detectAndResolveCollisions( (*element), collisionAreaPoints, tree->width, tree->height);

//            for(size_t i=0;i<collisionAreaPoints->size ();i++)
//            {
//                //std::cout<<"neighbour id:"<<(*collisionAreaPoints)[i].id<<'\n';

//                m_transform.setPosition ( (*collisionAreaPoints)[i].x/*/totalCollisionObjects*/ ,i*2*(*collisionAreaPoints)[i].y/*/totalCollisionObjects*/, (*collisionAreaPoints)[i].z);
////                m_transform.setScale (0.01, 0.01, 1);

//                loadMatricesToShader (m_transform,m_mouseGlobalTX, m_cam, collisionAreaColour);
//                ngl::VAOPrimitives *prim=ngl::VAOPrimitives::instance ();
//                prim->draw ("cube");


//            }

//             return;
//        }
//        else
//        {
//            //std::cout<<"NOT FOUND  to have neighbours!!!!..so draw it on  its own"<<'\n';
//            //std::cout<<"particle id:"<<a.id<<", at pos:("<<a.x<<", "<<a.y<<", "<<a.z<<")"<<'\n';'\n';

//            ngl::Colour collisionAreaColour(ngl::Random::instance ()->randomPositiveNumber (), ngl::Random::instance ()->randomPositiveNumber (), ngl::Random::instance ()->randomPositiveNumber (), 1);
//            m_transform.setPosition(a.x, a.y, a.z);
//            loadMatricesToShader (m_transform,m_mouseGlobalTX, m_cam, collisionAreaColour);
//            ngl::VAOPrimitives *prim=ngl::VAOPrimitives::instance ();
//            prim->draw ("cube");

//            return;
//        }

    }
    else//if it's a branch , find if one of the 8 sub-areas intersects with the searched area / and if yes..then draw it there in that area
    {

        //find the octant the point is in , instead of checking all 8 individual split pieces of space
//        int octant=getOctantContainingPoint(a, tree);
//        if (octant!=7)
//


//        int xsign = a.x>tree->width/2;
//        int ysign = a.y>tree->height/2;
//        int zsign = a.z>tree->depth/2;

//        int octant = xsign + 2*ysign + 4*zsign;
//        //std::cout<<"octant="<<octant<<'\n';


        //SOMETHING IS NOT QUITE WORKING HERE (probably wrong octant assignment),
        //So, need to investigate at some point ,
        //(however the manual octant check after that works just fine, so use this for now at least)

        // find in which octant does the 'a' point lies into
//        if (octant==0)
//            getPointCollisions (a,(tree->front_dl));
//        if (octant==1)
//            getPointCollisions (a,(tree->front_dr));
//        if (octant==2)
//            getPointCollisions (a,(tree->front_ul));
//        if (octant==3)
//            getPointCollisions (a,(tree->front_ur));
//        if (octant==4)
//            getPointCollisions (a,(tree->back_dl));
//        if (octant==5)
//            getPointCollisions (a,(tree->back_dr));
//        if (octant==6)
//            getPointCollisions (a,(tree->back_ul));
//        if (octant==7)
//            getPointCollisions (a,(tree->back_ur));



        //TOP

        if (a.x >= tree->front_dl->x && a.x <= tree->front_dl->x+tree->front_dl->width &&
            a.y >= tree->front_dl->y && a.y <= tree->front_dl->y+tree->front_dl->height &&
            a.z >= tree->front_dl->z && a.z <= tree->front_dl->z+tree->front_dl->depth
            )
        {
            //and dig one level down to check if 'a' is one of the leaf nodes of this tree->front_dl, so as to query all of its neighbours too
            getPointCollisions (a,(tree->front_dl));
        }

        if (a.x >= tree->front_dr->x && a.x <= tree->front_dr->x+tree->front_dr->width &&
            a.y >= tree->front_dr->y && a.y <= tree->front_dr->y+tree->front_dr->height &&
            a.z >= tree->front_dr->z && a.z <= tree->front_dr->z+tree->front_dr->depth
           )
        {
            getPointCollisions (a,(tree->front_dr));
        }

        if (a.x >= tree->front_ul->x && a.x <= tree->front_ul->x+tree->front_ul->width &&
            a.y >= tree->front_ul->y && a.y <= tree->front_ul->y+tree->front_ul->height &&
            a.z >= tree->front_ul->z && a.z <= tree->front_ul->z+tree->front_ul->depth
            )
        {
            getPointCollisions (a,(tree->front_ul));
        }

        if (a.x >= tree->front_ur->x && a.x <= tree->front_ur->x+tree->front_ur->width &&
            a.y >= tree->front_ur->y && a.y <= tree->front_ur->y+tree->front_ur->height &&
            a.z >= tree->front_ur->z && a.z <= tree->front_ur->z+tree->front_ur->depth
            )
        {
            getPointCollisions (a,(tree->front_ur));
        }

        //BOTTOM

        if (a.x >= tree->back_dl->x && a.x <= tree->back_dl->x+tree->back_dl->width &&
            a.y >= tree->back_dl->y && a.y <= tree->back_dl->y+tree->back_dl->height &&
            a.z >= tree->back_dl->z && a.z <= tree->back_dl->z+tree->back_dl->depth
            )
        {
            getPointCollisions (a,(tree->back_dl));
        }

        if (a.x >= tree->back_dr->x && a.x <= tree->back_dr->x+tree->back_dr->width &&
            a.y >= tree->back_dr->y && a.y <= tree->back_dr->y+tree->back_dr->height &&
            a.z >= tree->back_dr->z && a.z <= tree->back_dr->z+tree->back_dr->depth
           )
        {
            getPointCollisions (a,(tree->back_dr));
        }

        if (a.x >= tree->back_ul->x && a.x <= tree->back_ul->x+tree->back_ul->width &&
            a.y >= tree->back_ul->y && a.y <= tree->back_ul->y+tree->back_ul->height &&
            a.z >= tree->back_ul->z && a.z <= tree->back_ul->z+tree->back_ul->depth
            )
        {
            getPointCollisions (a,(tree->back_ul));
        }

        if (a.x >= tree->back_ur->x && a.x <= tree->back_ur->x+tree->back_ur->width &&
            a.y >= tree->back_ur->y && a.y <= tree->back_ur->y+tree->back_ur->height &&
            a.z >= tree->back_ur->z && a.z <= tree->back_ur->z+tree->back_ur->depth
            )
        {
            getPointCollisions (a,(tree->back_ur));
        }


    }

}


void NGLScene::deleteAreaByAreaElements(Octree &tree)
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



    //Comment out some of the following Octree collision regions and PushButton on the Gui, to delete some of the 1st level Octree splits (1 of the 4 first quadrants of the Octree)
    if (tree.front_dl!=NULL)
    {
        update ();
    }
    if (tree.front_dr!=NULL)
    {
        update ();
    }
    if (tree.front_ul!=NULL)
    {
        update ();
    }
    if (tree.front_ur!=NULL)
    {
        update ();
    }

    //Bottom
    if (tree.back_dl!=NULL)
    {
        update ();
    }
    if (tree.back_dr!=NULL)
    {
        update ();
    }
    if (tree.back_ul!=NULL)
    {
        update ();
    }
    if (tree.back_ur!=NULL)
    {
        update ();
    }

}


void NGLScene::checkWallCollision(Point &point)
{
    //when cubes out of the root bounding voliume, reverse velocity
    if ( (point.x>10) || (point.y>10) || (point.z>10) || (point.x<-10) || (point.y<-10) || (point.z<-10) )
    {
        point.vx = - point.vx;
        point.vy = - point.vy;
        point.vz = - point.vz;
    }
}

void NGLScene::drawBranches(Octree *tree)
{

    if ( (tree->front_dl==NULL && tree->front_dr==NULL && tree->front_ul==NULL && tree->front_ur==NULL)
         && (tree->back_dl==NULL && tree->back_dr==NULL && tree->back_ul==NULL && tree->back_ur==NULL)
          )
    {
         treesize+=tree->container.size();

         ngl::Colour collisionNeighbourhoodAreaColour(ngl::Random::instance ()->randomPositiveNumber (), ngl::Random::instance ()->randomPositiveNumber (), ngl::Random::instance ()->randomPositiveNumber (), 1);
         for(int i=0;i<tree->container.size();i++)
         {

             //update pos based on artificial velocity
             //std::cout<<"moving..particle ID "<<tree->container[i].id<<'\n';

             tree->container[i].x+=0.1;
//             tree->container[i].y+=tree->container[i].vy;
//             tree->container[i].z+=tree->container[i].vz;

             //check wall collision

             checkWallCollision(/*tree,*/tree->container[i]);


             //Draw each particle
             static float rot=0;
             m_transform.setRotation(0,rot,0);rot+=20;
             m_transform.setPosition (tree->container[i].x, tree->container[i].y, tree->container[i].z);

             //make sure we are updating the treePositions vector for each one too
             //check where the  tree->container[i] is on the treePositions vector and update that particular element ot the treePositions vector
             std::vector<Point>::iterator element=std::find(treePositions.begin(), treePositions.end(),  tree->container[i]);
             if ( element !=treePositions.end ())// 'a' element found!!
             {
                 // now update the appropriate element on the treePositions vector (use the index of where it was found on the treePositions vectors)
                 auto index = std::distance(treePositions.begin(), element);
                 // update that particular element ot the treePositions vector
                 treePositions[index] =tree->container[i];
             }

             //now everything according to tree->container[i]
             loadMatricesToShader (m_transform,m_mouseGlobalTX, m_cam, collisionNeighbourhoodAreaColour);
             ngl::VAOPrimitives *prim=ngl::VAOPrimitives::instance ();
             prim->draw ("cube");

             std::cout<<"drew Branch with id: "<<tree->container[i].id<<",at pos: ("<<tree->container[i].x<<", "<<tree->container[i].y<<","<<tree->container[i].z<<")"<<std::endl;

         }

         return ;
    }
    if (tree->front_dl!=NULL && tree->front_dl->container.size()!=0)
        drawBranches(tree->front_dl);

    if (tree->front_dr!=NULL&&tree->front_dr->container.size()!=0)
        drawBranches(tree->front_dr);

     if (tree->front_ul!=NULL&&tree->front_ul->container.size()!=0)
        drawBranches(tree->front_ul);

     if (tree->front_ur!=NULL&&tree->front_ur->container.size()!=0)
        drawBranches(tree->front_ur);

     if (tree->back_dl!=NULL&&tree->back_dl->container.size()!=0)
        drawBranches(tree->back_dl);

     if (tree->back_dr!=NULL&&tree->back_dr->container.size()!=0)
        drawBranches(tree->back_dr);

     if (tree->back_ul!=NULL&&tree->back_ul->container.size()!=0)
        drawBranches(tree->back_ul);

     if (tree->back_ur!=NULL&&tree->back_ur->container.size()!=0)
        drawBranches(tree->back_ur);
}

#include <ngl/NGLStream.h>
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



    //reconstruct all the time
    treesize=0;
    tree->nexttreeID=0;
    tree.reset(nullptr);
    tree.reset(new Octree(0,0,0,treewidth,treeheight,treedepth));
    for(int i=0;i<totalCollisionObjects;i++)
    {
       //update x,y,z with position from the previous frame
       float x = treePositions[i].x;
       float y = treePositions[i].y;
       float z = treePositions[i].z;

       ngl::Vec3 tVel(treePositions[i].vx,treePositions[i].vy,treePositions[i].vz);

       if(treePositions[i].id==4)
       {
           std::cout<<"tVel="<<tVel<<'\n';
       }

       Point tempPoint(i,x,y,z,tVel.m_x,tVel.m_y,tVel.m_z);//or insert x,y instead of i,i to create some randomness
       tree->addPoint(tempPoint);
    }

    treesize=0;
    //draw all branches of the tree
    tree->countBranches();

    //std::cout<<"treesize="<<treesize<<'\n';

    //Solve collisions
    for(size_t i=0;i<totalCollisionObjects;i++)
    {
       getPointCollisions(treePositions[i],tree.get());
    }

    //Draw updated Tree
    treesize=0;
    drawBranches(tree.get());


        QString text;
        text.sprintf ("Framerate is %d", fps);
        m_text->setColour (0.2,0.6,0.5);
        m_text->renderText (10,10,text);


        m_frames++;
}


void NGLScene::testButtonClicked(bool b)
{
    emit clicked (b);
    //std::cout<<"Button Clicked - manual signal-slot connection"<<std::endl;
    // m_rColor=1;

    //delete some of the 1st level nodes of the tree
//    deleteAreaByAreaElements(tree);

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
