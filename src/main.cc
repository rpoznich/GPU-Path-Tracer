#include <GL/glew.h>
#include "terrain.h"

#include "procedure_geometry.h"
#include "render_pass.h"
#include "config.h"
#include "gui.h"
#include "jpegio.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <deque>
#include <limits>
#include <glm/gtc/random.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/io.hpp>
#include <debuggl.h>
#include <random>
#include <math.h>
#include "bvhtree.h"

#define NUM_BOXES  21
#define PHOTON_MAP_PRECISION 400
int window_width = 1280, window_height = 720;
int view_width = 1280, view_height = 720;
const std::string window_title = "Minecraft 2.0";

const char* vertex_shader =
#include "shaders/default.vert"
;

const char* terrain_vertex_shader =
#include "shaders/terrain.vert"
;

const char* geometry_shader =
#include "shaders/default.geom"
;

const char* fragment_shader =
#include "shaders/default.frag"
;

const char* quad_vs =
#include "shaders/quad.vs"
;


const char* photon_cs =
#include "shaders/photonmap.comp"
;
const char* photon_scatter_cs =
#include "shaders/photonscatter.comp"
;


const char* quad_fs =
#include "shaders/screen.fs"
;

const char* raytrace_cs =
#include "shaders/raytrace.comp"
;

const char* photon_merge_cs =
#include "shaders/photonmerge.comp"
;


const char* floor_fragment_shader =
#include "shaders/floor.frag"
;
// std::vector<int> seeds;
// std::vector<float> random_angle; //random from 0 to 2 pi
// std::vector<glm::vec2> random_alpha_beta; //random alpha beta 


const char* cube_fragment_shader =
#include "shaders/cube.frag"
;

const char* water_fragment_shader =
#include "shaders/water.frag"
;
void ErrorCallback(int error, const char* description) {
	std::cerr << "GLFW Error: " << description << "\n";
}


int nextPowerOfTwo(int x) {
	x--;
	x |= x >> 1; // handle 2 bit numbers
	x |= x >> 2; // handle 4 bit numbers
	x |= x >> 4; // handle 8 bit numbers
	x |= x >> 8; // handle 16 bit numbers
	x |= x >> 16; // handle 32 bit numbers
	x++;
	return x;
}

GLFWwindow* init_glefw()
{
	if (!glfwInit())
		exit(EXIT_FAILURE);
	glfwSetErrorCallback(ErrorCallback);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SAMPLES, 4);
	auto ret = glfwCreateWindow(window_width, window_height, window_title.data(), nullptr, nullptr);
	CHECK_SUCCESS(ret != nullptr);
	glfwMakeContextCurrent(ret);
	glewExperimental = GL_TRUE;
	CHECK_SUCCESS(glewInit() == GLEW_OK);
	glGetError();  // clear GLEW's error for it
	glfwSwapInterval(1);
	const GLubyte* renderer = glGetString(GL_RENDERER);  // get renderer string
	const GLubyte* version = glGetString(GL_VERSION);    // version as a string
	std::cout << "Renderer: " << renderer << "\n";
	std::cout << "OpenGL version supported:" << version << "\n";

	return ret;
}

GLuint createNew2DTexture(GLint internalFormat, GLint format, int width, int height)
{
	// The texture we're going to render to
	GLuint renderedTexture;
	glGenTextures(1, &renderedTexture);

	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture(GL_TEXTURE_2D, renderedTexture);

	// Give an empty image to OpenGL ( the last "0" )
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, view_width, view_height, 0, format, GL_UNSIGNED_BYTE, 0);

	// Poor filtering. Needed !
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);

	return renderedTexture;
}

GLuint createNew3DTexture(GLint internalFormat, GLint format, int width, int height, int depth)
{
	//(generate texel code omitted)
	unsigned int texname;
	glGenTextures(1, &texname);
	glBindTexture(GL_TEXTURE_3D, texname);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
	glTexImage3D(GL_TEXTURE_3D, 0, internalFormat, width, height, depth, 0, format, 
	             GL_UNSIGNED_BYTE, 0);
	return texname;
}

//render to texture
void frameBufferToTexture(unsigned char* writeto, GLuint& renderedTexture)
{
	glGenTextures(1, &renderedTexture);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0, 0, view_width, view_height, GL_RGB, GL_UNSIGNED_BYTE, writeto);
	//SaveJPEG("screenshot.jpg", view_width, view_height, gui.snapshots[gui.snapshots.size()-1].data());



	//get orignal texture
	GLint original;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &original);

	glBindTexture(GL_TEXTURE_2D, renderedTexture);


	//put the image data in texture
	glTexImage2D(GL_TEXTURE_2D, 0,GL_RGB, view_width, view_height, 0, GL_RGB, GL_UNSIGNED_BYTE, writeto);

    glGenerateMipmap(GL_TEXTURE_2D); //for sampling

    glBindTexture(GL_TEXTURE_2D, original);

}

