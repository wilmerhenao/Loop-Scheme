// HOMEWORK 3

#include <iostream>
#include <cstring>
#ifdef _WIN32
#include <windows.h>
#endif
#include <fstream>
#include <math.h>
#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <vector>
#include <map>
#include "cvec3t.h"
#include "cvec4t.h"
#include "hmatrix.h"
#include "pair.h"
#include "halfedge.h"

#ifndef M_PI
#define M_PI            3.14159265358979323846	
#endif 
#define	BUFFER_SIZE 256
#define ZMIN 4294967295

typedef CVec4T<float> Vec4f;
typedef HMatrix<float> Maflo;

namespace WindowParams {
    static int WindowWidth = 800;
    static int WindowHeight = 600;
    static int MainWindow; 
};

//STL maps
typedef std::map<Pair, HalfEdge*, std::less<Pair> > EdgeMap;
typedef std::map<Pair, Vec3f, std::less<Pair> > NewVertexMap;
typedef std::map<Pair, int, std::less<Pair> > NewIndexMap;

int numfaces = 1;

EdgeMap edgemap;
vector<Vertex> vertixes;
vector<Face> faxes;
int selectedFace = -1 ;

Vec3f CameraPosition(4,-4,4);
Vec3f SphereCenter(0,0,0);
float SphereRadius = 2;
float ExaminerRotAngle = 0; 
Vec3f ExaminerRotAxis(-1,1,0); 
Maflo ExaminerRotation;

const char* objectfile[] = {"shuttle.obj", "cube.obj", "dodecahedron.obj", 
			    "icosahedron.obj", "tetrahedron.obj", "spaceship.obj"};
void Makeillus(void);
static void pickercalculator( int , int );

void Reshape(int width, int height) {
    WindowParams::WindowWidth = width;
    WindowParams::WindowHeight = height;
    glViewport(0,0,width,height);
    glMatrixMode(GL_PROJECTION); 
    glLoadIdentity();
    gluPerspective(40, width/float(height), 1, 10);

}

void Draw() {
    glMatrixMode(GL_MODELVIEW); 
    glLoadIdentity();
    gluLookAt(CameraPosition.x(),CameraPosition.y(),CameraPosition.z(),0,0,0,0,1,0);
    glClearColor( 0.0, 0.0, 0.0,0);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    // enable automatic rescaling of normals to unit length
    glEnable(GL_NORMALIZE);
    // enable two lights
    glEnable(GL_LIGHT0);
    //glEnable(GL_LIGHT1);
    // directional lights (w=0) along z axis
    glLightfv(GL_LIGHT0,GL_DIFFUSE, Vec4f(1,  1, 1,1));
    glLightfv(GL_LIGHT0,GL_POSITION, Vec4f(0,  0, 1,0));
    glLightfv(GL_LIGHT1,GL_DIFFUSE,Vec4f(1,  1, 1,1));
    glLightfv(GL_LIGHT1,GL_POSITION, Vec4f(0,  0, -1,0));

    glPushMatrix();

    glMultMatrixf(ExaminerRotation);
    glColor3f(1.0,1.0,1.0);
    Makeillus();
    glutWireCube(1.3 * SphereRadius);	
    glPopMatrix();
    glutSwapBuffers();
}


Vec3f ScreenToWorld(int windowid, int x, int y) { 
    glutSetWindow(windowid);
    GLdouble modelview[16];
    GLdouble projection[16];
    GLint viewport[4];
    double world_x, world_y, world_z; 
    // get current modelview, projection and viewport transforms
    glGetDoublev(GL_MODELVIEW_MATRIX,modelview);
    glGetDoublev(GL_PROJECTION_MATRIX,projection);
    glGetIntegerv(GL_VIEWPORT,viewport);
    // this function computes inverse of VPM and applies it to (x,y,0) to convert from pixel to world coords
    // this computes the world coordinates of the point on the near plane of the frustum which corresponds to pixel (x,y)
    gluUnProject(x,y,0,modelview,projection,viewport, &world_x,&world_y,&world_z);
    return Vec3f(world_x,world_y,world_z);
}


