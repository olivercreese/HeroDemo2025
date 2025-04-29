#include <iostream>
#include <GL/glew.h>
#include <3dgl/3dgl.h>
#include <GL/glut.h>
#include <GL/freeglut_ext.h>

#include <random>

// Include GLM core features
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#define M_PI 3.14159265

#pragma comment (lib, "glew32.lib")

using namespace std;
using namespace _3dgl;
using namespace glm;

// 3D Models
C3dglTerrain terrain, water;

// models
C3dglModel tree, wolf, bird;

// skybox 
C3dglSkyBox skybox;

//shader programs
C3dglProgram programBasic;
C3dglProgram programWater;
C3dglProgram programTerrain;
C3dglProgram programClouds;

//texture id's terrain
GLuint idTexSand, idTexGrass, idTexSnow;
//cloud vertex attribute id's
GLuint idTexCloudParticle, idBufferCloudVelocity, idBufferCloudStartTime, idBufferInitialPosition, idBufferTextureIndex, idBufferCloudCentres;
//cloud texture id's
GLuint idTexCloud1, idTexCloud2, idTexCloud3, idTexCloud4;
//water refletion id's
GLuint reflectionFBO, idTexReflection;
// wolf texture id
GLuint idTexWolf;
// goose texture id
GLuint idTexbird;

// bitmap for textures
C3dglBitmap bm;

// Water specific variables
float waterLevel = 4.6f;
float snowLevel = 40;
GLuint idFBO;
mat4 matrixReflection;
vec3 p,n;
GLuint W = 1280, H = 720;
bool isFirstPass = false;
vec3 waterColour = vec3(0.31f, 0.34f, 0.35f);

// Wolf variables and Velocity
vec3 wolfPos = vec3(0, 0, 0);	// iniital wolf position
vec3 wolfVel = vec3(0, 0, 0);
float signed_angle;
float animTime;
enum wolfState{IDLE,MOVING};
wolfState currentState = IDLE;
float counter = 0;
vec3 targetPos = vec3(0,0,0);

//goose variables
vec3 birdPos = vec3(0, 0, 0);	// iniital wolf position
vec3 birdVel = vec3(0, 0, 0);
float birdSigned_angle;
vec3 birdTargetPos = vec3(0, 0, 0);


//clouds params with saved values
const int NCLOUDS = 10000; //8000 
const int NCLUSTERSIZE = 140; //120
const vec3 windDirection = vec3(0.8, 0, 0);
const float cloudLevel = 200; //300 
const float cloudAreaSize = 600; //800 
const float cloudClusterRadius = 70; //60 
const float cloudParticleSize = 550; // 550 

//tree params
const int numberOfTrees = 300;
vec3 treePositions[numberOfTrees];



// The View Matrix
mat4 matrixView;

// Camera & navigation
float maxspeed = 4.f;	// camera max speed
float accel = 4.f;		// camera acceleration
vec3 _acc(0), _vel(0);	// camera acceleration and velocity vectors
float _fov = 60.f;		// field of view (zoom)


float randomNumberGen(float upper, float lower) {
	// Create a random device and a generator
	std::random_device rd;  // Random device
	std::mt19937 generator(rd()); // Mersenne Twister engine for random numbers

	// Define the distribution within the range
	std::uniform_int_distribution<int> distribution(lower, upper);
	
	// Generate a random number
	float random_number = distribution(generator);

	return random_number;
}

void populateTreePositions() {
	for (int i = 0; i <= numberOfTrees - 1; i++) {
		float randX = randomNumberGen(100,-100);
		float randZ = randomNumberGen(100, -100);
		treePositions[i] = vec3(randX, terrain.getInterpolatedHeight(randX, randZ), randZ);
		cout << randX << " " << treePositions[i].y << " " << randZ << endl;
		if (treePositions[i].y <= waterLevel) {
			cout << "Collision with water recalculating" << endl;
		}
		while (treePositions[i].y <= waterLevel) {
			float randX = randomNumberGen(100, -100);
			float randZ = randomNumberGen(100, -100);
			treePositions[i] = vec3(randX, terrain.getInterpolatedHeight(randX, randZ), randZ);
		}


	}
}

vec3 generateRandomPointInRadius(const glm::vec3& center, float radius) {
	vec3 point;

	// Random number generators
	random_device rd;
	mt19937 generator(rd());
	uniform_real_distribution<float> angleDist(0.0f, 2.0f * M_PI); // Random angle [0, 2?]
	uniform_real_distribution<float> radiusDist(0.0f, 1.0f);       // Random radius [0, 1]

	float angle = angleDist(generator); // Random angle
	float r = radius * sqrt(radiusDist(generator)); // Random radius (sqrt ensures uniform distribution in the area)

	// Convert polar coordinates to Cartesian coordinates
	float x = center.x + r * cos(angle);
	float z = center.z + r * sin(angle);

	point = vec3(x, cloudLevel, z);


	return point;
}

std::vector<float> cloudBufferPositions;
std::vector<float> cloudCentrePositions;
std::vector<float> cloudBufferVelocity;

