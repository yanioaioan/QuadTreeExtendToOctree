#ifndef NGLSCENE_H
#define NGLSCENE_H

#include <ngl/Vec3.h>
#include <ngl/Mat4.h>
#include <ngl/Camera.h>
#include <ngl/Transformation.h>
#include <ngl/Colour.h>
#include <iostream>
#include <algorithm>
#include <ngl/Text.h>
#include <QOpenGLWidget>

#include <nglscene.h>
#include <ngl/Random.h>
#include <memory>

//unsigned static int maxCapacity;

const static int totalCollisionObjects=5000;

#define epsilon 1.0E-2


 struct Point
 {
     float id;
     float x,y,z;

     float radius;
     float vx,vy,vz;


     Point(){}
     Point(float _id,float _x,float _y, float _z, float _vx,float _vy, float _vz ):id(_id), x(_x),y(_y),z(_z), vx(_vx),vy(_vy),vz(_vz)
     {
         radius=1.0f;

             //velocity needs to be retained from the previous frame and updated accordingly
     }

     bool AreSame(float a, float b)const
     {
         bool val=fabs(a - b) < epsilon;
         return val;
     }

     bool operator==(const Point& a ) const {

//         return a.x==x && a.y==y && a.z==z;
//         return AreSame(a.x, x) && AreSame(a.y, y) && AreSame(a.z, z);

         //compare beased on id
         return a.id ==id;
         }

 };

 static size_t treesize;


 typedef struct Octree
 {
     Octree(const Octree& t){x=t.x; y=t.y; z=t.z;}

     std::vector<Point>container;
     std::shared_ptr<Octree> front_dl =NULL;
     std::shared_ptr<Octree> front_dr =NULL;
     std::shared_ptr<Octree> front_ul =NULL;
     std::shared_ptr<Octree> front_ur =NULL;
     std::shared_ptr<Octree> back_dl =NULL;
     std::shared_ptr<Octree> back_dr =NULL;
     std::shared_ptr<Octree> back_ul =NULL;
     std::shared_ptr<Octree> back_ur =NULL;
     float x,y,z, width, height, depth;
     int treeId;

     static int nexttreeID;

     Octree(float _x,float _y,float _z, float _width,float _height, float _depth)
     {

         nexttreeID++;
         treeId=nexttreeID;
         x =_x;
         y =_y;
         z = _z;
         width =_width;
         height =_height;
         depth = _depth;

     }

    //when maxCapacity is equal to 0,
     //then this means we contain everything in the root
     const size_t spliInNodes=totalCollisionObjects/2;
     //If points are evenly distributed then , the more subtrees the better as we end up with fewer tests.
     //On the contrary, if we have a non even distribution for which we test against, then this probably means we are
     //creating too much overhead for no reason at all, as we split space and don't put/allocate points in
     unsigned int maxCapacity=5;//totalCollisionObjects/spliInNodes;

     void countBranches()
     {
         if ( (front_dl==NULL &&front_dr==NULL &&front_ul==NULL &&front_ur==NULL)
              && (back_dl==NULL &&back_dr==NULL &&back_ul==NULL &&back_ur==NULL)
               )
         {
              treesize+=container.size();

              return;

         }
         if (front_dl!=NULL && front_dl->container.size()!=0)
             front_dl->countBranches();

         if (front_dr!=NULL&&front_dr->container.size()!=0)
             front_dr->countBranches();

          if (front_ul!=NULL&&front_ul->container.size()!=0)
             front_ul->countBranches();

          if (front_ur!=NULL&&front_ur->container.size()!=0)
             front_ur->countBranches();

          if (back_dl!=NULL&&back_dl->container.size()!=0)
             back_dl->countBranches();
          if (back_dr!=NULL&&back_dr->container.size()!=0)
             back_dr->countBranches();
          if (back_ul!=NULL&&back_ul->container.size()!=0)
             back_ul->countBranches();
          if (back_ur!=NULL&&back_ur->container.size()!=0)
             back_ur->countBranches();
     }

     void split()
     {
         float halfwidth =width /2;
         float halfheight =height /2;
         float halfdepth = depth /2;
         //create 4 sub-trees based on the width&height of the parent Octree

         //front quad
         front_dl.reset(new Octree(x,y,z, halfwidth, halfheight,halfdepth));
         front_dr.reset(new Octree(x +halfwidth,y, z, halfwidth,halfheight,halfdepth));
         front_ul.reset(new Octree(x,y +halfheight, z, halfwidth,halfheight,halfdepth));
         front_ur.reset(new Octree(x+halfwidth,y+halfheight, z, halfwidth,halfheight,halfdepth));

         //back quad
         back_dl.reset(new Octree(x,y, z+halfdepth, halfwidth, halfheight,halfdepth));
         back_dr.reset(new Octree(x+halfwidth, y, z+halfdepth, halfwidth, halfheight, halfdepth));
         back_ul.reset(new Octree(x,y +halfheight, z+halfdepth, halfwidth,halfheight ,halfdepth));
         back_ur.reset(new Octree(x+halfwidth,y+halfheight, z+halfdepth, halfwidth, halfheight, halfdepth));

         for(unsigned int i =0;i <container.size();i++)
         {
             front_dl->addPoint(container[i]);
             front_dr->addPoint(container[i]);
             front_ul->addPoint(container[i]);
             front_ur->addPoint(container[i]);


             back_dl->addPoint(container[i]);
             back_dr->addPoint(container[i]);
             back_ul->addPoint(container[i]);
             back_ur->addPoint(container[i]);

         }




     }


     void addPoint(Point &a)
     {
         //std::cout <<x <<std::endl;
         //ensure within the quad boundairies
         if(a.x >=x && a.y >=y && a.z >=z && a.x <x +width &&a.y <y +height && a.z <z +depth)
         {
             if(front_dl ==NULL)
             {
                 //add the this node's container the points inserted
                 container.push_back(a);
                 //when the container of this node has exceeded the max specified children..SPLIT()
                 if (container.size()>maxCapacity)
                     split();
             }
             else
             {
                 //if the area already has a point, then assign it the appropriate quadrant
                 front_dl->addPoint(a);
                 front_dr->addPoint(a);
                 front_ul->addPoint(a);
                 front_ur->addPoint(a);

                 back_dl->addPoint(a);
                 back_dr->addPoint(a);
                 back_ul->addPoint(a);
                 back_ur->addPoint(a);
             }
         }
     }


//     void getPointCollisions(Point &a, Octree *tree)
//     {
//         //if tree node is a leaf node
//         if ( (tree->ul==NULL && tree->ur==NULL && tree->dl==NULL && tree->dr==NULL) /*&&  std::find(tree->container.begin(), tree->container.end(), a) !=tree->container.end()*/ )
//         {
//             //found element in root tree
//             std::vector<Point>::iterator element=std::find(tree->container.begin(), tree->container.end(), a);
//             if( element !=tree->container.end ())//found element
//             {
//                 //print out THE WHOLE container
////                  for(std::vector<Point>::iterator iter=tree->container.begin (); iter!=tree->container.end (); iter++)
////                  {
////                      std::cout<<"Point="<<(*iter).x<<","<<(*iter).y<<std::endl;
////                  }

//                std::cout<<"Search Point="<<(*element).x<<","<<(*element).y<<std::endl;
//                for(std::vector<Point>::iterator iter=tree->container.begin (); iter!=tree->container.end (); iter++)
//                {
//                    if(!((*iter)==a))
//                        std::cout<<"Collision Neighbour Point="<<(*iter).x<<","<<(*iter).y<<std::endl;
//                }


//                  return;
//             }



//         }
//         else//if it's a branch , find if one of the 4 sub-areas intersects with the searched area
//         {

//             if (a.x >= tree->dl->x && a.x <= tree->dl->x+tree->dl->width &&
//                 a.y >= tree->dl->y && a.y <= tree->dl->y+tree->dl->height)
//             {
//                 getPointCollisions (a,(tree->dl));
//             }

//             if (a.x >= tree->dr->x && a.x <= tree->dr->x+tree->dr->width &&
//                 a.y >= tree->dr->y && a.y <= tree->dr->y+tree->dr->height)
//             {
//                 getPointCollisions (a,(tree->dr));
//             }

//             if (a.x >= tree->ul->x && a.x <= tree->ul->x+tree->ul->width &&
//                 a.y >= tree->ul->y && a.y <= tree->ul->y+tree->ul->height)
//             {
//                 getPointCollisions (a,(tree->ul));
//             }

//             if (a.x >= tree->ur->x && a.x <= tree->ur->x+tree->ur->width &&
//                 a.y >= tree->ur->y && a.y <= tree->ur->y+tree->ur->height)
//             {
//                 getPointCollisions (a,(tree->ur));
//             }



////             // Decides in which quadrant the pixel lies,
////                 // and delegates the method to the appropriate node.

////             //Left areas
////                 if(a.x < tree->x + tree->width/ 2)
////                 {
////                   //Up Left
////                   if(a.y > tree->y + tree->height / 2)
////                   {
////                     getPointCollisions (a,(tree->ul));
////                   }
////                   else//Down Left
////                   {
////                     getPointCollisions (a,(tree->dl));
////                   }
////                 }
////                 else//Right areas
////                 {
////                     //Up Right
////                   if(a.y > tree->y + tree->height / 2)
////                   {
////                    getPointCollisions (a,(tree->ur));
////                   }
////                   else//Down Right
////                   {
////                     getPointCollisions (a,(tree->dr));
////                   }
////                 }






////             //ul
////             if (a.x >=tree.ul->x &&a.y >=tree.ul->y &&a.x <tree.ul->x +tree.ul->width /2 && a.y <tree.ul->y +tree.ul->height /2)
////             {
////                 getPointCollisions (a,*(tree.ul));
////             }
////             //ur
////             if (a.x >=tree.ur->x+tree.ur->width /2 &&a.y >=tree.ur->y &&a.x <tree.ur->x +tree.ur->width /2 && a.y <tree.ur->y +tree.ur->height /2)
////             {
////                 getPointCollisions (a,*(tree.ur));
////             }
////             //dl
////             if (a.x >=tree.dl->x &&a.y >=tree.dl->y+tree.dl->height /2 &&a.x <tree.dl->x +tree.dl->width /2 && a.y <tree.dl->y +tree.dl->height /2)
////             {
////                 getPointCollisions (a,*(tree.dl));
////             }
////             //dr
////             if (a.x >=tree.dr->x+tree.dr->width /2  &&a.y >=tree.dr->y+tree.dr->height /2 &&a.x <tree.dr->x +tree.dr->width /2 && a.y <tree.dr->y +tree.dr->height /2)
////             {
////                 getPointCollisions (a,*(tree.dr));
////             }



////             //found element in ul subtree
////             if( std::find(tree.ul->container.begin(), tree.ul->container.end(), a) !=tree.ul->container.end() )
////             {
////                 //print out container
////                  for(std::vector<Point>::iterator iter=tree.ul->container.begin (); iter!=tree.ul->container.end (); iter++)
////                  {
////                      std::cout<<(*iter).x<<","<<(*iter).y<<std::endl;
////                  }
////             }

////             //found element in ur subtree
////             if( std::find(tree.ur->container.begin(), tree.ur->container.end(), a) !=tree.ur->container.end() )
////             {
////                 //print out container
////                  for(std::vector<Point>::iterator iter=tree.ur->container.begin (); iter!=tree.ur->container.end (); iter++)
////                  {
////                      std::cout<<(*iter).x<<","<<(*iter).y<<std::endl;
////                  }
////             }

////             //found element in dl subtree
////             if( std::find(tree.dl->container.begin(), tree.dl->container.end(), a) !=tree.dl->container.end() )
////             {
////                 //print out container
////                  for(std::vector<Point>::iterator iter=tree.dl->container.begin (); iter!=tree.dl->container.end (); iter++)
////                  {
////                      std::cout<<(*iter).x<<","<<(*iter).y<<std::endl;
////                  }
////             }

////             //found element in dr subtree
////             if( std::find(tree.dr->container.begin(), tree.dr->container.end(), a) !=tree.dr->container.end() )
////             {
////                 //print out container
////                  for(std::vector<Point>::iterator iter=tree.dr->container.begin (); iter!=tree.dr->container.end (); iter++)
////                  {
////                      std::cout<<(*iter).x<<","<<(*iter).y<<std::endl;
////                  }
////             }



//         }

//     }

 }Octree;
 //Binary Search Tree