GLuint createFrameBuffer(GLuint tex)
{
	GLuint fbo;
	glGenFramebuffers(1,&fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	return fbo;
}

//photonmaptexture
GLuint createPhotonMapTexture() {
        /* Create it */
		GLuint photonMapTexture;
		glGenTextures(1, &photonMapTexture);
  
        glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, photonMapTexture);
        glTexStorage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 1, GL_RGBA16F, PHOTON_MAP_PRECISION, PHOTON_MAP_PRECISION, 6 * NUM_BOXES * 2);
        glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, 0);

        GLuint texBuffer;
        glGenBuffers(1, &texBuffer);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, texBuffer);
        unsigned int size = 2 * 4 * PHOTON_MAP_PRECISION * PHOTON_MAP_PRECISION * 6 * NUM_BOXES * 2;
        glBufferData(GL_PIXEL_UNPACK_BUFFER, size, NULL, GL_STATIC_DRAW);
        glClearBufferSubData(GL_PIXEL_UNPACK_BUFFER, GL_RGBA16F, 0, size, GL_RGBA, GL_HALF_FLOAT, NULL);
        glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, photonMapTexture);
        glTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, 0, 0, 0, PHOTON_MAP_PRECISION, PHOTON_MAP_PRECISION, 6 * NUM_BOXES * 2,
                GL_RG, GL_HALF_FLOAT, 0L);
        glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, 0);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        glDeleteBuffers(1,&texBuffer);

        return photonMapTexture;
}