void prepareCloudClusterVAB() {
	cloudBufferPositions.clear();
	cloudBufferVelocity.clear();
	std::vector<int> bufferTextureIndices;
	float time = 0;

	random_device rd;
	mt19937 generator(rd());
	uniform_int_distribution<int> textureDist(0,3); // Random number 0 - 3 for texture index

	int clusterCount = 0;

	float randXPos = randomNumberGen(cloudAreaSize, -cloudAreaSize);
	float randZPos = randomNumberGen(cloudAreaSize, -cloudAreaSize);
	

	for (int i = 0; i < NCLOUDS; i++) {

		if (clusterCount >= NCLUSTERSIZE) {
			randXPos = randomNumberGen(cloudAreaSize, -cloudAreaSize);
			randZPos = randomNumberGen(cloudAreaSize, -cloudAreaSize);
			
			clusterCount = 0;
		}
		vec3 randomPoint = generateRandomPointInRadius(vec3(randXPos, cloudLevel, randZPos), cloudClusterRadius);


		// Add particle position
		cloudBufferPositions.push_back(randomPoint.x);
		cloudBufferPositions.push_back(cloudLevel);
		cloudBufferPositions.push_back(randomPoint.z);

		// Add the same velocity for all particles in the cluster
		cloudBufferVelocity.push_back(windDirection.x);
		cloudBufferVelocity.push_back(0);
		cloudBufferVelocity.push_back(windDirection.z);

		cloudCentrePositions.push_back(randXPos);
		cloudCentrePositions.push_back(cloudLevel);
		cloudCentrePositions.push_back(randZPos);

		// Assign a random texture index
		bufferTextureIndices.push_back(textureDist(generator));

		clusterCount++;
	}

	// Create and bind position buffer
	glGenBuffers(1, &idBufferInitialPosition);
	glBindBuffer(GL_ARRAY_BUFFER, idBufferInitialPosition);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * cloudBufferPositions.size(), &cloudBufferPositions[0], GL_STATIC_DRAW);

	// Create and bind velocity buffer
	glGenBuffers(1, &idBufferCloudVelocity);
	glBindBuffer(GL_ARRAY_BUFFER, idBufferCloudVelocity);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * cloudBufferVelocity.size(), &cloudBufferVelocity[0], GL_STATIC_DRAW);

	// Create and bind centre position buffer
	glGenBuffers(1, &idBufferCloudCentres);
	glBindBuffer(GL_ARRAY_BUFFER, idBufferCloudCentres);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * cloudCentrePositions.size(), &cloudCentrePositions[0], GL_STATIC_DRAW);

	// Create and bind texture index buffer
	glGenBuffers(1, &idBufferTextureIndex);
	glBindBuffer(GL_ARRAY_BUFFER, idBufferTextureIndex);
	glBufferData(GL_ARRAY_BUFFER, sizeof(int) * bufferTextureIndices.size(), &bufferTextureIndices[0], GL_STATIC_DRAW);
}