class NGLScene: public QOpenGLWidget
{
    Q_OBJECT
public:
    static int treeID;

    NGLScene();
    ~NGLScene();

    float m_rColor;
    std::vector<Point> treePositions;


protected:
    void initializeGL ();
    void resizeGL (QResizeEvent *_event);
    void loadMatricesToShader(ngl::Transformation &_transform, const ngl::Mat4 &_globalTx, ngl::Camera *_cam, ngl::Colour &c);
    void detectAndResolveCollisions(Point &a, std::vector<Point> *collisionAreaPoints, const float &width, const float &height);
    void getPointCollisions(const Point a, std::shared_ptr<Octree> & tree);

    int getOctantContainingPoint(Point &point, Octree *tree) const ;

    void findTreeElements(Octree &tree);
    void deleteAreaByAreaElements(Octree &tree);
    void checkWallCollision(/*Octree *tree,*/ Point &point);

    void updatePosANDVelocityOfBranches(std::shared_ptr<Octree> &tree);

    void drawBranches(const std::shared_ptr<Octree> &tree);
    void saveNewPosBranches(const std::shared_ptr<Octree> &tree);



    void paintGL ();

    void timerEvent( QTimerEvent *_event );
    //----------------------------------------------------------------------------------------------------------------------
    void mouseMoveEvent (QMouseEvent * _event);
    //----------------------------------------------------------------------------------------------------------------------
    void mousePressEvent ( QMouseEvent * _event);
    //----------------------------------------------------------------------------------------------------------------------
    void mouseReleaseEvent ( QMouseEvent * _event );
    //----------------------------------------------------------------------------------------------------------------------
    void wheelEvent(QWheelEvent *_event);
    //----------------------------------------------------------------------------------------------------------------------

