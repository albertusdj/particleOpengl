#include <stdio.h>
#include <stdlib.h>

#include <vector>
#include <algorithm>

#include <GL/glew.h>

#include <GLFW/glfw3.h>
GLFWwindow* window;

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/norm.hpp>
using namespace glm;

#include "common/shader.hpp"
#include "common/texture.hpp"
#include "common/controls.hpp"

// CPU representation of a particle
struct Particle{
	glm::vec3 pos, speed;
	unsigned char r,g,b,a; // Color
	float size, angle, weight;
	float life; // Remaining life of the particle. if <0 : dead and unused.
	float cameradistance; // *Squared* distance to the camera. if dead : -1.0f

	bool operator<(const Particle& that) const {
		// Sort in reverse order : far particles drawn first.
		return this->cameradistance > that.cameradistance;
	}
};

const int MaxParticles = 100;
Particle ParticlesContainer[MaxParticles];
Particle ParticlesContainer2[MaxParticles];
int LastUsedParticle = 0;
int LastUsedParticle2 = 0;

// Finds a Particle in ParticlesContainer which isn't used yet.
// (i.e. life < 0);
int FindUnusedParticle(){

	for(int i=LastUsedParticle; i<MaxParticles; i++){
		if (ParticlesContainer[i].life < 0){
			LastUsedParticle = i;
			return i;
		}
	}

	for(int i=0; i<LastUsedParticle; i++){
		if (ParticlesContainer[i].life < 0){
			LastUsedParticle = i;
			return i;
		}
	}

	return 0; // All particles are taken, override the first one
}

int FindUnusedParticle2(){

	for(int i=LastUsedParticle2; i<MaxParticles; i++){
		if (ParticlesContainer2[i].life < 0){
			LastUsedParticle2 = i;
			return i;
		}
	}

	for(int i=0; i<LastUsedParticle2; i++){
		if (ParticlesContainer2[i].life < 0){
			LastUsedParticle2 = i;
			return i;
		}
	}

	return 0; // All particles are taken, override the first one
}

void SortParticles(){
	std::sort(&ParticlesContainer[0], &ParticlesContainer[MaxParticles]);
}

void SortParticles2(){
	std::sort(&ParticlesContainer2[0], &ParticlesContainer2[MaxParticles]);
}

