/*
compiling:
	gcc -Wall -o sphere sphere.c -lglut -lGL -lGLU -lm
*/

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#define PI 3.1415
#define KEY_SPACE 32
#define KEY_TAB 9
#define fontHelvetica GLUT_BITMAP_HELVETICA_18

#define WIDTH 180
#define HEIGHT 220
#define CAMERA_DISTANCE 350
#define SPHERE_RADIUS 100
#define CUBE_RADIUS 70
#define LINE_MODIFIER 1.005
#define MAX_SELECTION 10
#define ROTATION_SPEED 1.0
#define MOUSE_ROTATION_SPEED 250.0
#define MOVE_SPEED 2.0
#define DRAG_MOVE_SPEED 50.0
#define MIN_MOVE_ANGLE 5.0
#define DETAIL 5
#define VALUE pow(3.0, -0.5)

typedef struct{
	float x, y, z;
}Vertex, Vector;

typedef struct{
	GLdouble x, y, z;
}WindowVertex;

typedef struct{
	float red, green, blue;
}Color;

struct{
	struct{
		Vertex vertex[3], average;
		WindowVertex window[3];
		Color color;
	}triangle[24];
	struct{
		Vector vector;
		float angle;
		int direction;
		int flag;
	}move;
	int key[256], skey[256];
	struct{
		int x, y;
		int selected;
		int button[3];
		struct{
			int x, y;
		}click;
		struct{
			int x, y;
		}lastDrag;
	}mouse;
	struct{
		int width, height;
	}screen;
	float rotation[3];
	double rotationMatrix[16];
	int drawType;
	int format;
}Game;

GLfloat fAspect;
GLint fovy_=0;

float mod(float x, float y){
	while(x>=y){
		x -= y;
	}
	while(x<0){
		x += y;
	}
	return x;
}

float vectorModule(Vector v){
	return pow(v.x*v.x + v.y*v.y + v.z*v.z, 0.5);
}

Vector normalize(Vector v){
	float module = vectorModule(v);
	if(module==0.0) return v;
	v.x /= module;
	v.y /= module;
	v.z /= module;
	return v;
}

float scalarProduct(Vector v1, Vector v2){
	float module1, module2;
	module1 = vectorModule(v1);
	module2 = vectorModule(v2);
	if(module1==0 || module2==0) return 0;
	return (v1.x*v2.x + v1.y*v2.y + v1.z*v2.z)/(module1*module2);
}

Vector vectorialInversion(Vector v1, Vector v2){
	Vector v;
	float scalar = scalarProduct(v1, v2);
	float module = vectorModule(v1);
	v2 = normalize(v2);
	v.x = 2*(module*scalar)*v2.x - v1.x;
	v.y = 2*(module*scalar)*v2.y - v1.y;
	v.z = 2*(module*scalar)*v2.z - v1.z;
	return v;
}

Vector vectorialProduct(Vector v1, Vector v2){
	Vector v;
	v1 = normalize(v1);
	v2 = normalize(v2);
	v.x = v1.y*v2.z - v1.z*v2.y;
	v.y = v1.z*v2.x - v1.x*v2.z;
	v.z = v1.x*v2.y - v1.y*v2.x;
	return v;
}

void initialize(void){
	GLfloat ambient[4]={0.2,0.2,0.2,1.0};
	GLfloat diffuse[4]={0.7,0.7,0.7,0.5};
	GLfloat specular[4]={1.0, 1.0, 1.0, 1.0};
	GLfloat position[4]={30, 30, -0.8*CAMERA_DISTANCE, 1.0};
	GLfloat specularity[4]={1.0,1.0,1.0,1.0};
	GLint especMaterial = 50;
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glShadeModel(GL_SMOOTH);
	glMaterialfv(GL_FRONT,GL_SPECULAR, specularity);
	glMateriali(GL_FRONT,GL_SHININESS,especMaterial);
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient); 
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
	glLightfv(GL_LIGHT0, GL_POSITION, position );
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_NORMALIZE);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_DEPTH_TEST);
	fovy_=45;
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
  	glGetDoublev(GL_MODELVIEW_MATRIX, Game.rotationMatrix);
}