    void keyPressEvent(QKeyEvent *_event);

private:

    //----------------------------------------------------------------------------------------------------------------------
    /// @brief used to store the x rotation mouse value
    //----------------------------------------------------------------------------------------------------------------------
    int m_spinXFace;
    //----------------------------------------------------------------------------------------------------------------------
    /// @brief used to store the y rotation mouse value
    //----------------------------------------------------------------------------------------------------------------------
    int m_spinYFace;
    //----------------------------------------------------------------------------------------------------------------------
    /// @brief flag to indicate if the mouse button is pressed when dragging
    //----------------------------------------------------------------------------------------------------------------------
    bool m_rotate;
    //----------------------------------------------------------------------------------------------------------------------
    /// @brief flag to indicate if the Right mouse button is pressed when dragging
    //----------------------------------------------------------------------------------------------------------------------
    bool m_translate;
    //----------------------------------------------------------------------------------------------------------------------
    /// @brief the previous x mouse value
    //----------------------------------------------------------------------------------------------------------------------
    int m_origX;
    //----------------------------------------------------------------------------------------------------------------------
    /// @brief the previous y mouse value
    //----------------------------------------------------------------------------------------------------------------------
    int m_origY;
    //----------------------------------------------------------------------------------------------------------------------
    /// @brief the previous x mouse value for Position changes
    //----------------------------------------------------------------------------------------------------------------------
    int m_origXPos;
    //----------------------------------------------------------------------------------------------------------------------
    /// @brief the previous y mouse value for Position changes
    //----------------------------------------------------------------------------------------------------------------------
    int m_origYPos;
    //----------------------------------------------------------------------------------------------------------------------
    //----------------------------------------------------------------------------------------------------------------------
    /// @brief the model position for mouse movement
    //----------------------------------------------------------------------------------------------------------------------
    ngl::Vec3 m_modelPos;
    //----------------------------------------------------------------------------------------------------------------------
    /// @brief used to store the global mouse transforms
    //----------------------------------------------------------------------------------------------------------------------
    ngl::Mat4 m_mouseGlobalTX;
    int updateTimer;
    /// @brief flag to indicate if animation is active or not
    bool m_animate;
    ngl::Camera *m_cam;
    ngl::Transformation m_transform;
    ngl::Text *m_text;

    std::shared_ptr<Octree> tree;




signals:
    void clicked(bool);

public slots:
  void testButtonClicked(bool);


};

#endif // NGLSCENE_H
