//============================================================================
// Name        : hifem.cpp
// Author      : Pourya Shirazian
// Version     :
// Copyright   : CopyRight 2014
// Description : High Performance Implicit Surface Modeling and Animation
//============================================================================

#include <cstdio>
#include <iostream>
#include "base/Logger.h"
#include "base/Profiler.h"
#include "base/FileDirectory.h"
#include "base/SettingsDB.h"
#include "base/String.h"
#include "graphics/AppScreen.h"
#include "graphics/selectgl.h"
#include "graphics/SceneGraph.h"
#include "graphics/SGQuad.h"
#include "graphics/Gizmo.h"
#include "implicit/SketchMachine.h"
#include "implicit/FieldComputer.h"
//#include "glui/glui.h"

using namespace std;
using namespace PS;
using namespace PS::SG;
using namespace PS::SKETCH;
using namespace PS::FILESTRINGUTILS;

//test
PS::SKETCH::FieldComputer* g_fc = NULL;
void closeApp();

//DRAW
void draw() {
	TheSceneGraph::Instance().draw();
    TheSketchMachine::Instance().draw();

    if(g_fc)
    	g_fc->draw();

	glutSwapBuffers();
}

void mousePress(int button, int state, int x, int y) {
    //TheSketchMachine::Instance().mousePress(button, state, x, y);
	glutPostRedisplay();
}

void mouseMove(int x, int y) {
   // TheSketchMachine::Instance().mouseMove(x, y);
	glutPostRedisplay();
}

void mousePassiveMotion(int x, int y) {

}

void mouseWheel(int button, int dir, int x, int y) {
	TheSceneGraph::Instance().mouseWheel(button, dir, x, y);
	glutPostRedisplay();
}

void normalKey(unsigned char key, int x, int y) {
	switch(key){

	//Add Prim
	case('a'): {
		TheSketchMachine::Instance().blob()->actReset();

		SketchAddPrim* actAddPrim = new SketchAddPrim();
		actAddPrim->primType = primPoint;
		actAddPrim->pos = vec3f(0, 1, 0);
		actAddPrim->dir = vec3f(0, 1, 0);
		actAddPrim->res = vec3f(0, 0, 0);
		actAddPrim->color = vec4f(1, 0, 0, 1);

		TheSketchMachine::Instance().addAction(actAddPrim);
	}
	break;
            
        case('g'): {
            TheGizmoManager::Instance().setType(gtTranslate);
        }
        break;

        case('s'): {
            TheGizmoManager::Instance().setType(gtScale);
        }
        break;

        case('r'): {
            TheGizmoManager::Instance().setType(gtRotate);
        }
            break;

	case('t'): {
		LogInfo("Start testing polygonizer");

		TheProfiler::Instance().startSession();
		for(int i=0; i<100; i++) {
			ProfileStart();
			TheSketchMachine::Instance().sync();
			ProfileEnd();
		}

		TheProfiler::Instance().endSession();
		LogInfoArg1("End testing session took: %.3f", TheProfiler::Instance().session().duration());
	}
	break;

	case('f'): {
		TheSketchMachine::Instance().forward();
	}
	break;

	case('b'): {
		TheSketchMachine::Instance().backward();
	}
	break;

	}

}

void specialKey(int key, int x, int y) {
	switch(key) {
	case(GLUT_KEY_F11): {
		closeApp();
		break;
	}

	}
}