GLuint createSampler() {
		GLuint sampler;
		glGenSamplers(1, &sampler);
        glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glSamplerParameteri(sampler, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        return sampler;
}




GLuint createTerrainSSBO(const std::vector<glm::vec4>& cubes) {
	GLuint ssbo;
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_ARRAY_BUFFER, ssbo);
    unsigned char data[4 * 4 * 5 * cubes.size() / 2];
    size_t pointer = 0;
    for (size_t i = 0; i < cubes.size(); i+=2) {
    	//padding to get vec4
    	const glm::vec4& bmin = cubes[i];
    	const glm::vec4& bmax = cubes[i+1];
    	glm::vec4 kd_ = ((i/2) % 2) ? glm::vec4(0.0f,0.5f,0.5f, 1.0f) : glm::vec4(0.5,0.30,0.5,1.0f);
    	glm::vec4 ks_ = glm::vec4(0.0f,0.1f,0.1f, 1.0f);
        std::memcpy(data+pointer, &bmin, sizeof(float)* 4); //bmin
        pointer += sizeof(float) * 4;
        std::memcpy(data+pointer, &bmax, sizeof(float) * 4); //bmin
        pointer += sizeof(float) * 4;

        std::memcpy(data+pointer, &kd_, sizeof(float) * 4); //kd_
        pointer += sizeof(float) * 4;
        std::memcpy(data+pointer, &ks_, sizeof(float) * 4); //ks_
        pointer += sizeof(float) * 4;
        glm::uvec4 attr = glm::uvec4((i/2) % 5 == 0 ? 1:0,0,0,0);
        std::memcpy(data+pointer, &attr, sizeof(int) * 4);
        pointer += sizeof(int) * 4;

    }
    glBufferData(GL_ARRAY_BUFFER, 4 * 4 * 5 * cubes.size() / 2, data, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return ssbo;
}

struct BoxMaterial {
  glm::vec4 min;
  glm::vec4 max;
  glm::vec4 kd;
  glm::vec4 ks;
  glm::ivec4 attr;

   BoxMaterial(const glm::vec4& min_, const glm::vec4& max_, const glm::vec4& kd_, const glm::vec4& ks_, const glm::ivec4& attr_)
   : min(min_), max(max_), kd(kd_), ks(ks_), attr(attr_)
   {}
   BoxMaterial(glm::vec4&& min_,glm::vec4&& max_, glm::vec4&& kd_,glm::vec4&& ks_,glm::ivec4&& attr_)
   : min(min_), max(max_), kd(kd_), ks(ks_), attr(attr_)
   {}
};

BoxMaterial boxes[21] = {
  BoxMaterial(glm::vec4(43, 0.5, 43, 1.0f), glm::vec4(44, 1.5, 44, 1.0f), glm::vec4(0.0,0.5,0.5, 1.0f), glm::vec4(0.3,0.3,0.3, 1.0f), glm::ivec4(0,0,0,0)),
  BoxMaterial(glm::vec4(44, 0.0, 43, 1.0f), glm::vec4(45, 0.1, 44, 1.0f), glm::vec4(0.3,0.3,0.3, 1.0f), glm::vec4(0.3,0.3,0.3, 1.0f), glm::ivec4(0,0,0,0)),
  BoxMaterial(glm::vec4(45, 0.0, 43, 1.0f), glm::vec4(46, 0.1, 44, 1.0f), glm::vec4(0.3,0.3,0.3, 1.0f), glm::vec4(0.3,0.3,0.3, 1.0f), glm::ivec4(0,0,0,0)),
  BoxMaterial(glm::vec4(46, 0.0, 43, 1.0f), glm::vec4(47, 0.1, 44, 1.0f), glm::vec4(0.3,0.3,0.3, 1.0f), glm::vec4(0.3,0.3,0.3, 1.0f), glm::ivec4(0,0,0,0)),
  BoxMaterial(glm::vec4(47, 0.5, 43, 1.0f), glm::vec4(48, 1.5, 44, 1.0f), glm::vec4(0.3,0.3,0.3, 1.0f), glm::vec4(0.3,0.3,0.3, 1.0f), glm::ivec4(0,0,0,0)),
  BoxMaterial(glm::vec4(43, 0.0, 44, 1.0f), glm::vec4(44, 0.1, 45, 1.0f), glm::vec4(0.3,0.3,0.3, 1.0f), glm::vec4(0.3,0.3,0.3, 1.0f), glm::ivec4(0,0,0,0)),
  BoxMaterial(glm::vec4(44, 0.0, 44, 1.0f), glm::vec4(45, 0.1, 45, 1.0f), glm::vec4(0.3,0.3,0.3, 1.0f), glm::vec4(0.3,0.3,0.3, 1.0f), glm::ivec4(0,0,0,0)),
  BoxMaterial(glm::vec4(45, 0.0, 44, 1.0f), glm::vec4(46, 0.1, 45, 1.0f), glm::vec4(0.3,0.3,0.3, 1.0f), glm::vec4(0.3,0.3,0.3, 1.0f), glm::ivec4(0,0,0,0)),
  BoxMaterial(glm::vec4(46, 0.0, 44, 1.0f), glm::vec4(47, 0.1, 45, 1.0f), glm::vec4(0.3,0.3,0.3, 1.0f), glm::vec4(0.3,0.3,0.3, 1.0f), glm::ivec4(0,0,0,0)),
  BoxMaterial(glm::vec4(47, 0.0, 44, 1.0f), glm::vec4(48, 0.1, 45, 1.0f), glm::vec4(0.3,0.3,0.3, 1.0f), glm::vec4(0.3,0.3,0.3, 1.0f), glm::ivec4(0,0,0,0)),
  BoxMaterial(glm::vec4(43, 0.0, 45, 1.0f), glm::vec4(44, 0.1, 46, 1.0f), glm::vec4(0.3,0.3,0.3, 1.0f), glm::vec4(0.3,0.3,0.3, 1.0f), glm::ivec4(0,0,0,0)),
  BoxMaterial(glm::vec4(44, 0.0, 45, 1.0f), glm::vec4(45, 0.1, 46, 1.0f), glm::vec4(0.3,0.3,0.3, 1.0f), glm::vec4(0.3,0.3,0.3, 1.0f), glm::ivec4(0,0,0,0)),
  BoxMaterial(glm::vec4(45, 0.0, 45, 1.0f), glm::vec4(46, 0.1, 46, 1.0f), glm::vec4(0.3,0.3,0.3, 1.0f), glm::vec4(0.3,0.3,0.3, 1.0f), glm::ivec4(0,0,0,0)),
  BoxMaterial(glm::vec4(46, 0.0, 45, 1.0f), glm::vec4(47, 0.1, 46, 1.0f), glm::vec4(0.3,0.3,0.3, 1.0f), glm::vec4(0.3,0.3,0.3, 1.0f), glm::ivec4(0,0,0,0)),
  BoxMaterial(glm::vec4(47, 0.0, 45, 1.0f), glm::vec4(48, 0.1, 46, 1.0f), glm::vec4(0.3,0.3,0.3, 1.0f), glm::vec4(0.3,0.3,0.3, 1.0f), glm::ivec4(0,0,0,0)),
  BoxMaterial(glm::vec4(43, 0.0, 46, 1.0f), glm::vec4(44, 0.1, 47, 1.0f), glm::vec4(0.3,0.3,0.3, 1.0f), glm::vec4(0.3,0.3,0.3, 1.0f), glm::ivec4(0,0,0,0)),
  BoxMaterial(glm::vec4(44, 0.0, 46, 1.0f), glm::vec4(45, 0.1, 47, 1.0f), glm::vec4(0.3,0.3,0.3, 1.0f), glm::vec4(0.3,0.3,0.3, 1.0f), glm::ivec4(0,0,0,0)),
  BoxMaterial(glm::vec4(45, 0.0, 46, 1.0f), glm::vec4(46, 0.1, 47, 1.0f), glm::vec4(0.3,0.3,0.3, 1.0f), glm::vec4(0.3,0.3,0.3, 1.0f), glm::ivec4(0,0,0,0)),
  BoxMaterial(glm::vec4(46, 0.5, 46, 1.0f), glm::vec4(47, 1.5, 47, 1.0f), glm::vec4(0.5,0.30,0.5, 1.0f), glm::vec4(0.3,0.3,0.3, 1.0f), glm::ivec4(0,0,0,0)),
  BoxMaterial(glm::vec4(47, 0.0, 46, 1.0f), glm::vec4(48, 0.1, 47, 1.0f), glm::vec4(0.3,0.3,0.3, 1.0f), glm::vec4(0.5,0.5,0.5, 1.0f), glm::ivec4(0,0,0,0)),
  BoxMaterial(glm::vec4(45.3, 1.0, 44.5, 1.0f), glm::vec4(46.3, 2.0, 45.5, 1.0f), glm::vec4(0.2,0.2,0.04, 1.0f), glm::vec4(0.5,0.5,0.5, 1.0f), glm::ivec4(1,0,0,0))
};


GLuint createSimpleSceneSSBO()
{
	GLuint ssbo;
	glGenBuffers(1, &ssbo);
	glBindBuffer(GL_ARRAY_BUFFER, ssbo);

	unsigned char data[4 * 4 * 5 * 21];
	size_t pointer = 0;
	for(size_t i = 0; i < 21; ++i)
	{
		std::memcpy(data+pointer, &boxes[i].min, sizeof(float)* 4); //bmin
		pointer += sizeof(float) * 4;
		std::memcpy(data+pointer, &boxes[i].max, sizeof(float)* 4); //bmin
		pointer += sizeof(float) * 4;
		std::memcpy(data+pointer, &boxes[i].kd, sizeof(float)* 4); //bmin
		pointer += sizeof(float) * 4;
		std::memcpy(data+pointer, &boxes[i].ks, sizeof(float)* 4); //bmin
		pointer += sizeof(float) * 4;
		std::memcpy(data+pointer, &boxes[i].attr, sizeof(int)* 4); //bmin
		pointer += sizeof(int) * 4;
	}
	CHECK_GL_ERROR(glBufferData(GL_ARRAY_BUFFER, 4 * 4 * 5 * 21, data, GL_STATIC_DRAW));
    CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, 0));
    return ssbo;

}