bool init()
{
	// rendering states
	glEnable(GL_DEPTH_TEST);	// depth test is necessary for most 3D scenes
	glEnable(GL_NORMALIZE);		// normalization is needed by AssImp library models
	glShadeModel(GL_SMOOTH);	// smooth shading mode is the default one; try GL_FLAT here!
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);	// this is the default one; try GL_LINE!

	// Initialise Shaders
	C3dglShader vertexShader;
	C3dglShader fragmentShader;
	if (!vertexShader.create(GL_VERTEX_SHADER)) return false;
	if (!vertexShader.loadFromFile("shaders/basic.vert")) return false;
	if (!vertexShader.compile()) return false;

	if (!fragmentShader.create(GL_FRAGMENT_SHADER)) return false;
	if (!fragmentShader.loadFromFile("shaders/basic.frag")) return false;
	if (!fragmentShader.compile()) return false;

	if (!programBasic.create()) return false;
	if (!programBasic.attach(vertexShader)) return false;
	if (!programBasic.attach(fragmentShader)) return false;
	if (!programBasic.link()) return false;
	if (!programBasic.use(true)) return false;

	if (!vertexShader.create(GL_VERTEX_SHADER)) return false;
	if (!vertexShader.loadFromFile("shaders/water.vert")) return false;
	if (!vertexShader.compile()) return false;

	if (!fragmentShader.create(GL_FRAGMENT_SHADER)) return false;
	if (!fragmentShader.loadFromFile("shaders/water.frag")) return false;
	if (!fragmentShader.compile()) return false;

	if (!programWater.create()) return false;
	if (!programWater.attach(vertexShader)) return false;
	if (!programWater.attach(fragmentShader)) return false;
	if (!programWater.link()) return false;
	if (!programWater.use(true)) return false;


	if (!vertexShader.create(GL_VERTEX_SHADER)) return false;
	if (!vertexShader.loadFromFile("shaders/terrain.vert")) return false;
	if (!vertexShader.compile()) return false;

	if (!fragmentShader.create(GL_FRAGMENT_SHADER)) return false;
	if (!fragmentShader.loadFromFile("shaders/terrain.frag")) return false;
	if (!fragmentShader.compile()) return false;

	if (!programTerrain.create()) return false;
	if (!programTerrain.attach(vertexShader)) return false;
	if (!programTerrain.attach(fragmentShader)) return false;
	if (!programTerrain.link()) return false;
	if (!programTerrain.use(true)) return false;


	if (!vertexShader.create(GL_VERTEX_SHADER)) return false;
	if (!vertexShader.loadFromFile("shaders/clouds.vert")) return false;
	if (!vertexShader.compile()) return false;

	if (!fragmentShader.create(GL_FRAGMENT_SHADER)) return false;
	if (!fragmentShader.loadFromFile("shaders/clouds.frag")) return false;
	if (!fragmentShader.compile()) return false;

	if (!programClouds.create()) return false;
	if (!programClouds.attach(vertexShader)) return false;
	if (!programClouds.attach(fragmentShader)) return false;
	if (!programClouds.link()) return false;
	if (!programClouds.use(true)) return false;

	// glut additional setup
	glutSetVertexAttribCoord3(programBasic.getAttribLocation("aVertex"));
	glutSetVertexAttribNormal(programBasic.getAttribLocation("aNormal"));
	glutSetVertexAttribCoord3(programTerrain.getAttribLocation("aVertex"));
	glutSetVertexAttribNormal(programTerrain.getAttribLocation("aNormal"));




	// load your 3D models here!
	programTerrain.use();
	if (!terrain.load("models\\heightmap.png", 50)) return false;
	if (!water.load("models\\watermap.png", 50, &programWater)) return false;
	programBasic.use();
	if (!wolf.load("models\\wolf.dae")) return false;
	if (!bird.load("models\\birdy\\bird.gltf")) return false;
	if (!tree.load("models\\tree\\tree.3ds")) return false;
	tree.loadMaterials("models\\tree");
	tree.getMaterial(0)->loadTexture(GL_TEXTURE4, "models\\tree", "pine-trunk-norm.dds");
	tree.getMaterial(1)->loadTexture(GL_TEXTURE4, "models\\tree", "pine-leaf-norm.dds");
	tree.getMaterial(2)->loadTexture(GL_TEXTURE4, "models\\tree", "pine-branch-norm.dds");
	populateTreePositions();

	//////loading skybox cube map
	if (!skybox.load("models\\mountain\\mft.tga",
		"models\\mountain\\mlf.tga",
		"models\\mountain\\mbk.tga",
		"models\\mountain\\mrt.tga",
		"models\\mountain\\mup.tga",
		"models\\mountain\\mdn.tga")) return false;

	bird.loadAnimations();
	glActiveTexture(GL_TEXTURE0);
	bm.load("models\\birdy\\birdtex.jpg", GL_RGBA);
	if (!bm.getBits()) return false;
	glGenTextures(1, &idTexbird);
	glBindTexture(GL_TEXTURE_2D, idTexbird);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bm.getWidth(), bm.getHeight(), 0, GL_RGBA,
		GL_UNSIGNED_BYTE, bm.getBits());
	programBasic.sendUniform("texture0", 0);
	wolf.loadAnimations();
	///////////////////////////// load the wolf textures
	glActiveTexture(GL_TEXTURE0);
	bm.load("models/wolf.jpg", GL_RGBA);
	if (!bm.getBits()) return false;
	glGenTextures(1, &idTexWolf);
	glBindTexture(GL_TEXTURE_2D, idTexWolf);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bm.getWidth(), bm.getHeight(), 0, GL_RGBA,
		GL_UNSIGNED_BYTE, bm.getBits());
	programBasic.sendUniform("texture0", 0);

	wolfPos = vec3(randomNumberGen(100, -100), 0, randomNumberGen(100, -100));
	targetPos = wolfPos;




	// setup lights (for basic and terrain programs only, water does not use these lights):
	programTerrain.sendUniform("lightAmbient.color", vec3(0.2, 0.2, 0.2));
	programTerrain.sendUniform("lightDir.direction", vec3(1.0, 0.5, 1.0));
	programTerrain.sendUniform("lightDir.diffuse", vec3(1.0, 1.0, 1.0));
	programWater.sendUniform("lightAmbient.color", vec3(1.0, 1.0, 1.0));

	// setup materials (for basic and terrain programs only, water does not use these materials):
	programTerrain.sendUniform("materialAmbient", vec3(1.0, 1.0, 1.0));
	programWater.sendUniform("materialAmbient", vec3(1.0, 1.0, 1.0));
	programTerrain.sendUniform("materialDiffuse", vec3(1.0, 1.0, 1.0));

	// setup the water colours and level
	programWater.sendUniform("waterColor", vec4(waterColour, 0.2f));
	programTerrain.sendUniform("waterColor", vec3(waterColour));
	programTerrain.sendUniform("waterLevel", waterLevel);
	programTerrain.sendUniform("snowLevel", snowLevel);
	programTerrain.sendUniform("fogDensity", 0.3f);


	///////////////water reflection 
	// refelction calculation
	p = vec3(0.f, waterLevel, 0.f);
	n = vec3(0, 1, 0);
	// reflection matrix
	float a = n.x, b = n.y, c = n.z, d = -dot(p, n);
	// parameters of the reflection plane: Ax + By + Cz + d = 0
	matrixReflection = mat4(1 - 2 * a * a, -2 * a * b, -2 * a * c, 0,
		-2 * a * b, 1 - 2 * b * b, -2 * b * c, 0,
		-2 * a * c, -2 * b * c, 1 - 2 * c * c, 0,
		-2 * a * d, -2 * b * d, -2 * c * d, 1);

	//sends reflection matrix varibles to vertex shader 
	programWater.sendUniform("planeClip", vec4(a, b, c, d));
	programBasic.sendUniform("planeClip", vec4(a, b, c, d));
	programTerrain.sendUniform("planeClip", vec4(a, b, c, d));
	programClouds.sendUniform("planeClip", vec4(a, b, c, d));

	//creating and binding reflection texture
	glActiveTexture(GL_TEXTURE11);
	glGenTextures(1, &idTexReflection);
	glBindTexture(GL_TEXTURE_2D, idTexReflection);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	// Texture parameters - to get nice filtering
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
	//glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	//glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1280, 720, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	// Create a framebuffer object (FBO)
	glGenFramebuffers(1, &idFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, idFBO);

	// Attach a depth buffer
	GLuint depth_rb;
	glGenRenderbuffers(1, &depth_rb);
	glBindRenderbuffer(GL_RENDERBUFFER, depth_rb);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, W, H);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rb);

	// attach the texture to FBO colour attachment point
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, idTexReflection, 0);

	// switch back to window-system-provided framebuffer
	glBindFramebufferEXT(GL_FRAMEBUFFER, 0);

	programWater.sendUniform("reflectionTexture", 11);


	/////////////////////////////////setup clouds
	programClouds.use();
	glEnable(GL_PROGRAM_POINT_SIZE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	programClouds.sendUniform("cameraPosition", vec3(-2.0, 1.0, 3.0));
	programClouds.sendUniform("maxDistance", 1000);
	programClouds.sendUniform("clusterRadius", cloudClusterRadius);
	programClouds.sendUniform("baseParticleSize", cloudParticleSize);
	programClouds.sendUniform("sizeFalloffFactor", 0.01f);
	prepareCloudClusterVAB();

	glActiveTexture(GL_TEXTURE7);
	bm.load("models\\Clouds\\Cloud1.png", GL_RGBA);
	if (!bm.getBits()) return false;
	glGenTextures(1, &idTexCloud1);
	glBindTexture(GL_TEXTURE_2D, idTexCloud1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bm.getWidth(), bm.getHeight(), 0, GL_RGBA,
		GL_UNSIGNED_BYTE, bm.getBits());
	programClouds.sendUniform("textures[0]", 7);

	glActiveTexture(GL_TEXTURE8);
	bm.load("models\\Clouds\\Cloud2.png", GL_RGBA);
	if (!bm.getBits()) return false;
	glGenTextures(1, &idTexCloud2);
	glBindTexture(GL_TEXTURE_2D, idTexCloud2);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bm.getWidth(), bm.getHeight(), 0, GL_RGBA,
		GL_UNSIGNED_BYTE, bm.getBits());
	programClouds.sendUniform("textures[1]", 8);

	glActiveTexture(GL_TEXTURE9);
	bm.load("models\\Clouds\\Cloud3.png", GL_RGBA);
	if (!bm.getBits()) return false;
	glGenTextures(1, &idTexCloud3);
	glBindTexture(GL_TEXTURE_2D, idTexCloud3);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bm.getWidth(), bm.getHeight(), 0, GL_RGBA,
		GL_UNSIGNED_BYTE, bm.getBits());
	programClouds.sendUniform("textures[2]", 9);

	glActiveTexture(GL_TEXTURE10);
	bm.load("models\\Clouds\\Cloud4.png", GL_RGBA);
	if (!bm.getBits()) return false;
	glGenTextures(1, &idTexCloud4);
	glBindTexture(GL_TEXTURE_2D, idTexCloud4);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bm.getWidth(), bm.getHeight(), 0, GL_RGBA,
		GL_UNSIGNED_BYTE, bm.getBits());
	programClouds.sendUniform("textures[3]", 10);

	///////////////////////////////////////terrain textures 
	// setup the textures
	glActiveTexture(GL_TEXTURE6);
	bm.load("models/snow.png", GL_RGBA);
	if (!bm.getBits()) return false;
	glGenTextures(1, &idTexSnow);
	glBindTexture(GL_TEXTURE_2D, idTexSnow);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bm.getWidth(), bm.getHeight(), 0, GL_RGBA,
		GL_UNSIGNED_BYTE, bm.getBits());
	programTerrain.sendUniform("textureSnow", 6);

	glActiveTexture(GL_TEXTURE2);
	bm.load("models/grass.jpg", GL_RGBA);
	if (!bm.getBits()) return false;
	glGenTextures(1, &idTexGrass);
	glBindTexture(GL_TEXTURE_2D, idTexGrass);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bm.getWidth(), bm.getHeight(), 0, GL_RGBA,
		GL_UNSIGNED_BYTE, bm.getBits());
	programTerrain.sendUniform("textureShore", 2);

	glActiveTexture(GL_TEXTURE1);
	bm.load("models/sand.png", GL_RGBA);
	if (!bm.getBits()) return false;
	glGenTextures(1, &idTexSand);
	glBindTexture(GL_TEXTURE_2D, idTexSand);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bm.getWidth(),   bm.getHeight(), 0, GL_RGBA,
		GL_UNSIGNED_BYTE, bm.getBits());
	programTerrain.sendUniform("textureBed", 1);
	

	matrixView = lookAt(
		vec3(-82.0, 15, 0.5),
		vec3(-74.0, 15, 0.9),
		vec3(0.0, 1.0, 0.0));


	// setup the screen background colour
	glClearColor(0.2f, 0.6f, 1.f, 1.0f);   // blue sky colour

	cout << endl;
	cout << "Use:" << endl;
	cout << "  WASD or arrow key to navigate" << endl;
	cout << "  QE or PgUp/Dn to move the camera up and down" << endl;
	cout << "  Drag the mouse to look around" << endl;
	cout << endl;

	return true;
}