void initGL() {
	//Setup Shading Environment
	static const GLfloat lightColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	static const GLfloat lightPos[4] = { 0.0f, 9.0f, 0.0f, 1.0f };

	//Setup Light0 Position and Color
	glLightfv(GL_LIGHT0, GL_AMBIENT, lightColor);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColor);
	glLightfv(GL_LIGHT0, GL_SPECULAR, lightColor);
	glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

	//Turn on Light 0
	glEnable(GL_LIGHT0);
	//Enable Lighting
	glEnable(GL_LIGHTING);
	glEnable(GL_LINE_SMOOTH);

	//Enable features we want to use from OpenGL
    glEnable(GL_TEXTURE_2D);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_POLYGON_SMOOTH);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_STENCIL_TEST);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	glClearColor(0.45f, 0.45f, 0.45f, 1.0f);
	//glClearColor(1.0, 1.0, 1.0, 1.0);

	//Compiling shaders
	GLenum err = glewInit();
	if (err != GLEW_OK) {
		//Problem: glewInit failed, something is seriously wrong.
		fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
		exit(1);
	}
    
    //Version info
    LogInfoArg1("Supported OpenGL is %s", glGetString(GL_VERSION));
    LogInfoArg1("Supported GLSL is %s", glGetString(GL_SHADING_LANGUAGE_VERSION));

    AnsiStr rootPath = Settings::Instance().get("rootpath").get<AnsiStr>();
	//Load Shaders
	AnsiStr strShaderPath = rootPath + AnsiStr("data/shaders");
	TheShaderManager::Instance().addFromFolder(strShaderPath.cptr());
    
    //Load Textures
    AnsiStr strTexPath = rootPath + AnsiStr("data/textures/marble.png");
    TheTexManager::Instance().add(strTexPath);

	//Loading Geometries
	vec3f lo(-16, -16, -16);
	vec3f hi(16, 16, 16);
	TheSceneGraph::Instance().addSceneBox(AABB(lo, hi));
	TheSceneGraph::Instance().addFloor(16, 16, 4);
//	TheSceneGraph::Instance().add(&);
    
    //Add Textured Quad
    //SGQuad* quad = new SGQuad(3.0, 3.0, TheTexManager::Instance().get("marble"));
    //TheSceneGraph::Instance().add(quad);

	//Load initial model
	AnsiStr strModel = rootPath + Settings::Instance().get("model").get<AnsiStr>();
	TheSketchMachine::Instance().blob()->load(strModel);
	TheSketchMachine::Instance().sync();

	//Perform Tests For Field Computations
	g_fc = new PS::SKETCH::FieldComputer(*TheSketchMachine::Instance().blob());

	//Test
	//LogInfo("Test StackLess Voxel Field Compute");
	//g_fc->fieldsForVoxelGrid(0.1f, true);

	bool stackless = Settings::Instance().get("stackless").get<bool>();
	AnsiStr strArg =  (stackless ? "stackless" : "stack-based");
	LogInfoArg1("Test %s Voxel Field Compute", strArg.cptr());

	g_fc->fieldsForVoxelGrid(0.14f, stackless);

	LogInfoArg1("Number of vertices processed: %d", g_fc->countVertices());
}

/*!
 * Cleanup functions upon closing the app
 */
void closeApp() {

	LogInfo("Closing App and Clearing Cache");
	glutLeaveMainLoop();
	//TheSketchMachine::Instance().clearQ();
	SAFE_DELETE(g_fc);

	exit(1);
}

/*!
 * main function
 */
int main(int argc, char* argv[]) {
	LogInfo("Start HIFEM");
	LogInfoArg1("Current Path is: %s", ExtractFilePath(GetExePath()).cptr());
    AnsiStr rootPath = ExtractOneLevelUp(ExtractFilePath(GetExePath()));
    Settings::Instance().add(Value(DEFAULT_CELL_SIZE), "cellsize");
    Settings::Instance().add(Value(rootPath), "rootpath");
    Settings::Instance().add(Value(AnsiStr("data/models/blobtree/tumor.blob")), "model");
    Settings::Instance().add(true, "stackless");
    Settings::Instance().writeScript(rootPath + "settings.ini");

	//Processing args
    if(Settings::Instance().processCmdLine(argc, argv) < 0)
    	return 1;

    Settings::Instance().printAll();

    //Init
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_STENCIL);
	glutInitWindowSize(DEFAULT_WIDTH, DEFAULT_HEIGHT);
	glutCreateWindow("HIFEM: High-Performance Implicit Modeling and Physical Animation");
	glutDisplayFunc(draw);
	glutReshapeFunc(def_resize);
	glutMouseFunc(mousePress);
	glutMotionFunc(mouseMove);
	glutPassiveMotionFunc(mousePassiveMotion);
	glutMouseWheelFunc(mouseWheel);
	glutKeyboardFunc(normalKey);
	glutSpecialFunc(specialKey);
	glutCloseFunc(closeApp);

	initGL();

	glutPostRedisplay();

	glutMainLoop();

	return 0;
}