GLuint createTerrainBVH(const std::vector<Box>& boxes) {

	std::vector<const Box*> boxes_ptr;
	for(size_t i = 0; i < boxes.size(); ++i)
	{
		boxes_ptr.push_back(&boxes[i]);
	}
	std::cout << boxes.size() <<std::endl;
	BVHTree tree = BVHTree(boxes_ptr);

	GLuint ssbo;
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_ARRAY_BUFFER, ssbo);
    unsigned char data[4 * 4 * 3 * tree.nodes.size()];
    std::cout << tree.nodes.size() <<std::endl;
    size_t pointer = 0;
    for (size_t i = 0; i < tree.nodes.size(); ++i) {
    	//padding to get vec4
    	glm::vec4 bmin = glm::vec4(tree.nodes[i]->bmin, 1.0f);
    	glm::vec4 bmax = glm::vec4(tree.nodes[i]->bmax, 1.0f);
    	glm::ivec4 metadata = glm::ivec4(tree.nodes[i]->left, tree.nodes[i]->right, tree.nodes[i]->box_id, -1);
    	//if(i == 0)
    	std::cout << bmin << std::endl;
        std::memcpy(data+pointer, &bmin, sizeof(float)* 4); //bmin
        pointer += sizeof(float) * 4;
        std::memcpy(data+pointer, &bmax, sizeof(float)* 4); //bmin
        pointer += sizeof(float) * 4;
        std::memcpy(data+pointer, &metadata, sizeof(int) * 4); //kd_
        pointer += sizeof(int) * 4;

    }
    CHECK_GL_ERROR(glBufferData(GL_ARRAY_BUFFER, 4 * 4 * 3 * tree.nodes.size(), data, GL_STATIC_DRAW));
    CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, 0));
    return ssbo;
}