void wolfMove(vec3 targetPos) {
	vec3 direction = normalize(targetPos - wolfPos);

	float speed = 0.01f;
	wolfVel = direction * speed;

	signed_angle = atan2(direction.x, direction.z) - atan2(vec3(0, 0, 1).x, vec3(0, 0, 1).z);

}

bool waitFor(float amount, float deltaTime) {
	counter += deltaTime;
	if (counter >= amount) return true;
	else return false;
}

void genTargetPos() {
	vec3 pos = generateRandomPointInRadius(wolfPos, 25);
	float amendY = terrain.getInterpolatedHeight(pos.x, pos.z);

	while (amendY <= waterLevel) {
		pos = generateRandomPointInRadius(wolfPos, 25);
		amendY = terrain.getInterpolatedHeight(pos.x, pos.z);
	}

	targetPos = vec3(pos.x, terrain.getInterpolatedHeight(pos.x,pos.z), pos.z);

}

vec3 findWalkablePointInRadius(vec3 position, float radius) {
	vec3 point;

	// Random number generators
	random_device rd;
	mt19937 generator(rd());
	uniform_real_distribution<float> angleDist(0.0f, 2.0f * M_PI); // Random angle [0, 2?]
	uniform_real_distribution<float> radiusDist(0.0f, 1.0f);       // Random radius [0, 1]

	float angle = angleDist(generator); // Random angle
	float r = radius * sqrt(radiusDist(generator)); // Random radius (sqrt ensures uniform distribution in the area)

	// Convert polar coordinates to Cartesian coordinates
	float x = position.x + r * cos(angle);
	float z = position.z + r * sin(angle);

	point = vec3(x, terrain.getInterpolatedHeight(x,z), z);

	while (point.y <= waterLevel) {
		float angle = angleDist(generator); 
		float r = radius * sqrt(radiusDist(generator)); 
		float x = position.x + r * cos(angle);
		float z = position.z + r * sin(angle);
		point = vec3(x, terrain.getInterpolatedHeight(x, z), z);
	}


	return vec3(point.x,0,point.z);
}