bool SpherePoint(const Vec3f& center, float r, const Vec3f& pscreen, Vec3f& psphere) { 
    Vec3f v = (pscreen- CameraPosition).dir(); 
    Vec3f d = CameraPosition-center;
    float ddotv = d.dot(v);
    float D = ddotv*ddotv-d.dot() +r*r;
    if (D < 0) return false;
    float t = -ddotv-sqrt(D);
    psphere = CameraPosition+v*t;
    return true;
}

// a mouse button is pressed or released
Vec3f CurrentPsphere;
Vec3f NewPsphere;

void MouseClick (int button, int state, int x, int y) {
    y = WindowParams::WindowHeight - y - 1;
    if(state == GLUT_DOWN) {
	Vec3f psphere; 
	if(SpherePoint(SphereCenter,SphereRadius,ScreenToWorld(WindowParams::MainWindow,x,y),psphere)) {
	    CurrentPsphere = psphere;
	    NewPsphere = psphere;
	}
	glutPostRedisplay();
	//selection
	pickercalculator(x, y);
    } 
    if(state == GLUT_UP) { 
	CurrentPsphere = NewPsphere;
    }
}

void MouseMotion(int x, int y) { 
    y = WindowParams::WindowHeight - y-1;
    Vec3f psphere;
    if(SpherePoint(SphereCenter,SphereRadius,ScreenToWorld(WindowParams::MainWindow,x,y),psphere)) {
	ExaminerRotAxis = cross(CurrentPsphere-SphereCenter, psphere-SphereCenter);
	ExaminerRotAngle = acos((CurrentPsphere-SphereCenter).dot(psphere-SphereCenter)/SphereRadius/SphereRadius);
	ExaminerRotation = Maflo::Rotation(ExaminerRotAngle,ExaminerRotAxis)*ExaminerRotation;
	CurrentPsphere = psphere;
    }
    glutPostRedisplay();
}
//draw it face by face
void Makeillus(void){		
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(CameraPosition.x(),CameraPosition.y(),CameraPosition.z(),0,0,0,0,1,0);	
    glPushMatrix();		
    glMultMatrixf(ExaminerRotation);
    glEnable(GL_LIGHTING);
    for(int i = 0; i < numfaces ; ++i){
	// the normal
	Vec3f normalvec = faxes[i].getNormal();
	//face name is assigned the same index
	glPushName((GLuint) i);
	//highlight selected face
	if(i == selectedFace){
	    glDisable(GL_LIGHTING);
	    glColor3f(0.0, 0.6, 0.5);
	}				
	glBegin(GL_POLYGON);
	for(int verit = 0; verit < faxes[i].numedges; ++verit){								
	    glNormal3f(normalvec.x(),normalvec.y(),normalvec.z());
	    glVertex3f(faxes[i].ver[verit].getx(),faxes[i].ver[verit].gety(),faxes[i].ver[verit].getz());
	}
	glEnd();
	glEnable(GL_LIGHTING);
	glPopName();
    }
    glDisable(GL_LIGHTING);
    glPopMatrix();
}

static void pickercalculator( int x, int y ){
//  Picking the faces
    GLint viewport[4];
    GLdouble hmatrix[16];
    GLuint pablor, currnam, finalizat;
    GLuint zminimum = ZMIN;
    GLuint bufferselected[BUFFER_SIZE];
    GLint picks;
    GLuint zmin = 0;
    
    glGetIntegerv( GL_VIEWPORT, viewport );
    
    glSelectBuffer( BUFFER_SIZE, bufferselected );
    GLuint *pointer = bufferselected;
    glRenderMode( GL_SELECT );
    glInitNames();

    glMatrixMode( GL_PROJECTION );
    glPushMatrix();
    glMatrixMode(GL_PROJECTION); 
    glLoadIdentity();
    gluPerspective(400, WindowParams::WindowWidth/float(WindowParams::WindowHeight), 1, 20);

    glGetDoublev( GL_PROJECTION_MATRIX, hmatrix );

    glLoadIdentity();
    gluPickMatrix( (float)x, (float)y,3, 3, viewport);
    glMultMatrixd( hmatrix );

    Makeillus();

    glMatrixMode( GL_PROJECTION );
    glPopMatrix();

    picks = glRenderMode( GL_RENDER );
    
    finalizat = -1;
    for(int i = 1; i <= picks; ++i ) {         
    	pablor = *pointer++;  
    	zmin = *pointer++;            
    	zmin = *pointer++;                      
    	for(int j = 1; j <= pablor; ++j ) {      
    	    currnam = *pointer++;                
    	    if(zmin < zminimum){
    		zminimum = zmin;
    		finalizat = currnam;	
    	    }
    	}    
    }
    if(-1 != finalizat && finalizat == selectedFace){
	    selectedFace = -1;
    }else if(-1 != finalizat)
		selectedFace = finalizat;
    
    glutPostRedisplay();
}	