int main( void )
{
	// Initialise GLFW
	if( !glfwInit() )
	{
		fprintf( stderr, "Failed to initialize GLFW\n" );
		getchar();
		return -1;
	}


	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_RESIZABLE,GL_FALSE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow( 1024, 768, "Test", NULL, NULL);
	if( window == NULL ){
		fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
		getchar();
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	glewExperimental = true;
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		getchar();
		glfwTerminate();
		return -1;
	}

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

	// Dark blue background
	glClearColor(0.2f, 0.2f, 0.2f, 0.0f);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);
	

	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);




	// Create and compile our GLSL program from the shaders
	GLuint rainProgramID = LoadShaders( "Particle.vertexshader", "Particle.fragmentshader" );

	// Vertex shader
	GLuint CameraRight_worldspace_ID  = glGetUniformLocation(rainProgramID, "CameraRight_worldspace");
	GLuint CameraUp_worldspace_ID  = glGetUniformLocation(rainProgramID, "CameraUp_worldspace");
	GLuint ViewProjMatrixID = glGetUniformLocation(rainProgramID, "VP");

	// fragment shader
	GLuint rainTextureID  = glGetUniformLocation(rainProgramID, "myTextureSampler");

	
	static GLfloat* g_particule_position_size_data = new GLfloat[MaxParticles * 4];
	static GLubyte* g_particule_color_data         = new GLubyte[MaxParticles * 4];

	for(int i=0; i<MaxParticles; i++){
		ParticlesContainer[i].life = -1.0f;
		ParticlesContainer[i].cameradistance = -1.0f;
	}

	GLuint rainTexture = loadDDS("particle.DDS");



	GLuint smokeProgramID = LoadShaders( "Particle.vertexshader", "Particle.fragmentshader" );

	// Vertex shader
	GLuint CameraRight_worldspace_ID2  = glGetUniformLocation(smokeProgramID, "CameraRight_worldspace");
	GLuint CameraUp_worldspace_ID2  = glGetUniformLocation(smokeProgramID, "CameraUp_worldspace");
	GLuint ViewProjMatrixID2 = glGetUniformLocation(smokeProgramID, "VP");

	// fragment shader
	GLuint smokeTextureID  = glGetUniformLocation(smokeProgramID, "myTextureSampler");

	
	static GLfloat* g_particule_position_size_data2 = new GLfloat[MaxParticles * 4];
	static GLubyte* g_particule_color_data2         = new GLubyte[MaxParticles * 4];

	for(int i=0; i<MaxParticles; i++){
		ParticlesContainer2[i].life = -1.0f;
		ParticlesContainer2[i].cameradistance = -1.0f;
	}

	GLuint smokeTexture = loadDDS("particle.DDS");



	// Create and compile our GLSL program from the shaders
	GLuint programID = LoadShaders( "StandardShading.vertexshader", "StandardShading.fragmentshader" );

	// Get a handle for our "MVP" uniform
	GLuint MatrixID = glGetUniformLocation(programID, "MVP");
	GLuint ViewMatrixID = glGetUniformLocation(programID, "V");
	GLuint ModelMatrixID = glGetUniformLocation(programID, "M");

	GLuint texture = loadBMP_custom("woodtexture.bmp");
	GLuint TextureID = glGetUniformLocation(programID, "carTexture");

	
	GLuint frontwheelprogramID = LoadShaders( "FrontWheelVertexShader.vertexshader", "FragmentShader.fragmentshader" );
	GLuint FrontWheelMatrixID = glGetUniformLocation(frontwheelprogramID, "MVPFrontWheel");

	GLuint backwheelprogramID = LoadShaders( "BackWheelVertexShader.vertexshader", "FragmentShader.fragmentshader" );
	GLuint BackWheelMatrixID = glGetUniformLocation(backwheelprogramID, "MVPBackWheel");

	static const GLfloat g_frontwheel_buffer_data[] = {
		-0.5f, 0.1f, 0.3f,
		-0.3f, 0.1f, 0.3f,
		-0.3f, -0.1f, 0.3f,
		-0.3f, -0.1f, 0.3f,
		-0.5f, -0.1f, 0.3f,
		-0.5f, 0.1f, 0.3f,

		-0.5f, 0.1f, 0.2f,
		-0.3f, 0.1f, 0.2f,
		-0.3f, -0.1f, 0.2f,
		-0.3f, -0.1f, 0.2f,
		-0.5f, -0.1f, 0.2f,
		-0.5f, 0.1f, 0.2f,

		-0.5f, 0.1f, -0.3f,
		-0.3f, 0.1f, -0.3f,
		-0.3f, -0.1f, -0.3f,
		-0.3f, -0.1f, -0.3f,
		-0.5f, -0.1f, -0.3f,
		-0.5f, 0.1f, -0.3f,

		-0.5f, 0.1f, -0.2f,
		-0.3f, 0.1f, -0.2f,
		-0.3f, -0.1f, -0.2f,
		-0.3f, -0.1f, -0.2f,
		-0.5f, -0.1f, -0.2f,
		-0.5f, 0.1f, -0.2f,
		//penghubung
		-0.3f, 0.1f, 0.3f,
		-0.3f, 0.1f, 0.2f,
		-0.3f, -0.1f, 0.2f,
		-0.3f, -0.1f, 0.2f,
		-0.3f, -0.1f, 0.3f,
		-0.3f, 0.1f, 0.3f,

		-0.5f, 0.1f, 0.3f,
		-0.5f, 0.1f, 0.2f,
		-0.5f, -0.1f, 0.2f,
		-0.5f, -0.1f, 0.2f,
		-0.5f, -0.1f, 0.3f,
		-0.5f, 0.1f, 0.3f,

		-0.5f, 0.1f, 0.3f,
		-0.5f, 0.1f, 0.2f,
		-0.3f, 0.1f, 0.2f,
		-0.3f, 0.1f, 0.2f,
		-0.3f, 0.1f, 0.3f,
		-0.5f, 0.1f, 0.3f,

		-0.5f, -0.1f, 0.3f,
		-0.5f, -0.1f, 0.2f,
		-0.3f, -0.1f, 0.2f,
		-0.3f, -0.1f, 0.2f,
		-0.3f, -0.1f, 0.3f,
		-0.5f, -0.1f, 0.3f,

		-0.3f, 0.1f, -0.3f,
		-0.3f, 0.1f, -0.2f,
		-0.3f, -0.1f, -0.2f,
		-0.3f, -0.1f, -0.2f,
		-0.3f, -0.1f, -0.3f,
		-0.3f, 0.1f, -0.3f,

		-0.5f, 0.1f, -0.3f,
		-0.5f, 0.1f, -0.2f,
		-0.5f, -0.1f, -0.2f,
		-0.5f, -0.1f, -0.2f,
		-0.5f, -0.1f, -0.3f,
		-0.5f, 0.1f, -0.3f,

		-0.5f, 0.1f, -0.3f,
		-0.5f, 0.1f, -0.2f,
		-0.3f, 0.1f, -0.2f,
		-0.3f, 0.1f, -0.2f,
		-0.3f, 0.1f, -0.3f,
		-0.5f, 0.1f, -0.3f,

		-0.5f, -0.1f, -0.3f,
		-0.5f, -0.1f, -0.2f,
		-0.3f, -0.1f, -0.2f,
		-0.3f, -0.1f, -0.2f,
		-0.3f, -0.1f, -0.3f,
		-0.5f, -0.1f, -0.3f,
	};

	static const GLfloat g_backwheel_buffer_data[] = {
		0.3f, 0.1f, 0.3f,
		0.5f, 0.1f, 0.3f,
		0.5f, -0.1f, 0.3f,
		0.5f, -0.1f, 0.3f,
		0.3f, -0.1f, 0.3f,
		0.3f, 0.1f, 0.3f,

		0.3f, 0.1f, 0.2f,
		0.5f, 0.1f, 0.2f,
		0.5f, -0.1f, 0.2f,
		0.5f, -0.1f, 0.2f,
		0.3f, -0.1f, 0.2f,
		0.3f, 0.1f, 0.2f,

		0.3f, 0.1f, -0.3f,
		0.5f, 0.1f, -0.3f,
		0.5f, -0.1f, -0.3f,
		0.5f, -0.1f, -0.3f,
		0.3f, -0.1f, -0.3f,
		0.3f, 0.1f, -0.3f,

		0.3f, 0.1f, -0.2f,
		0.5f, 0.1f, -0.2f,
		0.5f, -0.1f, -0.2f,
		0.5f, -0.1f, -0.2f,
		0.3f, -0.1f, -0.2f,
		0.3f, 0.1f, -0.2f,
		//penghubung
		0.5f, 0.1f, 0.3f,
		0.5f, 0.1f, 0.2f,
		0.5f, -0.1f, 0.2f,
		0.5f, -0.1f, 0.2f,
		0.5f, -0.1f, 0.3f,
		0.5f, 0.1f, 0.3f,

		0.3f, 0.1f, 0.3f,
		0.3f, 0.1f, 0.2f,
		0.3f, -0.1f, 0.2f,
		0.3f, -0.1f, 0.2f,
		0.3f, -0.1f, 0.3f,
		0.3f, 0.1f, 0.3f,

		0.3f, 0.1f, 0.3f,
		0.3f, 0.1f, 0.2f,
		0.5f, 0.1f, 0.2f,
		0.5f, 0.1f, 0.2f,
		0.5f, 0.1f, 0.3f,
		0.3f, 0.1f, 0.3f,

		0.3f, -0.1f, 0.3f,
		0.3f, -0.1f, 0.2f,
		0.5f, -0.1f, 0.2f,
		0.5f, -0.1f, 0.2f,
		0.5f, -0.1f, 0.3f,
		0.3f, -0.1f, 0.3f,

		0.5f, 0.1f, -0.3f,
		0.5f, 0.1f, -0.2f,
		0.5f, -0.1f, -0.2f,
		0.5f, -0.1f, -0.2f,
		0.5f, -0.1f, -0.3f,
		0.5f, 0.1f, -0.3f,

		0.3f, 0.1f, -0.3f,
		0.3f, 0.1f, -0.2f,
		0.3f, -0.1f, -0.2f,
		0.3f, -0.1f, -0.2f,
		0.3f, -0.1f, -0.3f,
		0.3f, 0.1f, -0.3f,

		0.3f, 0.1f, -0.3f,
		0.3f, 0.1f, -0.2f,
		0.5f, 0.1f, -0.2f,
		0.5f, 0.1f, -0.2f,
		0.5f, 0.1f, -0.3f,
		0.3f, 0.1f, -0.3f,

		0.3f, -0.1f, -0.3f,
		0.3f, -0.1f, -0.2f,
		0.5f, -0.1f, -0.2f,
		0.5f, -0.1f, -0.2f,
		0.5f, -0.1f, -0.3f,
		0.3f, -0.1f, -0.3f,
	};


	// An array of 3 vectors which represents 3 vertices
	static const GLfloat g_vertex_buffer_data[] = {
		-0.9f, 0.0f, 0.2f, //Sisi kiri
		0.9f, 0.0f, 0.2f,
		-0.9f, 0.2f, 0.2f,
		0.9f, 0.2f, 0.2f,
		0.9f, 0.0f, 0.2f,
		-0.9f, 0.2f, 0.2f,
		-0.4f, 0.2f, 0.2f,
		0.9f, 0.2f, 0.2f,
		0.4f, 0.4f, 0.2f,
		0.4f, 0.4f, 0.2f,
		-0.2f, 0.4f, 0.2f,
		-0.4f, 0.2f, 0.2f,

		-0.9f, 0.0f, -0.2f, //Sisi kanan
		0.9f, 0.0f, -0.2f,
		-0.9f, 0.2f, -0.2f,
		0.9f, 0.2f, -0.2f,
		0.9f, 0.0f, -0.2f,
		-0.9f, 0.2f, -0.2f,
		-0.4f, 0.2f, -0.2f,
		0.9f, 0.2f, -0.2f,
		0.4f, 0.4f, -0.2f,
		0.4f, 0.4f, -0.2f,
		-0.2f, 0.4f, -0.2f,
		-0.4f, 0.2f, -0.2f,

		-0.9f, 0.2f, 0.2f,  //bumper
		-0.9f, 0.0f, 0.2f,
		-0.9f, 0.0f, -0.2f,
		-0.9f, 0.0f, -0.2f,
		-0.9f, 0.2f, -0.2f,
		-0.9f, 0.2f, 0.2f,

		-0.9f, 0.2f, 0.2f,  //kap mobil
		-0.4f, 0.2f, 0.2f,
		-0.4f, 0.2f, -0.2f,
		-0.4f, 0.2f, -0.2f,
		-0.9f, 0.2f, -0.2f,
		-0.9f, 0.2f, 0.2f,

		-0.4f, 0.2f, 0.2f,  //kaca depan
		-0.2f, 0.4f, 0.2f,
		-0.2f, 0.4f, -0.2f,
		-0.2f, 0.4f, -0.2f,
		-0.4f, 0.2f, -0.2f,
		-0.4f, 0.2f, 0.2f,

		-0.2f, 0.4f, 0.2f,  //atap
		0.4f, 0.4f, 0.2f,
		0.4f, 0.4f, -0.2f,
		0.4f, 0.4f, -0.2f,
		-0.2f, 0.4f, -0.2f,
		-0.2f, 0.4f, 0.2f,

		0.4f, 0.4f, 0.2f, //kaca belakang
		0.9f, 0.2f, 0.2f,
		0.9f, 0.2f, -0.2f,
		0.9f, 0.2f, -0.2f,
		0.4f, 0.4f, -0.2f,
		0.4f, 0.4f, 0.2f, 

		0.9f, 0.2f, 0.2f, //belakang
		0.9f, 0.0f, 0.2f,
		0.9f, 0.0f, -0.2f,
		0.9f, 0.0f, -0.2f,
		0.9f, 0.2f, -0.2f,
		0.9f, 0.2f, 0.2f,

		-0.9f, 0.0f, 0.2f, //alas
		0.9f, 0.0f, 0.2f,
		0.9f, 0.0f, -0.2f,
		0.9f, 0.0f, -0.2f,
		-0.9f, 0.0f, -0.2f,
		-0.9f, 0.0f, 0.2f
	};

	static const GLfloat g_normal_buffer_data[] = {
		0.0f, 0.0f, -1.0f, //Sisi kiri
		0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, -1.0f,

		0.0f, 0.0f, 1.0f, //Sisi kanan
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,

		-1.0f, 0.0f, 0.0f,  //bumper
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,

		0.0f, 1.0f, 0.0f,  //kap mobil
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,

		-1.0f, 0.0f, 0.0f, //kaca depan
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,

		0.0f, 1.0f, 0.0f,  //atap
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,

		1.0f, 0.0f, 0.0f, //kaca belakang
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,

		1.0f, 0.0f, 0.0f, //belakang
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,

		0.0f, -1.0f, 0.0f, //alas
		0.0f, -1.0f, 0.0f,
		0.0f, -1.0f, 0.0f,
		0.0f, -1.0f, 0.0f,
		0.0f, -1.0f, 0.0f,
		0.0f, -1.0f, 0.0f,
	};

	static const GLfloat g_uv_buffer_data[] = {
		0.0f, 0.0f, //Sisi kiri
		1.0f, 0.0f,
		0.0f, 0.11111f,
		1.0f, 0.11111f,
		1.0f, 0.0f,
		0.0f, 0.11111f,
		0.27778f, 0.11111f,
		1.0f, 0.11111f,
		0.72222f, 0.22222f,
		0.72222f, 0.22222f,
		0.38889f, 0.22222f,
		0.27778f, 0.11111f,

		0.0f, 0.0f, //Sisi kanan
		1.0f, 0.0f,
		0.0f, 0.11111f,
		1.0f, 0.11111f,
		1.0f, 0.0f,
		0.0f, 0.11111f,
		0.27778f, 0.11111f,
		1.0f, 0.11111f,
		0.72222f, 0.22222f,
		0.72222f, 0.22222f,
		0.38889f, 0.22222f,
		0.27778f, 0.11111f,

		0.4f, 0.2f,   //bumper
		0.4f, 0.0f,
		0.0f, 0.0f, 
		0.0f, 0.0f, 
		0.0f, 0.2f, 
		0.4f, 0.2f, 

		0.0f, 0.0f,
		0.2f, 0.0f,
		0.2f, 0.2f,
		0.2f, 0.2f,
		0.0f, 0.2f,
		0.0f, 0.0f,

		0.0f, 0.0f,
		0.2f, 0.0f,
		0.2f, 0.2f,
		0.2f, 0.2f,
		0.0f, 0.2f,
		0.0f, 0.0f,

		0.0f, 0.0f,
		0.2f, 0.0f,
		0.2f, 0.2f,
		0.2f, 0.2f,
		0.0f, 0.2f,
		0.0f, 0.0f,

		0.0f, 0.0f,
		0.2f, 0.0f,
		0.2f, 0.2f,
		0.2f, 0.2f,
		0.0f, 0.2f,
		0.0f, 0.0f,

		0.0f, 0.0f,
		0.2f, 0.0f,
		0.2f, 0.2f,
		0.2f, 0.2f,
		0.0f, 0.2f,
		0.0f, 0.0f,

		0.0f, 0.0f,
		0.2f, 0.0f,
		0.2f, 0.2f,
		0.2f, 0.2f,
		0.0f, 0.2f,
		0.0f, 0.0f,
	};

	static const GLfloat g_wheel_uv_buffer_data[] = {
		0.0f, 0.2f,
		0.2f, 0.2f,
		0.2f, 0.0f,
		0.2f, 0.0f,
		0.0f, 0.0f,
		0.0f, 0.2f,

		0.0f, 0.2f,
		0.2f, 0.2f,
		0.2f, 0.0f,
		0.2f, 0.0f,
		0.0f, 0.0f,
		0.0f, 0.2f,

		0.0f, 0.2f,
		0.2f, 0.2f,
		0.2f, 0.0f,
		0.2f, 0.0f,
		0.0f, 0.0f,
		0.0f, 0.2f,

		0.0f, 0.2f,
		0.2f, 0.2f,
		0.2f, 0.0f,
		0.2f, 0.0f,
		0.0f, 0.0f,
		0.0f, 0.2f,

		0.0f, 0.2f,
		0.1f, 0.2f,
		0.1f, 0.0f,
		0.1f, 0.0f,
		0.0f, 0.0f,
		0.0f, 0.2f,

		0.0f, 0.2f,
		0.1f, 0.2f,
		0.1f, 0.0f,
		0.1f, 0.0f,
		0.0f, 0.0f,
		0.0f, 0.2f,

		0.0f, 0.2f,
		0.1f, 0.2f,
		0.1f, 0.0f,
		0.1f, 0.0f,
		0.0f, 0.0f,
		0.0f, 0.2f,

		0.0f, 0.2f,
		0.1f, 0.2f,
		0.1f, 0.0f,
		0.1f, 0.0f,
		0.0f, 0.0f,
		0.0f, 0.2f,

		0.0f, 0.2f,
		0.1f, 0.2f,
		0.1f, 0.0f,
		0.1f, 0.0f,
		0.0f, 0.0f,
		0.0f, 0.2f,

		0.0f, 0.2f,
		0.1f, 0.2f,
		0.1f, 0.0f,
		0.1f, 0.0f,
		0.0f, 0.0f,
		0.0f, 0.2f,

		0.0f, 0.2f,
		0.1f, 0.2f,
		0.1f, 0.0f,
		0.1f, 0.0f,
		0.0f, 0.0f,
		0.0f, 0.2f,

		0.0f, 0.2f,
		0.1f, 0.2f,
		0.1f, 0.0f,
		0.1f, 0.0f,
		0.0f, 0.0f,
		0.0f, 0.2f,
	};

	static const GLfloat g_rain_buffer_data[] = {
 		-0.5f, -0.5f, 0.0f,
 		0.5f, -0.5f, 0.0f,
 		-0.5f, 0.5f, 0.0f,
 		0.5f, 0.5f, 0.0f,
	};

	static const GLfloat g_smoke_buffer_data[] = {
 		-0.5f, -0.5f, 0.0f,
 		0.5f, -0.5f, 0.0f,
 		-0.5f, 0.5f, 0.0f,
 		0.5f, 0.5f, 0.0f,
	};




	GLuint billboard_vertex_buffer;
	glGenBuffers(1, &billboard_vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, billboard_vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_rain_buffer_data), g_rain_buffer_data, GL_STATIC_DRAW);

	// The VBO containing the positions and sizes of the particles
	GLuint particles_position_buffer;
	glGenBuffers(1, &particles_position_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, particles_position_buffer);
	// Initialize with empty (NULL) buffer : it will be updated later, each frame.
	glBufferData(GL_ARRAY_BUFFER, MaxParticles * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);

	// The VBO containing the colors of the particles
	GLuint particles_color_buffer;
	glGenBuffers(1, &particles_color_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, particles_color_buffer);
	// Initialize with empty (NULL) buffer : it will be updated later, each frame.
	glBufferData(GL_ARRAY_BUFFER, MaxParticles * 4 * sizeof(GLubyte), NULL, GL_STREAM_DRAW);


	GLuint billboard_vertex_buffer2;
	glGenBuffers(1, &billboard_vertex_buffer2);
	glBindBuffer(GL_ARRAY_BUFFER, billboard_vertex_buffer2);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_smoke_buffer_data), g_smoke_buffer_data, GL_STATIC_DRAW);

	// The VBO containing the positions and sizes of the particles
	GLuint particles_position_buffer2;
	glGenBuffers(1, &particles_position_buffer2);
	glBindBuffer(GL_ARRAY_BUFFER, particles_position_buffer2);
	// Initialize with empty (NULL) buffer : it will be updated later, each frame.
	glBufferData(GL_ARRAY_BUFFER, MaxParticles * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);

	// The VBO containing the colors of the particles
	GLuint particles_color_buffer2;
	glGenBuffers(1, &particles_color_buffer2);
	glBindBuffer(GL_ARRAY_BUFFER, particles_color_buffer2);
	// Initialize with empty (NULL) buffer : it will be updated later, each frame.
	glBufferData(GL_ARRAY_BUFFER, MaxParticles * 4 * sizeof(GLubyte), NULL, GL_STREAM_DRAW);





	// This will identify our vertex buffer
	GLuint vertexbuffer;
	// Generate 1 buffer, put the resulting identifier in vertexbuffer
	glGenBuffers(1, &vertexbuffer);
	// The following commands will talk about our 'vertexbuffer' buffer
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	// Give our vertices to OpenGL.
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

	// GLuint colorbuffer;
	// glGenBuffers(1, &colorbuffer);
	// glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
	// glBufferData(GL_ARRAY_BUFFER, sizeof(g_color_buffer_data), g_color_buffer_data, GL_STATIC_DRAW);

	GLuint frontwheelvertexbuffer;
	glGenBuffers(1, &frontwheelvertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, frontwheelvertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_frontwheel_buffer_data), g_frontwheel_buffer_data, GL_STATIC_DRAW);

	GLuint backwheelvertexbuffer;
	glGenBuffers(1, &backwheelvertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, backwheelvertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_backwheel_buffer_data), g_backwheel_buffer_data, GL_STATIC_DRAW);

	GLuint uvbuffer;
	glGenBuffers(1, &uvbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_uv_buffer_data), g_uv_buffer_data, GL_STATIC_DRAW);

	GLuint wheeluvbuffer;
	glGenBuffers(1, &wheeluvbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, wheeluvbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_wheel_uv_buffer_data), g_wheel_uv_buffer_data, GL_STATIC_DRAW);

	GLuint normalbuffer;
	glGenBuffers(1, &normalbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_normal_buffer_data), g_normal_buffer_data, GL_STATIC_DRAW);

	// Get a handle for our "LightPosition" uniform
	glUseProgram(programID);
	GLuint LightID = glGetUniformLocation(programID, "LightPosition_worldspace");

	float angle = 0;
	double lastTime = glfwGetTime();
	double lastTime2 = glfwGetTime();
	do{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		//-----------------------rain------------------------------------------
		double currentTime = glfwGetTime();
		double delta = currentTime - lastTime;
		lastTime = currentTime;

		computeMatricesFromInputs();
		glm::mat4 RainProjectionMatrix = getProjectionMatrix();
		glm::mat4 RainViewMatrix = getViewMatrix();

		// We will need the camera's position in order to sort the particles
		// w.r.t the camera's distance.
		// There should be a getCameraPosition() function in common/controls.cpp, 
		// but this works too.
		glm::vec3 CameraPosition(glm::inverse(RainViewMatrix)[3]);

		glm::mat4 ViewProjectionMatrix = RainProjectionMatrix * RainViewMatrix;


		// Generate 10 new particule each millisecond,
		// but limit this to 16 ms (60 fps), or if you have 1 long frame (1sec),
		// newparticles will be huge and the next frame even longer.
		int newparticles = (int)(delta*10000.0);
		if (newparticles > (int)(0.016f*10000.0))
			newparticles = (int)(0.016f*10000.0);
		
		for(int i=0; i<newparticles; i++){
			int particleIndex = FindUnusedParticle();
			ParticlesContainer[particleIndex].life = 5.0f; // This particle will live 5 seconds.
			ParticlesContainer[particleIndex].pos = glm::vec3(0,3.0f,0.0f);

			float spread = 1.5f;
			glm::vec3 maindir = glm::vec3(0.0f, 0.0f, 0.0f);
			// Very bad way to generate a random direction; 
			// See for instance http://stackoverflow.com/questions/5408276/python-uniform-spherical-distribution instead,
			// combined with some user-controlled parameters (main direction, spread, etc)
			glm::vec3 randomdir = glm::vec3(
				(rand()%2000 - 1000.0f)/1000.0f,
				(rand()%2000 - 1000.0f)/1000.0f,
				(rand()%2000 - 1000.0f)/1000.0f
			);
			
			ParticlesContainer[particleIndex].speed = maindir + randomdir*spread;


			// Very bad way to generate a random color
			ParticlesContainer[particleIndex].r = rand() % 256;
			ParticlesContainer[particleIndex].g = rand() % 256;
			ParticlesContainer[particleIndex].b = rand() % 256;
			ParticlesContainer[particleIndex].a = (rand() % 256) / 3;

			ParticlesContainer[particleIndex].size = (rand()%1000)/200000.0f + 0.1f;
			
		}

		// Simulate all particles
		int ParticlesCount = 0;
		for(int i=0; i<MaxParticles; i++){

			Particle& p = ParticlesContainer[i]; // shortcut

			if(p.life > 0.0f){

				// Decrease life
				p.life -= delta;
				if (p.life > 0.0f){

					if ((p.pos.y <= 0.6f && p.pos.x > -0.5f && p.pos.x < 0.5f && p.pos.z < 0.5f && p.pos.z > -0.5f) || 
						(p.pos.y <= 0.3f && p.pos.x > -1 && p.pos.x < -0.5f && p.pos.z < 0.5f && p.pos.z > -0.5f) ||
						(p.pos.y <= 0.3f && p.pos.x > 0.5f && p.pos.x < 1 && p.pos.z < 0.5f && p.pos.z > -0.5f)) {
						// p.cameradistance = -1.0f;
						p.speed = p.speed * glm::vec3(1,-0.2f,0);
					}  
					{

						// Simulate simple physics : gravity only, no collisions
						p.speed += glm::vec3(0.0f,-9.81f, 0.0f) * (float)delta * 0.5f;
						p.pos += p.speed * (float)delta;
						p.cameradistance = glm::length2( p.pos - CameraPosition );
						//ParticlesContainer[i].pos += glm::vec3(0.0f,10.0f, 0.0f) * (float)delta;

						// Fill the GPU buffer
						g_particule_position_size_data[4*ParticlesCount+0] = p.pos.x;
						g_particule_position_size_data[4*ParticlesCount+1] = p.pos.y;
						g_particule_position_size_data[4*ParticlesCount+2] = p.pos.z;
													   
						g_particule_position_size_data[4*ParticlesCount+3] = p.size;
													   
						g_particule_color_data[4*ParticlesCount+0] = p.r;
						g_particule_color_data[4*ParticlesCount+1] = p.g;
						g_particule_color_data[4*ParticlesCount+2] = p.b;
						g_particule_color_data[4*ParticlesCount+3] = p.a;
					}

				}else{
					// Particles that just died will be put at the end of the buffer in SortParticles();
					p.cameradistance = -1.0f;
				}

				ParticlesCount++;

			}
		}

		SortParticles();

		//printf("%d ",ParticlesCount);

		// Update the buffers that OpenGL uses for rendering.
		// There are much more sophisticated means to stream data from the CPU to the GPU, 
		// but this is outside the scope of this tutorial.
		// http://www.opengl.org/wiki/Buffer_Object_Streaming

		glBindBuffer(GL_ARRAY_BUFFER, particles_position_buffer);
		glBufferData(GL_ARRAY_BUFFER, MaxParticles * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW); // Buffer orphaning, a common way to improve streaming perf. See above link for details.
		glBufferSubData(GL_ARRAY_BUFFER, 0, ParticlesCount * sizeof(GLfloat) * 4, g_particule_position_size_data);

		glBindBuffer(GL_ARRAY_BUFFER, particles_color_buffer);
		glBufferData(GL_ARRAY_BUFFER, MaxParticles * 4 * sizeof(GLubyte), NULL, GL_STREAM_DRAW); // Buffer orphaning, a common way to improve streaming perf. See above link for details.
		glBufferSubData(GL_ARRAY_BUFFER, 0, ParticlesCount * sizeof(GLubyte) * 4, g_particule_color_data);


		glEnable(GL_BLEND);
		//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// Use our shader
		glUseProgram(rainProgramID);

		// Bind our texture in Texture Unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, rainTexture);
		// Set our "myTextureSampler" sampler to use Texture Unit 0
		glUniform1i(rainTextureID, 0);

		// Same as the billboards tutorial
		glUniform3f(CameraRight_worldspace_ID, RainViewMatrix[0][0], RainViewMatrix[1][0], RainViewMatrix[2][0]);
		glUniform3f(CameraUp_worldspace_ID   , RainViewMatrix[0][1], RainViewMatrix[1][1], RainViewMatrix[2][1]);

		glUniformMatrix4fv(ViewProjMatrixID, 1, GL_FALSE, &ViewProjectionMatrix[0][0]);

		// 1rst attribute buffer : vertices
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, billboard_vertex_buffer);
		glVertexAttribPointer(
			0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);
		
		// 2nd attribute buffer : positions of particles' centers
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, particles_position_buffer);
		glVertexAttribPointer(
			1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
			4,                                // size : x + y + z + size => 4
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);

		// 3rd attribute buffer : particles' colors
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, particles_color_buffer);
		glVertexAttribPointer(
			2,                                // attribute. No particular reason for 1, but must match the layout in the shader.
			4,                                // size : r + g + b + a => 4
			GL_UNSIGNED_BYTE,                 // type
			GL_TRUE,                          // normalized?    *** YES, this means that the unsigned char[4] will be accessible with a vec4 (floats) in the shader ***
			0,                                // stride
			(void*)0                          // array buffer offset
		);

		// These functions are specific to glDrawArrays*Instanced*.
		// The first parameter is the attribute buffer we're talking about.
		// The second parameter is the "rate at which generic vertex attributes advance when rendering multiple instances"
		// http://www.opengl.org/sdk/docs/man/xhtml/glVertexAttribDivisor.xml
		glVertexAttribDivisor(0, 0); // particles vertices : always reuse the same 4 vertices -> 0
		glVertexAttribDivisor(1, 1); // positions : one per quad (its center)                 -> 1
		glVertexAttribDivisor(2, 1); // color : one per quad                                  -> 1

		// Draw the particules !
		// This draws many times a small triangle_strip (which looks like a quad).
		// This is equivalent to :
		// for(i in ParticlesCount) : glDrawArrays(GL_TRIANGLE_STRIP, 0, 4), 
		// but faster.
		glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, ParticlesCount);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);



		//-----------------------smoke------------------------------------------
		double currentTime2 = glfwGetTime();
		double delta2 = currentTime2 - lastTime2;
		lastTime2 = currentTime2;

		computeMatricesFromInputs();
		glm::mat4 SmokeProjectionMatrix = getProjectionMatrix();
		glm::mat4 SmokeViewMatrix = getViewMatrix();

		// We will need the camera's position in order to sort the particles
		// w.r.t the camera's distance.
		// There should be a getCameraPosition() function in common/controls.cpp, 
		// but this works too.
		glm::vec3 CameraPosition2(glm::inverse(SmokeViewMatrix)[3]);

		glm::mat4 ViewProjectionMatrix2 = SmokeProjectionMatrix * SmokeViewMatrix;


		// Generate 10 new particule each millisecond,
		// but limit this to 16 ms (60 fps), or if you have 1 long frame (1sec),
		// newparticles will be huge and the next frame even longer.
		int newparticles2 = (int)(delta2*10000.0);
		if (newparticles2 > (int)(0.016f*10000.0))
			newparticles2 = (int)(0.016f*10000.0);
		
		for(int i=0; i<newparticles2; i++){
			int particleIndex = FindUnusedParticle2();
			ParticlesContainer2[particleIndex].life = 5.0f; // This particle will live 5 seconds.
			ParticlesContainer2[particleIndex].pos = glm::vec3(1.0f,0.0f,0.0f);

			float spread = 0.2f;
			glm::vec3 maindir = glm::vec3(0.0f, 0.0f, 0.0f);
			// Very bad way to generate a random direction; 
			// See for instance http://stackoverflow.com/questions/5408276/python-uniform-spherical-distribution instead,
			// combined with some user-controlled parameters (main direction, spread, etc)
			glm::vec3 randomdir = glm::vec3(
				0.0f,
				(rand()%2000 - 1000.0f)/1000.0f,
				(rand()%2000 - 1000.0f)/1000.0f
			);
			
			ParticlesContainer2[particleIndex].speed = maindir + randomdir*spread;


			// Very bad way to generate a random color
			ParticlesContainer2[particleIndex].r = rand() % 256;
			ParticlesContainer2[particleIndex].g = rand() % 256;
			ParticlesContainer2[particleIndex].b = rand() % 256;
			ParticlesContainer2[particleIndex].a = (rand() % 256) / 3;

			ParticlesContainer2[particleIndex].size = (rand()%1000)/200000.0f + 0.1f;
			
		}

		// Simulate all particles
		int ParticlesCount2 = 0;
		for(int i=0; i<MaxParticles; i++){

			Particle& p = ParticlesContainer2[i]; // shortcut

			if(p.life > 0.0f){

				// Decrease life
				p.life -= delta2;
				if (p.life > 0.0f){

					if ((p.pos.y <= 0.6f && p.pos.x > -0.5f && p.pos.x < 0.5f) || 
						(p.pos.y <= 0.3f && p.pos.x > -1 && p.pos.x < -0.5f) ||
						(p.pos.y <= 0.3f && p.pos.x > 0.5f && p.pos.x < 1)) {
						// p.cameradistance = -1.0f;
						p.speed = p.speed * glm::vec3(1,-0.2f,0);
					}  
					{

						// Simulate simple physics : gravity only, no collisions
						p.speed += glm::vec3(2.0f,0.5f, 0.0f) * (float)delta2 * 0.5f;
						p.pos += p.speed * (float)delta2;
						p.cameradistance = glm::length2( p.pos - CameraPosition2 );
						//ParticlesContainer[i].pos += glm::vec3(0.0f,10.0f, 0.0f) * (float)delta;

						// Fill the GPU buffer
						g_particule_position_size_data2[4*ParticlesCount2+0] = p.pos.x;
						g_particule_position_size_data2[4*ParticlesCount2+1] = p.pos.y;
						g_particule_position_size_data2[4*ParticlesCount2+2] = p.pos.z;
													   
						g_particule_position_size_data2[4*ParticlesCount2+3] = p.size;
													   
						g_particule_color_data2[4*ParticlesCount2+0] = p.r;
						g_particule_color_data2[4*ParticlesCount2+1] = p.g;
						g_particule_color_data2[4*ParticlesCount2+2] = p.b;
						g_particule_color_data2[4*ParticlesCount2+3] = p.a;
					}

				}else{
					// Particles that just died will be put at the end of the buffer in SortParticles();
					p.cameradistance = -1.0f;
				}

				ParticlesCount2++;

			}
		}

		SortParticles2();

		//printf("%d ",ParticlesCount);

		// Update the buffers that OpenGL uses for rendering.
		// There are much more sophisticated means to stream data from the CPU to the GPU, 
		// but this is outside the scope of this tutorial.
		// http://www.opengl.org/wiki/Buffer_Object_Streaming

		glBindBuffer(GL_ARRAY_BUFFER, particles_position_buffer2);
		glBufferData(GL_ARRAY_BUFFER, MaxParticles * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW); // Buffer orphaning, a common way to improve streaming perf. See above link for details.
		glBufferSubData(GL_ARRAY_BUFFER, 0, ParticlesCount2 * sizeof(GLfloat) * 4, g_particule_position_size_data2);

		glBindBuffer(GL_ARRAY_BUFFER, particles_color_buffer2);
		glBufferData(GL_ARRAY_BUFFER, MaxParticles * 4 * sizeof(GLubyte), NULL, GL_STREAM_DRAW); // Buffer orphaning, a common way to improve streaming perf. See above link for details.
		glBufferSubData(GL_ARRAY_BUFFER, 0, ParticlesCount2 * sizeof(GLubyte) * 4, g_particule_color_data2);


		glEnable(GL_BLEND);
		//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// Use our shader
		glUseProgram(smokeProgramID);

		// Bind our texture in Texture Unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, smokeTexture);
		// Set our "myTextureSampler" sampler to use Texture Unit 0
		glUniform1i(smokeTextureID, 0);

		// Same as the billboards tutorial
		glUniform3f(CameraRight_worldspace_ID2, SmokeViewMatrix[0][0], SmokeViewMatrix[1][0], SmokeViewMatrix[2][0]);
		glUniform3f(CameraUp_worldspace_ID2   , SmokeViewMatrix[0][1], SmokeViewMatrix[1][1], SmokeViewMatrix[2][1]);

		glUniformMatrix4fv(ViewProjMatrixID2, 1, GL_FALSE, &ViewProjectionMatrix2[0][0]);

		// 1rst attribute buffer : vertices
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, billboard_vertex_buffer2);
		glVertexAttribPointer(
			0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);
		
		// 2nd attribute buffer : positions of particles' centers
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, particles_position_buffer2);
		glVertexAttribPointer(
			1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
			4,                                // size : x + y + z + size => 4
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);

		// 3rd attribute buffer : particles' colors
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, particles_color_buffer2);
		glVertexAttribPointer(
			2,                                // attribute. No particular reason for 1, but must match the layout in the shader.
			4,                                // size : r + g + b + a => 4
			GL_UNSIGNED_BYTE,                 // type
			GL_TRUE,                          // normalized?    *** YES, this means that the unsigned char[4] will be accessible with a vec4 (floats) in the shader ***
			0,                                // stride
			(void*)0                          // array buffer offset
		);

		// These functions are specific to glDrawArrays*Instanced*.
		// The first parameter is the attribute buffer we're talking about.
		// The second parameter is the "rate at which generic vertex attributes advance when rendering multiple instances"
		// http://www.opengl.org/sdk/docs/man/xhtml/glVertexAttribDivisor.xml
		glVertexAttribDivisor(0, 0); // particles vertices : always reuse the same 4 vertices -> 0
		glVertexAttribDivisor(1, 1); // positions : one per quad (its center)                 -> 1
		glVertexAttribDivisor(2, 1); // color : one per quad                                  -> 1

		// Draw the particules !
		// This draws many times a small triangle_strip (which looks like a quad).
		// This is equivalent to :
		// for(i in ParticlesCount) : glDrawArrays(GL_TRIANGLE_STRIP, 0, 4), 
		// but faster.
		glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, ParticlesCount2);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);

		
		//--------------light--------------------------------------------------
		glm::vec3 lightPos = glm::vec3(-2,0,-2);
		glUniform3f(LightID, lightPos.x, lightPos.y, lightPos.z);

		//--------------car----------------------------------------------------
		glUseProgram(programID);

		// Compute the MVP matrix from keyboard and mouse input
		computeMatricesFromInputs();
		glm::mat4 ProjectionMatrix = getProjectionMatrix();
		glm::mat4 ViewMatrix = getViewMatrix();
		glm::mat4 ModelMatrix = glm::mat4(1.0);
		glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

		// Send our transformation to the currently bound shader, 
		// in the "MVP" uniform
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);

		// Bind our texture in Texture Unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
		// Set our "myTextureSampler" sampler to use Texture Unit 0
		glUniform1i(TextureID, 0);

		// 1rst attribute buffer : vertices
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glVertexAttribPointer(
		0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		(void*)0            // array buffer offset
		);

		// 2nd attribute buffer : texture
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
		glVertexAttribPointer(
			1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
			3,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);

		// 3rd attribute buffer : normals
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
		glVertexAttribPointer(
			2,                                // attribute
			3,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);

		// Draw the triangle !
		glDrawArrays(GL_TRIANGLES, 0, 66*3); // Starting from vertex 0; 3 vertices total -> 1 triangle
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);

		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);


		//------------------front wheel------------------------------------------------------------------------
		// Compute the MVP matrix from keyboard and mouse input
		glUseProgram(frontwheelprogramID);
		
		computeMatricesFromInputs();
		glm::mat4 ProjectionMatrixFrontWheel = getProjectionMatrix();
		glm::mat4 ViewMatrixFrontWheel = getViewMatrix();
		glm::mat4 ModelMatrixFrontWheel = glm::mat4(1.0);

		glm::mat4 FrontwheelSubTranslation = glm::translate(glm::mat4(1.0f), glm::vec3(0.4f, 0.0f, 0.0f));
		glm::mat4 FrontwheelRotation = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f));
		glm::mat4 FrontwheelAddTranslation = glm::translate(glm::mat4(1.0f), glm::vec3(-0.4f, 0.0f, 0.0f));
		glm::mat4 FrontwheelTransform = FrontwheelAddTranslation * FrontwheelRotation * FrontwheelSubTranslation;
		
		glm::mat4 MVPFrontWheel = ProjectionMatrixFrontWheel * ViewMatrixFrontWheel * ModelMatrixFrontWheel * FrontwheelTransform;

		// Send our transformation to the currently bound shader, 
		// in the "MVP" uniform
		glUniformMatrix4fv(FrontWheelMatrixID, 1, GL_FALSE, &MVPFrontWheel[0][0]);

		// Bind our texture in Texture Unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
		// Set our "myTextureSampler" sampler to use Texture Unit 0
		glUniform1i(TextureID, 0);

		// 1rst attribute buffer : vertices
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, frontwheelvertexbuffer);
		glVertexAttribPointer(
		0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		(void*)0            // array buffer offset
		);

		// 2nd attribute buffer : texture
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, wheeluvbuffer);
		glVertexAttribPointer(
			1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
			3,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);

		// Draw the triangle !
		glDrawArrays(GL_TRIANGLES, 0, 72*3); // Starting from vertex 0; 3 vertices total -> 1 triangle
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);

		glUniformMatrix4fv(FrontWheelMatrixID, 1, GL_FALSE, &MVPFrontWheel[0][0]);

		
		
		//--------------------------------------backwheel-----------------------------------------------------
		// Compute the MVP matrix from keyboard and mouse input
		glUseProgram(backwheelprogramID);
		
		computeMatricesFromInputs();
		glm::mat4 ProjectionMatrixBackWheel = getProjectionMatrix();
		glm::mat4 ViewMatrixBackWheel = getViewMatrix();
		glm::mat4 ModelMatrixBackWheel = glm::mat4(1.0);

		glm::mat4 BackwheelSubTranslation = glm::translate(glm::mat4(1.0f), glm::vec3(-0.4f, 0.0f, 0.0f));
		glm::mat4 BackwheelRotation = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f));
		glm::mat4 BackwheelAddTranslation = glm::translate(glm::mat4(1.0f), glm::vec3(0.4f, 0.0f, 0.0f));
		glm::mat4 BackwheelTransform = BackwheelAddTranslation * BackwheelRotation * BackwheelSubTranslation;

		glm::mat4 MVPBackWheel = ProjectionMatrixBackWheel * ViewMatrixBackWheel * ModelMatrixBackWheel * BackwheelTransform;

		// Send our transformation to the currently bound shader, 
		// in the "MVP" uniform
		glUniformMatrix4fv(BackWheelMatrixID, 1, GL_FALSE, &MVPBackWheel[0][0]);

		// Bind our texture in Texture Unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
		// Set our "myTextureSampler" sampler to use Texture Unit 0
		glUniform1i(TextureID, 0);

		// 1rst attribute buffer : vertices
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, backwheelvertexbuffer);
		glVertexAttribPointer(
		0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		(void*)0            // array buffer offset
		);

		// 2nd attribute buffer : texture
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, wheeluvbuffer);
		glVertexAttribPointer(
			1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
			3,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);

		// Draw the triangle !
		glDrawArrays(GL_TRIANGLES, 0, 72*3); // Starting from vertex 0; 3 vertices total -> 1 triangle
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);

		glUniformMatrix4fv(BackWheelMatrixID, 1, GL_FALSE, &MVPBackWheel[0][0]);


		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

		angle -= 1.0f;
		
	} // Check if the ESC key was pressed or the window was closed
	while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
		   glfwWindowShouldClose(window) == 0 );

	delete[] g_particule_position_size_data;

	// Cleanup VBO and shader
	glDeleteBuffers(1, &particles_color_buffer);
	glDeleteBuffers(1, &particles_position_buffer);
	glDeleteBuffers(1, &billboard_vertex_buffer);
	glDeleteProgram(rainProgramID);
	glDeleteTextures(1, &rainTexture);


	// Cleanup VBO
	glDeleteBuffers(1, &vertexbuffer);
	glDeleteVertexArrays(1, &VertexArrayID);
	glDeleteProgram(programID);



	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}