vec3 findSwimmablePointInRadius(vec3 position, float radius) {
	vec3 point;

	// Random number generators
	random_device rd;
	mt19937 generator(rd());
	uniform_real_distribution<float> angleDist(0.0f, 2.0f * M_PI); // Random angle [0, 2?]
	uniform_real_distribution<float> radiusDist(0.0f, 1.0f);       // Random radius [0, 1]

	float angle = angleDist(generator); // Random angle
	float r = radius * sqrt(radiusDist(generator)); // Random radius (sqrt ensures uniform distribution in the area)

	// Convert polar coordinates to Cartesian coordinates
	float x = position.x + r * cos(angle);
	float z = position.z + r * sin(angle);

	point = vec3(x, terrain.getInterpolatedHeight(x, z), z);

	while (point.y > waterLevel) {
		float angle = angleDist(generator);
		float r = radius * sqrt(radiusDist(generator));
		float x = position.x + r * cos(angle);
		float z = position.z + r * sin(angle);
		point = vec3(x, terrain.getInterpolatedHeight(x, z), z);
	}


	return vec3(point.x, waterLevel, point.z);
}
void birdMove(vec3 targetPos) {
	vec3 direction = normalize(targetPos - birdPos);

	float speed = 0.04f;
	birdVel = direction * speed;

	birdSigned_angle = atan2(direction.x, direction.z) - atan2(vec3(0, 0, 1).x, vec3(0, 0, 1).z);

}


void renderBird(mat4& matrixView, float time, float deltaTime) {
	mat4 m = matrixView;
	programBasic.use();
	programBasic.sendUniform("lightAmbient.color", vec3(1.0, 1.0, 1.0));
	programBasic.sendUniform("materialAmbient", vec3(0.1f, 0.1f, 0.1f)); // white background for textures
	programBasic.sendUniform("materialDiffuse", vec3(1.0f, 1.0f, 1.0f));
	programBasic.sendUniform("lightDir.direction", vec3(1.0, 0.5, 1.0));
	programBasic.sendUniform("lightDir.diffuse", vec3(1.0, 1.0, 1.0));

	birdMove(birdTargetPos);

	if (distance(targetPos, wolfPos) > 1) {
		wolfPos = wolfPos + wolfVel;
		animTime = time;
	}
	else
	{
		birdTargetPos = vec3(randomNumberGen(100, -100), randomNumberGen(50, -100), randomNumberGen(100, -100));
	}



	std::vector<mat4> transforms;
	bird.getAnimData(0, time, transforms);
	programBasic.sendUniform("bones", &transforms[0], transforms.size());
	m = matrixView;
	m = translate(m, birdPos);
	m = rotate(m, radians(1.0f) + signed_angle, vec3(0.f, 1.f, 0.f));
	m = scale(m, vec3(1.2f, 1.2f, 1.2f));
	bird.render(m);

}


void renderWolf(mat4& matrixView, float time, float deltaTime) {
	mat4 m = matrixView;
	programBasic.use();
	programBasic.sendUniform("lightAmbient.color", vec3(0.1, 0.1, 0.1));
	programBasic.sendUniform("materialAmbient", vec3(0.1f, 0.1f, 0.1f)); // white background for textures
	programBasic.sendUniform("materialDiffuse", vec3(1.0f, 1.0f, 1.0f));
	programBasic.sendUniform("lightDir.direction", vec3(1.0, 0.5, 1.0));
	programBasic.sendUniform("lightDir.diffuse", vec3(1.0, 1.0, 1.0));

	switch (currentState) {
	case IDLE:
		if (waitFor(randomNumberGen(60, 30), deltaTime)) {
			targetPos = findWalkablePointInRadius(wolfPos, 25);
			currentState = MOVING;
		}
		break;
	}
	
	// wolf movement
	wolfMove(targetPos);

	if (distance(targetPos, wolfPos) > 1) {
		wolfPos = wolfPos + wolfVel;
		animTime = time;
	}
	else
	{
		currentState = IDLE;
	}

	vec3 amendY = vec3(vec3(0, terrain.getInterpolatedHeight(wolfPos.x, wolfPos.z), 0));

	// render the wolf
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, idTexWolf);
	std::vector<mat4> transforms;
	wolf.getAnimData(0, animTime, transforms);
	programBasic.sendUniform("bones", &transforms[0], transforms.size());
	m = matrixView;
	m = translate(m, wolfPos + amendY);
	m = rotate(m, radians(1.0f) + signed_angle, vec3(0.f, 1.f, 0.f));
	wolf.render(m);


}