int main(int argc, char* argv[])
{

	GLFWwindow *window = init_glefw();
	GUI gui(window, view_width, view_height);
	MatrixPointers mats; // Define MatrixPointers here for lambda to capture
	mats = gui.getMatrixPointers();

	std::vector<glm::vec4> floor_vertices;
	std::vector<glm::uvec3> floor_faces;
	create_floor(floor_vertices, floor_faces);


	//terrain cubes

	std::vector<glm::vec4> cubes;
	std::vector<glm::uvec3> cube_faces;

	Terrain terra(BOX_SIZE, num_x, num_z, GROUND);
	terra.InitializeTerrain(gui.getCamera()[0],gui.getCamera()[2]);

	terra.terrainMatrixToVector(cubes, cube_faces , gui, false, false);

	GLuint terrainSSBO = createSimpleSceneSSBO();
	std::vector<Box> boxes;
	for (size_t i = 0; i < cubes.size(); i+=2) {
		Box newBox = Box(glm::vec3(cubes[i]), glm::vec3(cubes[i+1]));
		newBox.index = i / 2;
		boxes.push_back(newBox);

	}
	GLuint bvhSSBO = createTerrainBVH(boxes);


	


	//water cubes
	std::vector<glm::vec4> water_vertices;
	std::vector<glm::uvec3> water_faces;

	//terra.createWaterPlane(water_vertices, water_faces, gui);

	glm::vec4 light_position = glm::vec4(30, 30, 30, 1.0f);
	float near_plane = 1.0f, far_plane = 200.0f;
	glm::mat4 lightProjection = glm::ortho(-200.0f, 200.0f, -200.0f, 200.0f, near_plane, far_plane);
	glm::mat4 lightView = glm::lookAt(glm::vec3(light_position), 
	                      glm::vec3( 0.0f, 0.0f,  0.0f), 
	                      glm::vec3( 0.0f, -1.0f,  0.0f));  

	glm::mat4 lightPV = lightProjection * lightView;
	

	/*
	 * In the following we are going to define several lambda functions as
	 * the data source of GLSL uniforms
	 *
	 * Introduction about lambda functions:
	 *      http://en.cppreference.com/w/cpp/language/lambda
	 *      http://www.stroustrup.com/C++11FAQ.html#lambda
	 *
	 * Note: lambda expressions cannot be converted to std::function directly
	 *       Hence we need to declare the data function explicitly.
	 *
	 * CAVEAT: DO NOT RETURN const T&, which compiles but causes
	 *         segfaults.
	 *
	 * Do not worry about the efficient issue, copy elision in C++ 17 will
	 * minimize the performance impact.
	 *
	 * More details about copy elision:
	 *      https://en.cppreference.com/w/cpp/language/copy_elision
	 */

	// FIXME: add more lambdas for data_source if you want to use RenderPass.
	//        Otherwise, do whatever you like here
	std::function<const glm::mat4*()> model_data = [&mats]() {
		return mats.model;
	};

	int start = std::chrono::duration_cast<std::chrono::milliseconds> (std::chrono::system_clock::now().time_since_epoch()).count();
	std::function<glm::mat4()> view_data = [&mats]() { return *mats.view; };
	std::function<std::vector<int>()> seeds_data = [&terra]() { return terra.getSeeds(); };
	std::function<glm::vec3()> ray00_data = [&gui]() { return gui.getEyeRay(-1,-1); };
	std::function<glm::vec3()> ray01_data = [&gui]() { return gui.getEyeRay(-1,1); };
	std::function<glm::vec3()> ray10_data = [&gui]() { return gui.getEyeRay(1,-1); };
	std::function<glm::vec3()> ray11_data = [&gui]() { return gui.getEyeRay(1,1); };
	std::function<int()> time_data = [&start]() { return std::chrono::duration_cast<std::chrono::milliseconds> (std::chrono::system_clock::now().time_since_epoch()).count() - start; };

	std::function<std::vector<float>()> random_angle_data = [&terra]() { return terra.getRandomAngles(); };
	std::function<glm::mat4()> proj_data = [&mats]() { return *mats.projection; };
	std::function<float()> min_width_data = [&terra]() { return terra.getMinX(); };
	std::cout << terra.getMaxX() << std::endl;
	std::function<float()> max_width_data = [&terra]() { return terra.getMaxX(); };
	std::function<float()> min_height_data = [&terra]() { return terra.getMinY();  };
	std::function<float()> max_height_data = [&terra]() { return terra.getMaxY();  };
	std::function<float()> min_depth_data = [&terra]() { return terra.getMinZ(); };
	std::function<float()> max_depth_data = [&terra]() { return terra.getMaxZ(); };
	std::function<glm::mat4()> light_proj_data = [&lightProjection](){ return lightProjection; };
	std::function<glm::mat4()> light_pv_data = [&lightPV](){ return lightPV; };
	std::function<glm::mat4()> light_view_data = [&lightView](){ return lightView; };
	std::function<glm::mat4()> identity_mat = [](){ return glm::mat4(1.0f); };
	std::function<glm::vec3()> cam_data = [&gui](){ return gui.getCamera(); };
	std::function<glm::vec4()> lp_data = [&light_position]() { return light_position; };
	std::function<glm::vec3()> lp_cam_data = [&light_position]() { return glm::vec3(light_position); };
	std::function<int()> ss_data = [&gui](){ return gui.ss_index; };

	auto std_model = std::make_shared<ShaderUniform<const glm::mat4*>>("model", model_data);
	auto floor_model = make_uniform("model", identity_mat);
	auto light_model = make_uniform("model", identity_mat);
	auto min_width = make_uniform("min_width", min_width_data);
	auto max_width = make_uniform("max_width", max_width_data);
	auto min_depth = make_uniform("min_depth", min_depth_data);
	auto max_depth = make_uniform("max_depth", max_depth_data);
	auto min_height = make_uniform("min_height", min_height_data);
	auto max_height = make_uniform("max_height", max_height_data);
	auto std_view = make_uniform("view", view_data);
	auto std_camera = make_uniform("camera_position", cam_data);
	auto std_proj = make_uniform("projection", proj_data);
	auto std_light = make_uniform("light_position", lp_data);
	auto ss_index_data = make_uniform("ss_index", ss_data);
	auto std_light_proj = make_uniform("projection", light_proj_data);
	auto std_light_view = make_uniform("view", light_view_data);
	auto std_light_pv = make_uniform("light_pv", light_pv_data);
	auto std_light_camera = make_uniform("camera_position", lp_cam_data);
	auto seeds_gl = make_uniform("seeds", seeds_data);
	auto random_angle_gl = make_uniform("random_angle", random_angle_data);
	auto ray00 = make_uniform("ray00", ray00_data);
	auto ray01 = make_uniform("ray01", ray01_data);
	auto ray10 = make_uniform("ray10", ray10_data);
	auto ray11 = make_uniform("ray11", ray11_data);
	auto time = make_uniform("time", time_data);

	std::function<float()> alpha_data = [&gui]() {
		static const float transparet = 0.5; // Alpha constant goes here
		static const float non_transparet = 1.0;
		if (gui.isTransparent())
			return transparet;
		else
			return non_transparet;
	};
	auto object_alpha = make_uniform("alpha", alpha_data);

	
	// FIXME: define more ShaderUniforms for RenderPass if you want to use it.
	//        Otherwise, do whatever you like here

	// Floor render pass
	RenderDataInput floor_pass_input;
	floor_pass_input.assign(0, "vertex_position", floor_vertices.data(), floor_vertices.size(), 4, GL_FLOAT);
	floor_pass_input.assignIndex(floor_faces.data(), floor_faces.size(), 3);
	RenderPass floor_pass(-1,
			floor_pass_input,
			{ vertex_shader, geometry_shader, floor_fragment_shader},
			{ floor_model, std_view, std_proj, std_light, std_light_pv },
			{ "fragment_color" }
			);


	std::vector<glm::vec2> quad_verts;
	std::vector<glm::uvec3> quad_indices;

	//create a quad that covers normalized device coord
	quad_verts.push_back(glm::vec2(-1.0, 1.0f));
	quad_verts.push_back(glm::vec2(-1.0f, -1.0f));
	quad_verts.push_back(glm::vec2(1.0f, -1.0f));
	quad_verts.push_back(glm::vec2(1.0f, 1.0f));
	quad_indices.push_back(glm::uvec3(0, 1, 2));
	quad_indices.push_back(glm::uvec3(0, 2, 3));

	//screen render pass
	RenderDataInput screen_pass_input;
	screen_pass_input.assign(0, "vertex", quad_verts.data(), quad_verts.size(), 2, GL_FLOAT);
	screen_pass_input.assignIndex(quad_indices.data(), quad_indices.size(), 3);
	RenderPass screen_pass(-1,
			screen_pass_input,
			{ quad_vs, NULL, quad_fs},
			{ std_light },
			{ "fragment_color" }
			);


	glUseProgram(screen_pass.sp_);
	glUniform1i(screen_pass.getUniformLocation("tex"),0);
	glUseProgram(0);

	//raytracer render pass
	RenderDataInput raytracer_pass_input;

	RenderPass raytracer_pass(-1,
			raytracer_pass_input,
			{ NULL, NULL, NULL, raytrace_cs},
			{ std_camera, ray00, ray01, ray10, ray11, time},
			{ }
			);

	raytracer_pass.getUniformLocation("cubeMaps");
	GLuint raytracerTexture = createNew2DTexture(GL_RGBA16F, GL_RGBA, view_width, view_height);
	GLint frameBufferBinding = raytracer_pass.getUniform(raytracer_pass.getUniformLocation("framebuffer"));
	GLint photonMapsBinding_rt = raytracer_pass.getUniform(raytracer_pass.getUniformLocation("photonMaps"));



	//photon mapping render pass
	RenderDataInput photon_pass_input;
	RenderPass photon_pass(-1,
			photon_pass_input,
			{ NULL, NULL, NULL, photon_cs},
			{ std_light, time, max_height, min_height, max_width, min_width, max_depth, min_depth},
			{  }
			);

	// photon_pass.addShader(photon_cs, GL_COMPUTE_SHADER);


	int workGroupSize[3];

	glUseProgram(photon_pass.sp_);
	CHECK_GL_ERROR(glGetProgramiv(photon_pass.sp_, GL_COMPUTE_WORK_GROUP_SIZE, workGroupSize));
	std::cout<< workGroupSize[0] << " " << workGroupSize[1] << " " << workGroupSize[2] <<std::endl;
	glUseProgram(0);


	//uniform allocated location for photon maps of cubes


	GLint loc = photon_pass.getUniformLocation("photonMaps");
	GLint photonMapsBinding = photon_pass.getUniform(loc);

	//photon mapping render pass
	RenderDataInput photon_scatter_pass_input;
	RenderPass photon_scatter_pass(-1,
			photon_scatter_pass_input,
			{ NULL, NULL, NULL, photon_scatter_cs},
			{ std_light, time},
			{  }
			);
	GLint photonMapsBinding_scatter = photon_scatter_pass.getUniform(photon_scatter_pass.getUniformLocation("photonMaps"));



	// //photon merge render pass
	// RenderDataInput photon_merge_pass_input;
	// RenderPass photon_merge_pass(-1,
	// 		photon_merge_pass_input,
	// 		{ NULL, NULL, NULL, photon_merge_cs},
	// 		{ },
	// 		{  }
	// 		);
	// GLint photonMapsBinding_merge = photon_merge_pass.getUniform(photon_merge_pass.getUniformLocation("photonMaps"));
	//Photonmap for the cubes
	GLuint photonMapTexture = createPhotonMapTexture();

	GLuint cubeSampler = createSampler();

	float aspect = 0.0f;
	bool need_update = false;
	bool draw_floor = true;

	int steps = 1000;
	int worksizeX = nextPowerOfTwo(1024);
    int worksizeY = nextPowerOfTwo(1024);
	// while(steps)
	// {
	// 	glfwGetFramebufferSize(window, &window_width, &window_height);
	
	// 	glViewport(0, 0, view_width, view_height);
	// 	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	// 	glEnable(GL_DEPTH_TEST);
	// 	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// 	glDepthFunc(GL_LESS);
	// 	glCullFace(GL_BACK);
		
	// 	photon_pass.setup();
	// 	CHECK_GL_ERROR(glBindImageTexture(photonMapsBinding, photonMapTexture, 0, true, 0, GL_READ_WRITE, GL_RG16F));

 //        CHECK_GL_ERROR(glDispatchCompute(worksizeX / workGroupSize[0], worksizeY / workGroupSize[1], 1));
 //        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
 //        glBindImageTexture(photonMapsBinding, 0, 0, true, 0, GL_READ_WRITE, GL_RG16F);
 //        --steps;
	// }
    std::vector<GLubyte> emptyData(view_width * view_width * 4, 0);
    int i = 0;
	while (!glfwWindowShouldClose(window)) {
		//cube_pass.updateVBO(0,cube_vertices.data(), cube_vertices.size());
		
		// Setup some basic window stuff.
		glfwGetFramebufferSize(window, &window_width, &window_height);
	
		glViewport(0, 0, view_width, view_height);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glEnable(GL_DEPTH_TEST);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDepthFunc(GL_LESS);
		glCullFace(GL_BACK);
		
		// if(gui.isDirty())
		// {
		// 	glBindTexture(GL_TEXTURE_2D, raytracerTexture);
		// 	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, view_width, view_height, GL_BGRA, GL_UNSIGNED_BYTE, &emptyData[0]);
		// 	glBindTexture(GL_TEXTURE_2D, 0);
		// }
		
		gui.updateMatrices(false);
		mats = gui.getMatrixPointers();


		//std::cout<< gui.getCamera() << std::endl;
		

		
		//if(i < 10000)
		//{

			photon_pass.setup();
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, terrainSSBO);
			CHECK_GL_ERROR(glBindImageTexture(photonMapsBinding, photonMapTexture, 0, true, 0, GL_READ_WRITE, GL_RGBA16F));
	        CHECK_GL_ERROR(glDispatchCompute(worksizeX / workGroupSize[0], worksizeY / workGroupSize[1], 1));
	        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	        glBindImageTexture(photonMapsBinding, 0, 0, true, 0, GL_READ_WRITE, GL_RGBA16F);
	        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);
	  //  }



        if(i > 100)
        {
	        photon_scatter_pass.setup();
	        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, terrainSSBO);

			CHECK_GL_ERROR(glBindImageTexture(photonMapsBinding_scatter, photonMapTexture, 0, true, 0, GL_READ_WRITE, GL_RGBA16F));

	        CHECK_GL_ERROR(glDispatchCompute(worksizeX / workGroupSize[0], worksizeY / workGroupSize[1], 1));
	        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	        glBindImageTexture(photonMapsBinding_scatter, 0, 0, true, 0, GL_READ_WRITE, GL_RGBA16F);
	        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);
	    }


        ++i;
        //first pass, render raytracing texture
		raytracer_pass.setup();
		CHECK_GL_ERROR(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, terrainSSBO));
		CHECK_GL_ERROR(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, bvhSSBO));
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        CHECK_GL_ERROR(glBindImageTexture(photonMapsBinding_rt, photonMapTexture, 0, true, 0, GL_READ_WRITE, GL_RGBA16F));

        glBindImageTexture(frameBufferBinding, raytracerTexture, 0, false, 0, GL_WRITE_ONLY, GL_RGBA16F);
        glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, photonMapTexture);
        glBindSampler(0, cubeSampler);
		CHECK_GL_ERROR(glDispatchCompute(nextPowerOfTwo(view_width) / workGroupSize[0], nextPowerOfTwo(view_height) / workGroupSize[1], 1));
		glBindSampler(0, 0);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, 0);
        glBindImageTexture(0, 0, 0, false, 0, GL_READ_WRITE, GL_RGBA16F);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);
        CHECK_GL_ERROR(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0));
        screen_pass.setup();
		glBindTexture(GL_TEXTURE_2D,raytracerTexture);
		CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES,
                              quad_indices.size() * 3,
                              GL_UNSIGNED_INT, 0));
		glBindTexture(GL_TEXTURE_2D,0);