void edgemapwork(void){
    EdgeMap::const_iterator iter;
    //set up halfedge sym pointer
    for(iter = edgemap.begin(); iter != edgemap.end(); iter++){
	EdgeMap::const_iterator iter1;
	EdgeMap::const_iterator iter2;
	Pair p1(iter->first.a,iter->first.b);
	Pair p2(iter->first.b,iter->first.a);
	iter1 = edgemap.find(p1);
	iter2 = edgemap.find(p2);
	iter2->second->sym = iter1->second;
    }	
}
 
//load the obj file and build classes
void filereader(const char * filename){	
    selectedFace = -1 ;
    numfaces = 1;
    vertixes.resize(0);
    faxes.resize(0);
    int nVertices=0;
    int nFaces=0;
    float x, y, z;
    int vi;
    HalfEdge *myhalfedge;
    char characters;

    ifstream file(filename, ios::in);
    if(!file){
	std::cerr << "File could not be found." << std::endl;
	exit(1);
    }

    // count faces and vertices
    while(file >> characters){
	if('f' == characters)
	    ++nFaces;	
	if('v' == characters)
	    ++nVertices;			
	else
	    std::cout << "Invalid" << std::endl;
	file.ignore(50,'\n');
    }
    file.clear();
    file.seekg(0);	//file position @ the beginning
    
    numfaces = nFaces;


    
    for(int i = 1 ; i <= nVertices; ++i){
	file >> characters;
	file >> x >> y >> z;
	file.ignore(50,'\n');		
	Vec3f v(x/4, y/4, z/4); //vertex
	Vertex myvertex;
	myvertex.coord = v;
	vertixes.push_back(myvertex); // save vertices
    }

    file.setf(ios::skipws); // skip the white space
    for(int i = 0; i < nFaces ; ++i){
	Face myface;

	std::vector<int> v;  // vector of vertex indices in this face
	std::vector<int>::const_iterator vit;
	file >> characters;
	while(file >> vi)					
	    v.push_back(vi);
	int j = 0;

	//vertex
	Vertex *myver;
	myver = new Vertex[v.size()];

	myface.ver = myver;
	faxes.push_back(myface); 
	faxes[i].numedges = v.size();
	myhalfedge = new HalfEdge[v.size()];

	for(vit = v.begin(); vit != v.end(); ++j, ++vit){			
	    faxes[i].ver[j] = vertixes[*vit - 1];   
	    faxes[i].ver[j].id = *vit;
	}
	faxes[i].listEdge = &myhalfedge[0]; j = 0;
	for(vit = v.begin() ; j < v.size(); ++j, vit++){
	    myhalfedge[j].head = &faxes[i].ver[(j+1)%v.size()];
	    myhalfedge[j].thisface = &faxes[i];
	    myhalfedge[j].next = &myhalfedge[(j+1)%v.size()];
	    
	    if(v.end() != (vit+1)){				
		Pair pair(*vit, *(vit+1));				
		edgemap.insert(EdgeMap::value_type(pair, &myhalfedge[j]));
	    }else{
		Pair pair(*vit, *v.begin());
		edgemap.insert(EdgeMap::value_type(pair, &myhalfedge[j]));
	    }		
	}
	
	file.clear();		
	//delete [] he;
    }
    edgemapwork();
}