void renderTerrain(mat4& matrixView, float time, float deltaTime) {
	// Render Terrain
	mat4 m = matrixView;
	programTerrain.use();
	terrain.render(m);
}


void renderWater(mat4& matrixView, float time, float deltaTime) {
	// Render Water
	mat4 m = matrixView;
	programWater.use();
	glActiveTexture(GL_TEXTURE11);
	glBindTexture(GL_TEXTURE_2D, idTexReflection);
	// Setup the Diffuse Material to: Watery Blue
	programWater.sendUniform("materialAmbient", vec3(waterColour));
	m = matrixView;
	m = translate(m, vec3(0, waterLevel, 0));
	programWater.sendUniform("matrixModelView", m);
	water.render(m);
}


vec3 getCameraWorldPosition(mat4& matrixView) {
	// Calculate the inverse of the view matrix to get camera world position
	mat4 matrixViewInverse = glm::inverse(matrixView);
	vec3 cameraWorldPosition = vec3(matrixViewInverse[3]);
	return cameraWorldPosition;
}


void updateCloudPositions(float deltaTime) {
	for (size_t i = 0; i < cloudBufferPositions.size(); i += 3) {
		// Update position based on velocity
		cloudBufferPositions[i] += cloudBufferVelocity[i] * deltaTime;     // Update x
		//cloudBufferPositions[i + 1] += cloudBufferVelocity[i + 1] * deltaTime; // Update y (if needed)
		cloudBufferPositions[i + 2] += cloudBufferVelocity[i + 2] * deltaTime; // Update z

		// Check if the cloud has crossed the recycling limit
		if (cloudBufferPositions[i] > cloudAreaSize+100) {
			cloudBufferPositions[i] = -(cloudAreaSize+100); // Reset to the opposite side
		}
	}
	for (size_t i = 0; i < cloudCentrePositions.size(); i += 3) {
		cloudCentrePositions[i] += cloudBufferVelocity[i] * deltaTime;     // Update x

		cloudCentrePositions[i + 2] += cloudBufferVelocity[i + 2] * deltaTime; // Update z
		if (cloudCentrePositions[i] > cloudAreaSize+100) {
			cloudCentrePositions[i] = -(cloudAreaSize+100); // Reset to the opposite side
		}
	}

	glBindBuffer(GL_ARRAY_BUFFER, idBufferInitialPosition);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * cloudBufferPositions.size(), &cloudBufferPositions[0]);


	glBindBuffer(GL_ARRAY_BUFFER, idBufferCloudCentres);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * cloudCentrePositions.size(), &cloudCentrePositions[0]);
}

void renderClouds(mat4& matrixView, float time, float deltaTime) {
	mat4 m = matrixView;
	programClouds.use();
	// setup the point size
	glEnable(GL_POINT_SPRITE);
	//glPointSize(cloudParticleSize);
	glDepthMask(GL_FALSE);
	// rendr the buffer
	GLint aVelocity = programClouds.getAttribLocation("aVelocity");
	GLint aPosition = programClouds.getAttribLocation("aPosition");
	GLint aTextureIndex = programClouds.getAttribLocation("aTextureIndex");
	GLint aCentre = programClouds.getAttribLocation("centreCloudPos");


	programClouds.sendUniform("cameraPosition", getCameraWorldPosition(m));

	updateCloudPositions(deltaTime); //reseting clouds after drifting past 800 on x axis

	glEnableVertexAttribArray(aVelocity); // velocity
	glEnableVertexAttribArray(aPosition); // position
	glEnableVertexAttribArray(aTextureIndex); // index of texture to use
	glEnableVertexAttribArray(aCentre);
	glBindBuffer(GL_ARRAY_BUFFER, idBufferTextureIndex);
	glVertexAttribPointer(aTextureIndex, 1, GL_INT, GL_FALSE, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, idBufferCloudCentres);
	glVertexAttribPointer(aCentre, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, idBufferCloudVelocity);
	glVertexAttribPointer(aVelocity, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, idBufferInitialPosition);
	glVertexAttribPointer(aPosition, 3, GL_FLOAT, GL_FALSE, 0, 0);

	if (isFirstPass) {
		glDisable(GL_CLIP_PLANE0);
		glDrawArrays(GL_POINTS, 0, NCLOUDS);
		glEnable(GL_CLIP_PLANE0);
	}
	else
		glDrawArrays(GL_POINTS, 0, NCLOUDS);

	glDisableVertexAttribArray(aVelocity);
	glDisableVertexAttribArray(aTextureIndex);
	glDisableVertexAttribArray(aPosition);
	glDisableVertexAttribArray(aCentre);

	glDepthMask(GL_TRUE);

}

void renderTrees(mat4& matrixView, float time, float deltaTime) {
	programBasic.use();
	glDepthMask(GL_FALSE);
	programBasic.sendUniform("lightAmbient.color", vec3(0.4, 0.4, 0.4));
	programBasic.sendUniform("materialDiffuse", vec3(1.0, 1.0, 1.0));
	programBasic.sendUniform("materialAmbient", vec3(0.1, 0.1, 0.1));
	programBasic.sendUniform("lightDir.direction", vec3(1.0, 0.5, 1.0));
	programBasic.sendUniform("lightDir.diffuse", vec3(1.0, 1.0, 1.0));
	// render the trees
	programBasic.sendUniform("useNormalMap", true);
	for (int i = 0; i <= numberOfTrees -1; i++) {
		if (treePositions[i].y <= waterLevel) continue;
		mat4 m = matrixView;
		m = translate(matrixView, treePositions[i]);
		m = scale(m, vec3(0.01f, 0.01f, 0.01f));
		tree.render(m);
	}
	programBasic.sendUniform("useNormalMap", false);
	glDepthMask(GL_TRUE);

}