void configVision(void){
	glLineWidth(4.0);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(fovy_,fAspect,0.4,500);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void changeWindowSize(GLsizei w, GLsizei h){
	if ( h == 0 ) h = 1;
    glViewport(0, 0, w, h);
	fAspect = (GLfloat)w/(GLfloat)h;
	configVision();
	Game.screen.width = w;
	Game.screen.height = h;
}

void resetGame(void){
	Vertex vertex[6][5]={0}, tmp;
	int i,j,k;
	
	Color color[6];
	color[0].red = 0.0;
	color[0].green = 1.0;
	color[0].blue = 0.0;
	color[1].red = 0.0;
	color[1].green = 0.0;
	color[1].blue = 1.0;
	color[2].red = 1.0;
	color[2].green = 1.0;
	color[2].blue = 1.0;
	color[3].red = 1.0;
	color[3].green = 1.0;
	color[3].blue = 0.0;
	color[4].red = 1.0;
	color[4].green = 0.0;
	color[4].blue = 0.0;
	color[5].red = 1.0;
	color[5].green = 0.75;
	color[5].blue = 0.0;
	
	vertex[0][0].x = -1.0;
	vertex[1][0].x = 1.0;
	vertex[2][0].y = -1.0;
	vertex[3][0].y = 1.0;
	vertex[4][0].z = -1.0;
	vertex[5][0].z = 1.0;
	for(i=0; i<6; i++){
		for(j=1; j<5; j++){
			if(vertex[i][0].x != 0){
				vertex[i][j].x = VALUE*vertex[i][0].x;
				vertex[i][j].y = VALUE*pow(-1, (int)(j+1)/2);
				vertex[i][j].z = VALUE*pow(-1, (int)j/2);
			}
			if(vertex[i][0].y != 0){
				vertex[i][j].y = VALUE*vertex[i][0].y;
				vertex[i][j].z = VALUE*pow(-1, (int)(j+1)/2);
				vertex[i][j].x = VALUE*pow(-1, (int)j/2);
			}
			if(vertex[i][0].z != 0){
				vertex[i][j].z = VALUE*vertex[i][0].z;
				vertex[i][j].x = VALUE*pow(-1, (int)(j+1)/2);
				vertex[i][j].y = VALUE*pow(-1, (int)j/2);
			}
		}
	}
	
	for(i=0; i<6; i++){
		for(j=0; j<4; j++){
			Game.triangle[i*4+j].color = color[i];
			Game.triangle[i*4+j].vertex[0] = vertex[i][0];
			Game.triangle[i*4+j].vertex[1] = vertex[i][j+1];
			if(i%2==0){
				tmp = Game.triangle[i*4+j].vertex[0];
				Game.triangle[i*4+j].vertex[0] = Game.triangle[i*4+j].vertex[1];
				Game.triangle[i*4+j].vertex[1] = tmp;
			}
			Game.triangle[i*4+(int)mod(j-1,4)].vertex[2] = vertex[i][j+1];
		}
	}
	
	for(i=0; i<6; i++){
		for(j=0; j<4; j++){
			Game.triangle[i*4+j].average.x = 0.0;
			Game.triangle[i*4+j].average.y = 0.0;
			Game.triangle[i*4+j].average.z = 0.0;
			for(k=0; k<3; k++){
				Game.triangle[i*4+j].average.x += Game.triangle[i*4+j].vertex[k].x;
				Game.triangle[i*4+j].average.y += Game.triangle[i*4+j].vertex[k].y;
				Game.triangle[i*4+j].average.z += Game.triangle[i*4+j].vertex[k].z;
			}
			Game.triangle[i*4+j].average.x /= 3.0;
			Game.triangle[i*4+j].average.y /= 3.0;
			Game.triangle[i*4+j].average.z /= 3.0;
		}
	}
	
	Game.move.angle = 0.0;
	Game.move.vector.x = 0.0;
	Game.move.vector.y = 0.0;
	Game.move.vector.z = 0.0;
	for(i=0; i<3; i++) Game.rotation[i] = 0.0;
	Game.drawType = 0;
	Game.mouse.selected = -1;
	Game.move.flag = 0;
}

void normalizePositionCube(Vertex *vertex){
	float p[3] = {-1.0, 0.0, 1.0};
	float diff, bestDiff, bestP;
	int i;
	
	bestDiff = 999;
	for(i=0; i<3; i++){
		diff = (p[i]-vertex->x)*(p[i]-vertex->x);
		if(diff<bestDiff){
			bestDiff = diff;
			bestP = p[i];
		}
	}
	vertex->x = bestP;
	
	bestDiff = 999;
	for(i=0; i<3; i++){
		diff = (p[i]-vertex->y)*(p[i]-vertex->y);
		if(diff<bestDiff){
			bestDiff = diff;
			bestP = p[i];
		}
	}
	vertex->y = bestP;
	
	bestDiff = 999;
	for(i=0; i<3; i++){
		diff = (p[i]-vertex->z)*(p[i]-vertex->z);
		if(diff<bestDiff){
			bestDiff = diff;
			bestP = p[i];
		}
	}
	vertex->z = bestP;
}

void normalizePosition(Vertex *vertex){
	float p[5] = {-1.0, -VALUE, 0.0, VALUE, 1.0};
	float diff, bestDiff, bestP;
	int i;
	
	bestDiff = 999;
	for(i=0; i<5; i++){
		diff = (p[i]-vertex->x)*(p[i]-vertex->x);
		if(diff<bestDiff){
			bestDiff = diff;
			bestP = p[i];
		}
	}
	vertex->x = bestP;
	
	bestDiff = 999;
	for(i=0; i<5; i++){
		diff = (p[i]-vertex->y)*(p[i]-vertex->y);
		if(diff<bestDiff){
			bestDiff = diff;
			bestP = p[i];
		}
	}
	vertex->y = bestP;
	
	bestDiff = 999;
	for(i=0; i<5; i++){
		diff = (p[i]-vertex->z)*(p[i]-vertex->z);
		if(diff<bestDiff){
			bestDiff = diff;
			bestP = p[i];
		}
	}
	vertex->z = bestP;
}

void drawTriangle(int detail, Vertex v0, Vertex v1, Vertex v2){
	int i;
	float module;
	Vertex vertex[3] = {v0, v1, v2}, nv[3];
	--detail;
	if(detail==0){
		glBegin(GL_TRIANGLES);
		if(Game.format==0){
			for(i=0; i<3; i++){
				glNormal3f(-vertex[i].x, -vertex[i].y, -vertex[i].z);
				glVertex3f(vertex[i].x*SPHERE_RADIUS, vertex[i].y*SPHERE_RADIUS, vertex[i].z*SPHERE_RADIUS);
			}
		}else{
			for(i=0; i<3; i++){
				normalizePositionCube(&vertex[i]);
				glNormal3f(-vertex[i].x, -vertex[i].y, -vertex[i].z);
				glVertex3f(vertex[i].x*CUBE_RADIUS, vertex[i].y*CUBE_RADIUS, vertex[i].z*CUBE_RADIUS);
			}
		}
		glEnd();
	}else{
		for(i=0; i<3; i++){
			nv[i].x = vertex[i].x + vertex[(i+1)%3].x;
			nv[i].y = vertex[i].y + vertex[(i+1)%3].y;
			nv[i].z = vertex[i].z + vertex[(i+1)%3].z;
			module = vectorModule(nv[i]);
			nv[i].x /= module;
			nv[i].y /= module;
			nv[i].z /= module;
		}
		drawTriangle(detail, v0, nv[0], nv[2]);
		drawTriangle(detail, nv[0], v1, nv[1]);
		drawTriangle(detail, nv[2], nv[1], v2);
		drawTriangle(detail, nv[0], nv[1], nv[2]);
	}
}

void drawLine(int detail, Vertex v0, Vertex v1){
	int i;
	float module;
	Vertex nv;
	--detail;
	if(detail==0){
		if(Game.format==0){
			glBegin(GL_LINES);
				glNormal3f(-v0.x, -v0.y, -v0.z);
				glVertex3f(v0.x*LINE_MODIFIER*SPHERE_RADIUS, v0.y*LINE_MODIFIER*SPHERE_RADIUS, v0.z*LINE_MODIFIER*SPHERE_RADIUS);
				glNormal3f(-v1.x, -v1.y, -v1.z);
				glVertex3f(v1.x*LINE_MODIFIER*SPHERE_RADIUS, v1.y*LINE_MODIFIER*SPHERE_RADIUS, v1.z*LINE_MODIFIER*SPHERE_RADIUS);
				glNormal3f(-v0.x, -v0.y, -v0.z);
				glVertex3f(v0.x*LINE_MODIFIER*SPHERE_RADIUS, v0.y*LINE_MODIFIER*SPHERE_RADIUS, v0.z*LINE_MODIFIER*SPHERE_RADIUS);
			glEnd();
		}else{
			glBegin(GL_LINES);
				normalizePositionCube(&v0);
				normalizePositionCube(&v1);
				glNormal3f(-v0.x, -v0.y, -v0.z);
				glVertex3f(v0.x*LINE_MODIFIER*CUBE_RADIUS, v0.y*LINE_MODIFIER*CUBE_RADIUS, v0.z*LINE_MODIFIER*CUBE_RADIUS);
				glNormal3f(-v1.x, -v1.y, -v1.z);
				glVertex3f(v1.x*LINE_MODIFIER*CUBE_RADIUS, v1.y*LINE_MODIFIER*CUBE_RADIUS, v1.z*LINE_MODIFIER*CUBE_RADIUS);
				glNormal3f(-v0.x, -v0.y, -v0.z);
				glVertex3f(v0.x*LINE_MODIFIER*CUBE_RADIUS, v0.y*LINE_MODIFIER*CUBE_RADIUS, v0.z*LINE_MODIFIER*CUBE_RADIUS);
			glEnd();
		}
	}else{
		nv.x = v0.x + v1.x;
		nv.y = v0.y + v1.y;
		nv.z = v0.z + v1.z;
		module = vectorModule(nv);
		nv.x /= module;
		nv.y /= module;
		nv.z /= module;
		drawLine(detail, v0, nv);
		drawLine(detail, nv, v1);
	}
}

void draw(void){
	int i, j;
	float color, scalar;
	Vector v;
	Vertex tmp;
	GLdouble modelView[16];
	GLdouble projection[16];
	GLint viewport[4];
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	
	//Update rotation matrix
	glLoadIdentity();
	glRotatef(Game.rotation[0], 1.0, 0.0, 0.0);
	glRotatef(Game.rotation[1], 0.0, 1.0, 0.0);
	glRotatef(Game.rotation[2], 0.0, 0.0, 1.0);
  	glMultMatrixd(Game.rotationMatrix);
  	glGetDoublev(GL_MODELVIEW_MATRIX, Game.rotationMatrix);
	for(i=0; i<3; i++) Game.rotation[i] = 0.0;
	
	//Do the rest
	glLoadIdentity();
  	glTranslatef(0.0, 0.0, -CAMERA_DISTANCE);
  	glMultMatrixd(Game.rotationMatrix);
  	for(i=0; i<24; i++){
		glPushMatrix();
			if(Game.move.flag || (Game.mouse.selected>=0 && Game.move.angle*Game.move.direction>=MIN_MOVE_ANGLE)){
				v = Game.triangle[i].average;
				scalar = scalarProduct(v, Game.move.vector);
				if(scalar>0.0) glRotatef(Game.move.angle, Game.move.vector.x, Game.move.vector.y, Game.move.vector.z);
			}
			
			glGetDoublev(GL_MODELVIEW_MATRIX, modelView);
			glGetDoublev(GL_PROJECTION_MATRIX, projection);
			glGetIntegerv(GL_VIEWPORT, viewport);
			
			if(Game.drawType==0){
				glColor3f(Game.triangle[i].color.red, Game.triangle[i].color.green, Game.triangle[i].color.blue);
			}else{
				color = (i+1.5)/25.0;
				glColor3f(color, color, color);
			}
			drawTriangle(DETAIL, Game.triangle[i].vertex[0], Game.triangle[i].vertex[1], Game.triangle[i].vertex[2]);
			
			if(Game.mouse.selected==-1){
				for(j=0; j<3; j++){
					tmp = Game.triangle[i].vertex[j];
					if(Game.format==0){
						gluProject(tmp.x*SPHERE_RADIUS, tmp.y*SPHERE_RADIUS, tmp.z*SPHERE_RADIUS, modelView, projection, viewport, &Game.triangle[i].window[j].x, &Game.triangle[i].window[j].y, &Game.triangle[i].window[j].z);
					}else{
						normalizePositionCube(&tmp);
						gluProject(tmp.x*CUBE_RADIUS, tmp.y*CUBE_RADIUS, tmp.z*CUBE_RADIUS, modelView, projection, viewport, &Game.triangle[i].window[j].x, &Game.triangle[i].window[j].y, &Game.triangle[i].window[j].z);
					}
				}
			}
			
			glColor3f(0.0, 0.0, 0.0);
			drawLine(DETAIL, Game.triangle[i].vertex[0], Game.triangle[i].vertex[1]);
			drawLine(DETAIL, Game.triangle[i].vertex[1], Game.triangle[i].vertex[2]);
			drawLine(DETAIL, Game.triangle[i].vertex[2], Game.triangle[i].vertex[0]);
        glPopMatrix();
	}
	
 	if(Game.drawType==0) glutSwapBuffers();
}

int select(int x, int y){
	int ret;
	Game.drawType = 1;
	glDisable(GL_LIGHTING);
	draw();
	
	unsigned char color[3];
	glReadPixels(x , y , 1 , 1 , GL_RGB , GL_UNSIGNED_BYTE , color);
	ret = 25*color[1]/255 - 1;
	
	Game.drawType = 0;
	glEnable(GL_LIGHTING);
	
	return ret;
}

void applyMove(void){
	Vector v;
	float scalar;
	int i,j;
	for(i=0; i<24; i++){
		v = Game.triangle[i].average;
		scalar = scalarProduct(v, Game.move.vector);
		if(scalar>0.0){
			Game.triangle[i].average.x = 0.0;
			Game.triangle[i].average.y = 0.0;
			Game.triangle[i].average.z = 0.0;
			for(j=0; j<3; j++){
				Game.triangle[i].vertex[j] = vectorialInversion(Game.triangle[i].vertex[j], Game.move.vector);
				normalizePosition(&Game.triangle[i].vertex[j]);
				Game.triangle[i].average.x += Game.triangle[i].vertex[j].x;
				Game.triangle[i].average.y += Game.triangle[i].vertex[j].y;
				Game.triangle[i].average.z += Game.triangle[i].vertex[j].z;
			}
			Game.triangle[i].average.x /= 3.0;
			Game.triangle[i].average.y /= 3.0;
			Game.triangle[i].average.z /= 3.0;
		}
	}
}

void shuffle(void){
	int i, triangle, vertex1, vertex2;
	Vector a[2];
	triangle = rand()%24;
	vertex1 = rand()%3;
	do{
		vertex2 = rand()%3;
	}while(vertex2==vertex1);
	a[1] = Game.triangle[triangle].vertex[vertex1];
	a[2] = Game.triangle[triangle].vertex[vertex2];
	Game.move.vector = vectorialProduct(a[1], a[2]);
	applyMove();
}

void swapFormat(void){
	int i,j;
	Game.format = !Game.format;
}

void controls(void){
	if(Game.mouse.selected==-1){
		if(Game.key['W'] || Game.key['w']) Game.rotation[0] = ROTATION_SPEED;
		if(Game.key['S'] || Game.key['s']) Game.rotation[0] = -ROTATION_SPEED;
		if(Game.key['A'] || Game.key['a']) Game.rotation[1] = ROTATION_SPEED;
		if(Game.key['D'] || Game.key['d']) Game.rotation[1] = -ROTATION_SPEED;
		if(Game.key['Q'] || Game.key['q']) Game.rotation[2] = ROTATION_SPEED;
		if(Game.key['E'] || Game.key['e']) Game.rotation[2] = -ROTATION_SPEED;
		if(!Game.move.flag){
			if(Game.key['R'] || Game.key['r']) shuffle();
			if(Game.key[KEY_SPACE]) resetGame();
		}
	}
}

void game_loop(int dt){
	controls();
	glutPostRedisplay();
	glutTimerFunc(10, game_loop, dt);
	if(Game.move.flag){
		Game.move.angle += Game.move.direction*MOVE_SPEED;
		if(Game.move.angle>=180 || Game.move.angle<=-180){
			applyMove();
			Game.move.flag = 0;
		}
	}
}

void keyboardDown(unsigned char key, int x, int y){
	//printf("k%d\n", key);
	if(Game.mouse.selected==-1){
		if(key==KEY_TAB && Game.key[key]==0) swapFormat();
	}
	Game.key[key] = 1;
}

void keyboardUp(unsigned char key, int x, int y){
	Game.key[key] = 0;
}

void keyboardSpecialDown(int key, int x, int y){
	//printf("s%d\n", key);
	Game.skey[key] = 1;
}

void keyboardSpecialUp(int key, int x, int y){
	Game.skey[key] = 0;
}

void mouseMove(int x, int y){
	Vector m, v[3], a[2], av;
	int i, maxId, negativeFlag, maxNegative;
	float scalar, maxScalar = -1, maxModule;
	
	Game.mouse.x = x;
	Game.mouse.y = Game.screen.height-y;
	
	if(Game.mouse.selected==-1 && Game.mouse.button[2]){
		Game.rotation[0] = (Game.mouse.lastDrag.y-Game.mouse.y)*MOUSE_ROTATION_SPEED/Game.screen.height;
		Game.rotation[1] = -(Game.mouse.lastDrag.x-Game.mouse.x)*MOUSE_ROTATION_SPEED/Game.screen.height;
		Game.mouse.lastDrag.x = Game.mouse.x;
		Game.mouse.lastDrag.y = Game.mouse.y;
	}
	
	if(Game.mouse.selected>=0 && Game.mouse.button[0]){
		Game.mouse.x = x;
		Game.mouse.y = Game.screen.height-y;
		m.x = Game.mouse.x - Game.mouse.click.x;
		m.y = Game.mouse.y - Game.mouse.click.y;
		m.z = 0.0;
		for(i=0; i<3; i++){
			v[i].x = Game.triangle[Game.mouse.selected].window[(i+1)%3].x - Game.triangle[Game.mouse.selected].window[i].x;
			v[i].y = Game.triangle[Game.mouse.selected].window[(i+1)%3].y - Game.triangle[Game.mouse.selected].window[i].y;
			v[i].z = 0.0;
			scalar = scalarProduct(m, v[i]);
			if(scalar<0){
				scalar *= -1;
				negativeFlag = 1;
			}else{
				negativeFlag = 0;
			}
			if(scalar>maxScalar){
				maxScalar = scalar;
				maxId = i;
				maxNegative = negativeFlag;
			}
		}
		maxModule = vectorModule(v[maxId]);
		a[1] = Game.triangle[Game.mouse.selected].vertex[maxId];
		a[2] = Game.triangle[Game.mouse.selected].vertex[(maxId+1)%3];
		av.x = a[1].x - a[2].x;
		av.y = a[1].y - a[2].y;
		av.z = a[1].z - a[2].z;
		Game.move.vector = vectorialProduct(a[1], a[2]);
		Game.move.angle = DRAG_MOVE_SPEED*vectorModule(av)*vectorModule(m)/maxModule;
		if(maxNegative) Game.move.direction = -1;
		else Game.move.direction = 1;
		Game.move.angle *= Game.move.direction;
	}
}

void mouseClick(int button, int state, int x, int y){
	Game.mouse.button[button] = !state;
	if(!Game.move.flag){
		if(button==0){
			if(Game.mouse.button[button]){
				Game.mouse.click.x = x;
				Game.mouse.click.y = Game.screen.height-y;
				Game.mouse.selected = select(Game.mouse.click.x, Game.mouse.click.y);
			}
		}
	}
	
	if(Game.mouse.selected>=0 && button==0 && !Game.mouse.button[button]){
		Game.mouse.selected = -1;
		if(Game.move.angle*Game.move.direction>=MIN_MOVE_ANGLE){
			Game.move.flag = 1;
		}else{
			Game.move.angle = 0;
		}
		Game.mouse.lastDrag.x = Game.mouse.x;
		Game.mouse.lastDrag.y = Game.mouse.y;
	}
	
	if(Game.mouse.selected==-1 && button==2 && Game.mouse.button[button]){
		Game.mouse.lastDrag.x = Game.mouse.x;
		Game.mouse.lastDrag.y = Game.mouse.y;
	}
	
	mouseMove(x, y);
}

int main(int argc, char** argv){
	printf("Created by Lucas Preischadt Pinheiro\n\n"
			"Controls:\n"
			"\tW, A, S, D, Q, E, Right Click: move camera\n"
			"\tLeft Click: move puzzle\n"
			"\tR: shuffle puzzle\n"
			"\tSpacebar: reset puzzle\n"
			"\tTab: swap format (sphere/cube)\n");
	
	resetGame();
	Game.format = 0;
	memset(Game.key, 0, sizeof(Game.key));
	srand(time(NULL));
	
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(800, 600);
	glutCreateWindow("Preischadt's Sphere");
	glutDisplayFunc(draw);
    glutReshapeFunc(changeWindowSize);
	glutKeyboardFunc(keyboardDown);
	glutKeyboardUpFunc(keyboardUp);
	glutSpecialFunc(keyboardSpecialDown);
	glutSpecialUpFunc(keyboardSpecialUp);
	glutMouseFunc(mouseClick);
	glutMotionFunc(mouseMove);
	glutPassiveMotionFunc(mouseMove);
	initialize();
	glutTimerFunc(100, game_loop, 1);
	glutMainLoop();
	
	return 1;
}