// 		if(terra.updateCube(gui.getCamera()[0], gui.getCamera()[2]) || need_update)
// 		{
// 			terra.terrainMatrixToVector(cube_vertices, cube_faces, gui, false);
// 			terra.createWaterPlane(water_vertices, water_faces,gui);
// 			cube_pass.updateVBO(0, cube_vertices.data(), cube_vertices.size());
// 			water_pass.updateVBO(0, water_vertices.data(), water_vertices.size());
// 			need_update = false;
// 		}

// #if 0
// 		std::cerr << model_data() << '\n';
// 		std::cerr << "call from outside: " << std_model->data_source() << "\n";
// 		std_model->bind(0);
// #endif

// 		//Then draw floor.
// 		// if (draw_floor) {
// 		// 	floor_pass.setup();
// 		// 	// Draw our triangles.
// 		// 	CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES,
// 		// 	                              floor_faces.size() * 3,
// 		// 	                              GL_UNSIGNED_INT, 0));
// 		// 	glBindTexture(GL_TEXTURE_2D, 0);
// 		// }

// 		// Then draw floor.
// 		if (draw_floor) {
// 			cube_pass.setup();
// 			// Draw our triangles.
// 			CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES,
// 			                              cube_vertices.size() / VERTICES_PER_CUBE * 13 * 3,
// 			                              GL_UNSIGNED_INT, 0));
		
// 			water_pass.setup();
// 			glEnable(GL_BLEND);
// 			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
// 			// Draw our triangles.
// 			CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES,
// 										water_vertices.size() / VERTICES_PER_CUBE * 13 * 3 
// 			                              ,
// 			                              GL_UNSIGNED_INT, 0));
// 			glDisable(GL_BLEND);
// 		}


		// Poll and swap.
		glfwPollEvents();
		glfwSwapBuffers(window);
	}
	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}