void renderSkybox(mat4& matrixView, float time, float deltaTime) {
	mat4 m = matrixView;
	programBasic.use();
	programBasic.sendUniform("lightAmbient.color", vec3(1.0, 1.0, 1.0));
	programBasic.sendUniform("materialDiffuse", vec3(0.0f, 0.0f, 0.0f));	// setting up light for skybox
	programBasic.sendUniform("materialAmbient", vec3(1.0f, 1.0f, 1.0f));
	programBasic.sendUniform("lightDir.direction", vec3(1.0, 0.5, 1.0));
	programBasic.sendUniform("lightDir.diffuse", vec3(1.0, 1.0, 1.0));
	if (isFirstPass) {
		glDisable(GL_CLIP_PLANE0);
		skybox.render(m);
		glEnable(GL_CLIP_PLANE0);
	}
	else 
		skybox.render(m);

}

void renderScene(mat4& matrixView, float time, float deltaTime)
{
	renderSkybox(matrixView, time, deltaTime);
	renderTerrain(matrixView,time,deltaTime);
	renderWater(matrixView, time, deltaTime);
	renderClouds(matrixView, time, deltaTime);
	renderTrees(matrixView, time, deltaTime);
	renderWolf(matrixView, time, deltaTime);
	//renderBird(matrixView, time, deltaTime);
}

void renderReflection(mat4& matrixView, float time, float deltaTime)
{
	glEnable(GL_CLIP_PLANE0);

	// Bind the Framebuffer for off-screen rendering
	glBindFramebuffer(GL_FRAMEBUFFER, idFBO);
	isFirstPass = true;

	// Clear the FBO
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	matrixView *= matrixReflection;
	//programWater.sendUniform("matrixView", matrixView);  water dosent need to reflect itself
	programTerrain.sendUniform("matrixView", matrixView);
	programBasic.sendUniform("matrixView", matrixView); 
	programClouds.sendUniform("matrixView", matrixView);
	renderScene(matrixView, time, deltaTime);

	glDisable(GL_CLIP_PLANE0);
	isFirstPass = false;

}

void onRender()
{
	// these variables control time & animation
	static float prev = 0;
	float time = glutGet(GLUT_ELAPSED_TIME) * 0.001f;	// time since start in seconds
	float deltaTime = time - prev;						// time since last frame
	prev = time;										// framerate is 1/deltaTime
	// send the animation time to shaders
	programWater.sendUniform("t", time);
	programClouds.sendUniform("time", time);

	// clear screen and buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	_vel = clamp(_vel + _acc * deltaTime, -vec3(maxspeed), vec3(maxspeed));
	float pitch = getPitch(matrixView);
	matrixView = rotate(translate(rotate(mat4(1),
		pitch, vec3(1, 0, 0)),	// switch the pitch off
		_vel * deltaTime),		// animate camera motion (controlled by WASD keys)
		-pitch, vec3(1, 0, 0))	// switch the pitch on
		* matrixView;

	float terrainY = -std::max(terrain.getInterpolatedHeight(inverse(matrixView)[3][0], inverse(matrixView)[3][2]), waterLevel);
	//matrixView = translate(matrixView, vec3(0, terrainY, 0));


	renderReflection(matrixView, time, deltaTime); // renders reflection in off screen texture 

	glBindFramebuffer(GL_FRAMEBUFFER, 0); //  second pass rendering / on screen rendering 

	// reverting camera back to normal view
	matrixView *= matrixReflection;
	//programWater.sendUniform("matrixView", matrixView);	water dosent need to reflect itself
	programTerrain.sendUniform("matrixView", matrixView);
	programBasic.sendUniform("matrixView", matrixView);  
	programClouds.sendUniform("matrixView", matrixView);
	// render the scene as normal 
	renderScene(matrixView, time, deltaTime);


	// essential for double-buffering technique
	glutSwapBuffers();

	// proceed the animation
	glutPostRedisplay();
}



// called before window opened or resized - to setup the Projection Matrix
void onReshape(int w, int h)
{
	float ratio = w * 1.0f / h;      // we hope that h is not zero
	glViewport(0, 0, w, h);
	mat4 m = perspective(radians(_fov), ratio, 0.02f, 1000.f);
	programBasic.sendUniform("matrixProjection", m);
	programTerrain.sendUniform("matrixProjection", m);
	programWater.sendUniform("matrixProjection", m);
	programClouds.sendUniform("matrixProjection", m);
}

// Handle WASDQE keys
void onKeyDown(unsigned char key, int x, int y)
{
	switch (tolower(key))
	{
	case 'w': _acc.z = accel; break;
	case 's': _acc.z = -accel; break;
	case 'a': _acc.x = accel; break;
	case 'd': _acc.x = -accel; break;
	case 'e': _acc.y = accel; break;
	case 'q': _acc.y = -accel; break;
	case 'p': 	
		mat4 matrixViewInverse = glm::inverse(matrixView);
		vec3 cameraWorldPosition = vec3(matrixViewInverse[3]);
		cout << "X:" << cameraWorldPosition.x << endl;
		cout << "Y:" << cameraWorldPosition.y << endl;
		cout << "Z:" << cameraWorldPosition.z << endl;
		break;
	}
}

