// -*- Mode: c++ -*-

#ifndef	__HALFEDGE_H__
#define	__HALFEDGE_H__

#include "cvec2t.h"
#include "cvec3t.h"
#include "cvec4t.h"

typedef CVec3T<float> Vec3f;
class Vertex{
public:
    float getx(){return(coord.x());}
    float gety(){return(coord.y());}
    float getz(){return(coord.z());}
    Vec3f coord;
    int id;
};

class Face{
public:
    //gets a normal vector to the face
    Vec3f getNormal() {return(cross(ver[1].coord - ver[0].coord, ver[2].coord - ver[1].coord));	}
    struct HalfEdge *listEdge;  // points to the first edge only
    struct Vertex *ver;  // use classes instead
    int numedges;
};

class HalfEdge{	
public:
    struct Vertex *head;     // next nead
    struct Face *thisface;      // face (to the left)
    struct HalfEdge *sym;
    struct HalfEdge *next;  // siguiente half edge
};

#endif