void triangularizationte(){
    int newfacesnum = 0;	//number of faces @ the end
    Face newface;
    vector<Face> newFaces;
    int index = 0;
    HalfEdge mynewhalfedge[3];

    for(int i = 0 ; i < numfaces ; ++i){
	if(3 == faxes[i].numedges){
	    ++newfacesnum;
	} else{
	    newfacesnum += faxes[i].numedges - 2;}
    }

    for(int i = 0; i < newfacesnum; i++)
	newFaces.push_back(newface);

    for(int i = 0 ; i< numfaces; i++){
	if(3 == faxes[i].numedges){
	    //triangle
	    newFaces[index].numedges = 3;
	    newFaces[index].ver = new Vertex[3];
	    for(int k=0 ; k < 3; k++){
		newFaces[index].ver[k] = faxes[i].ver[k];
		newFaces[index].ver[k].id = faxes[i].ver[k].id;
	    }
	    ++index;
	} else {
	    //polygon to be split into triangles
	    for(int j = 0 ; j< faxes[i].numedges-2; j++){
		newFaces[index+j].numedges = 3;
		newFaces[index+j].ver = new Vertex[3];
		for(int k=0 ; k<3; k++){
		    if( k==0 ){
			newFaces[index+j].ver[k] = faxes[i].ver[0];
			newFaces[index+j].ver[k].id = faxes[i].ver[0].id;
		    } else{
			newFaces[index+j].ver[k] = faxes[i].ver[k+j];
			newFaces[index+j].ver[k].id = faxes[i].ver[k+j].id;
		    }
		}
	    }
	    index += faxes[i].numedges - 2;
	}
    }
 
    for(int i = 0; i < newfacesnum; ++i){
	newFaces[i].listEdge = &mynewhalfedge[0];
	for(int j = 0 ; j < 3; ++j){
	    mynewhalfedge[j].head = &newFaces[i].ver[(j+1) % 3];
	    mynewhalfedge[j].thisface = &newFaces[i];
	    mynewhalfedge[j].next = &mynewhalfedge[(j+1) % 3];
	    Pair pair(newFaces[i].ver[j].id, newFaces[i].ver[(j+1) % 3].id);			
	    edgemap.insert(EdgeMap::value_type(pair, &mynewhalfedge[j]));						
	}
    }
    edgemapwork();
    numfaces = newfacesnum;
    faxes = newFaces;
}

void loop_scheme(){
    int newfacesnum = 4 * numfaces;	//Every face will be split in 4
    vector<Face> newFaces;
    Face newface;
    int index = 0;	// index for new array of faces
    NewVertexMap newvertexmap;
    NewIndexMap newindexmap;
    for(int i = 0; i < newfacesnum; i++)
	newFaces.push_back(newface);

    for(int i = 0; i < numfaces; i++){
    	Vertex *newvertix = new Vertex[3];
	size_t newpositions[3] = {0};
    	for(int j = 0; j < 3; j++){
    	    newvertix[j].coord = (faxes[i].ver[j].coord + faxes[i].ver[(j+1)%3].coord)/2;
    	    Pair pair1(faxes[i].ver[j].id, faxes[i].ver[(j+1)%3].id);	
    	    Pair pair2(faxes[i].ver[(j+1)%3].id, faxes[i].ver[j].id);
    	    if(faxes[i].ver[(j+1)%3].id > faxes[i].ver[j].id){
    		newvertexmap.insert(NewVertexMap::value_type(pair1, newvertix[j].coord));
    	    } else {		
    		newvertexmap.insert(NewVertexMap::value_type(pair2, newvertix[j].coord));
    	    }
	    newpositions[j] = vertixes.size();
	    vertixes.push_back(newvertix[j]);
    	}
	newFaces[4 * i].ver = new Vertex[3]; newFaces[4 * i].numedges = 3;
	newFaces[4 * i + 1].ver = new Vertex[3]; newFaces[4 * i + 1].numedges = 3;
	newFaces[4 * i + 2].ver = new Vertex[3]; newFaces[4 * i + 2].numedges = 3;
	newFaces[4 * i + 3].ver = new Vertex[3]; newFaces[4 * i + 3].numedges = 3;
	// Assign vertices
	newFaces[4 * i].ver[0] = faxes[i].ver[0]; newFaces[4 * i].ver[1] = newvertix[0]; newFaces[4 * i].ver[2] = newvertix[2];
	newFaces[4 * i + 1].ver[0] = newvertix[0]; newFaces[4 * i + 1].ver[1] = faxes[i].ver[1]; newFaces[4 * i + 1].ver[2] = newvertix[1];
	newFaces[4 * i + 2].ver[0] = newvertix[1]; newFaces[4 * i + 2].ver[1] = faxes[i].ver[2]; newFaces[4 * i + 2].ver[2] = newvertix[2];
	newFaces[4 * i + 3].ver[0] = newvertix[0]; newFaces[4 * i + 3].ver[1] = newvertix[1]; newFaces[4 * i + 3].ver[2] = newvertix[2];
	
    }
    // for(int i = 0 ; i< numfaces; i++){
    // 	Vec3f *newvertix = new Vec3f[3];
    // 	for(int j = 0; j < 3; j++){
    // 	    newvertix[j] = (faces[i].ver[j].coord + faces[i].ver[(j+1)%3].coord)/2;
    // 	    Pair pair1(faces[i].ver[j].id, faces[i].ver[(j+1)%3].id);	
    // 	    Pair pair2(faces[i].ver[(j+1)%3].id, faces[i].ver[j].id);
    // 	    if(faces[i].ver[(j+1)%3].id > faces[i].ver[j].id){
    // 		newvertexmap.insert(NewVertexMap::value_type(pair1, newvertix[j]));
    // 	    } else {		
    // 		newvertexmap.insert(NewVertexMap::value_type(pair2, newvertix[j]));
    // 	    }
    // 	} 
    // }

    // // Lets assign the new vertices and values
    // NewVertexMap::const_iterator iter;
    // for(iter = newvertexmap.begin(); iter != newvertexmap.end(); iter++){
    // 	newindexmap.insert(NewIndexMap::value_type(iter->first, (int)vertixes.size())); // contains the position
    // 	vertixes.push_back(newvertexmap.find(iter->first));
    // }

    // for(int i = 0; i < numfaces; i++){
	
    // }

    //Fix at the end
    numfaces = newfacesnum;
    faxes = newFaces;
    selectedFace = -1;
}