// Handle WASDQE keys (key up)
void onKeyUp(unsigned char key, int x, int y)
{
	switch (tolower(key))
	{
	case 'w':
	case 's': _acc.z = _vel.z = 0; break;
	case 'a':
	case 'd': _acc.x = _vel.x = 0; break;
	case 'q':
	case 'e': _acc.y = _vel.y = 0; break;
	}
}

// Handle arrow keys and Alt+F4
void onSpecDown(int key, int x, int y)
{
	maxspeed = glutGetModifiers() & GLUT_ACTIVE_SHIFT ? 20.f : 4.f;
	switch (key)
	{
	case GLUT_KEY_F4:		if ((glutGetModifiers() & GLUT_ACTIVE_ALT) != 0) exit(0); break;
	case GLUT_KEY_UP:		onKeyDown('w', x, y); break;
	case GLUT_KEY_DOWN:		onKeyDown('s', x, y); break;
	case GLUT_KEY_LEFT:		onKeyDown('a', x, y); break;
	case GLUT_KEY_RIGHT:	onKeyDown('d', x, y); break;
	case GLUT_KEY_PAGE_UP:	onKeyDown('q', x, y); break;
	case GLUT_KEY_PAGE_DOWN:onKeyDown('e', x, y); break;
	case GLUT_KEY_F11:		glutFullScreenToggle();
	}
}

// Handle arrow keys (key up)
void onSpecUp(int key, int x, int y)
{
	maxspeed = glutGetModifiers() & GLUT_ACTIVE_SHIFT ? 20.f : 4.f;
	switch (key)
	{
	case GLUT_KEY_UP:		onKeyUp('w', x, y); break;
	case GLUT_KEY_DOWN:		onKeyUp('s', x, y); break;
	case GLUT_KEY_LEFT:		onKeyUp('a', x, y); break;
	case GLUT_KEY_RIGHT:	onKeyUp('d', x, y); break;
	case GLUT_KEY_PAGE_UP:	onKeyUp('q', x, y); break;
	case GLUT_KEY_PAGE_DOWN:onKeyUp('e', x, y); break;
	}
}

// Handle mouse click
void onMouse(int button, int state, int x, int y)
{
	glutSetCursor(state == GLUT_DOWN ? GLUT_CURSOR_CROSSHAIR : GLUT_CURSOR_INHERIT);
	glutWarpPointer(glutGet(GLUT_WINDOW_WIDTH) / 2, glutGet(GLUT_WINDOW_HEIGHT) / 2);
	if (button == 1)
	{
		_fov = 60.0f;
		onReshape(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
	}
}

// handle mouse move
void onMotion(int x, int y)
{
	glutWarpPointer(glutGet(GLUT_WINDOW_WIDTH) / 2, glutGet(GLUT_WINDOW_HEIGHT) / 2);

	// find delta (change to) pan & pitch
	float deltaYaw = 0.005f * (x - glutGet(GLUT_WINDOW_WIDTH) / 2);
	float deltaPitch = 0.005f * (y - glutGet(GLUT_WINDOW_HEIGHT) / 2);

	if (abs(deltaYaw) > 0.3f || abs(deltaPitch) > 0.3f)
		return;	// avoid warping side-effects

	// View = Pitch * DeltaPitch * DeltaYaw * Pitch^-1 * View;
	constexpr float maxPitch = radians(80.f);
	float pitch = getPitch(matrixView);
	float newPitch = glm::clamp(pitch + deltaPitch, -maxPitch, maxPitch);
	matrixView = rotate(rotate(rotate(mat4(1.f),
		newPitch, vec3(1.f, 0.f, 0.f)),
		deltaYaw, vec3(0.f, 1.f, 0.f)), 
		-pitch, vec3(1.f, 0.f, 0.f)) 
		* matrixView;
}

void onMouseWheel(int button, int dir, int x, int y)
{
	_fov = glm::clamp(_fov - dir * 5.f, 5.0f, 175.f);
	onReshape(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
}

int main(int argc, char **argv)
{
	// init GLUT and create Window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(1280, 720);
	glutCreateWindow("3DGL Scene: Water Rendering (initial)");

	// init glew
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		C3dglLogger::log("GLEW Error {}", (const char*)glewGetErrorString(err));
		return 0;
	}
	C3dglLogger::log("Using GLEW {}", (const char*)glewGetString(GLEW_VERSION));

	// register callbacks
	glutDisplayFunc(onRender);
	glutReshapeFunc(onReshape);
	glutKeyboardFunc(onKeyDown);
	glutSpecialFunc(onSpecDown);
	glutKeyboardUpFunc(onKeyUp);
	glutSpecialUpFunc(onSpecUp);
	glutMouseFunc(onMouse);
	glutMotionFunc(onMotion);
	glutMouseWheelFunc(onMouseWheel);

	C3dglLogger::log("Vendor: {}", (const char *)glGetString(GL_VENDOR));
	C3dglLogger::log("Renderer: {}", (const char *)glGetString(GL_RENDERER));
	C3dglLogger::log("Version: {}", (const char*)glGetString(GL_VERSION));
	C3dglLogger::log("");

	// init light and everything – not a GLUT or callback function!
	if (!init())
	{
		C3dglLogger::log("Application failed to initialise\r\n");
		return 0;
	}

	// enter GLUT event processing cycle
	glutMainLoop();

	return 1;
}