void facedeleter(){
    if(selectedFace >= 0){

	for(int i= selectedFace; i < numfaces-1 ; i++){
	    // basically assign it to the next face (can't delete last face though)
	    faxes[i].listEdge = faxes[i+1].listEdge;
	    faxes[i].ver = faxes[i+1].ver;
	    faxes[i].numedges = faxes[i+1].numedges;
	}
	faxes.pop_back(); // pop one out calling the destructor
	// delete pointers to this face
	for(int i = 0; i < faxes[selectedFace].numedges; i++){
	    Pair p(faxes[selectedFace].ver[(i+1) % (faxes[selectedFace].numedges)].id, faxes[selectedFace].ver[i].id);
	    EdgeMap::const_iterator iter;
	    iter = edgemap.find(p);
	    iter->second->thisface = NULL;
	}
	--numfaces;
	selectedFace = -1;
    }
}

void keyboard(unsigned char c, int x, int y){
    switch(c){
    case '1':
	filereader(objectfile[0]);
	glutPostRedisplay();
	break;
    case '2':
	filereader(objectfile[1]);
	glutPostRedisplay();
	break;
    case '3':  
	filereader(objectfile[2]);
	glutPostRedisplay();
	break;
    case '4':
	filereader(objectfile[3]);
	glutPostRedisplay();
	break;
    case '5':
	filereader(objectfile[4]);
	glutPostRedisplay();
	break;
    case '6':   
	filereader(objectfile[5]);
	glutPostRedisplay();
	break;
    case 't':
    case 'T':
	triangularizationte();
        glutPostRedisplay();
    break;
    case 'd':
    case 'D':
	facedeleter();
        glutPostRedisplay();
    break;
    case 's':
    case 'S':
	triangularizationte();
        loop_scheme();
        glutPostRedisplay();
    break;
    case 27:
	exit(0);
	break;
    default:
	break;
    }
}

int main(int argc, char* argv[]) {
    filereader(objectfile[0]);
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA|GLUT_DOUBLE|GLUT_DEPTH);
    glutInitWindowSize(WindowParams::WindowWidth,WindowParams::WindowHeight);
    WindowParams::MainWindow = glutCreateWindow("Homework 3: Meshes and subdivision");
    glutDisplayFunc(Draw); 
    glutReshapeFunc(Reshape);
    glutMouseFunc(MouseClick);
    glutMotionFunc(MouseMotion);
    glutKeyboardFunc(keyboard);
    glutMainLoop();
    return 0;
}
