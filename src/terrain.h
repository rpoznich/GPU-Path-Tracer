#ifndef TERRAIN_H
#define TERRAIN_H
#include "config.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <deque>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/io.hpp>
#include <debuggl.h>
#include <random>

class GUI;


const float STANDARD_TERRAIN_HEIGHT = 0.01f;
const float patch_size = 5.0f;


class Terrain
{
	const float  BOX_SIZE;
	const size_t num_x;
	const size_t num_z;
	const float GROUND;
	const float kFloorXRender;
	const float kFloorZRender;
	std::deque<std::deque<glm::vec4>> cube_matrix;
	std::vector<glm::vec3> gradients;
	std::vector<glm::vec3> alpha_beta_type;
	std::vector<int> seeds;
	float _eye_x;
	float _eye_z;
	float min_height;
	float max_height;
	std::vector<float> random_angle; //random from 0 to 2 pi
	std::vector<glm::vec2> random_alpha_beta; //random alpha beta
	template<typename T>
	void _CreateCube(T& vertices, float start_x, float start_y, float start_z) const;

	template<typename T,typename T2>
	void CreateCube(T& vertices, T2&indices, float start_x, float start_y, float start_z) const;


	glm::vec2 getGradientVector(float patch_x, float patch_y, float patch_z) const;
	float getNoise(float patch_size,float cube_x, float cube_y, float cube_z) const;
	const glm::vec2& getAlphaBeta(float patch_x, float patch_z) const;
	void displaceCube();

	public:
		Terrain(float box_size, size_t _num_x, size_t _num_z, float ground) 
			: BOX_SIZE(box_size), num_x(_num_x), num_z(_num_z),
              GROUND(ground), kFloorXRender(BOX_SIZE * num_x / 2),
              kFloorZRender(BOX_SIZE * num_z / 2), seeds(), random_angle(), random_alpha_beta(){

            seeds.resize(512);
			for(size_t i = 0; i < 512; ++i)
				seeds[i] = i % 256;
			shuffle(seeds.begin(), seeds.end(), std::default_random_engine());

			random_angle.resize(256);
			for(size_t i = 0; i < 256; ++i){
				std::default_random_engine generator(i);
		  		std::uniform_real_distribution<> distribution(0, 2 * M_PI);
				random_angle[i] =  (float)distribution(generator);
			}
			random_alpha_beta.resize(256);
			for(size_t i = 0; i < 256; ++i){
				std::default_random_engine generator(i);
		  		std::uniform_real_distribution<> distribution(0, 0.2);
				random_alpha_beta[i] =  glm::vec2((float)distribution(generator),(float)distribution(generator));
			}


        }

	
		void InitializeTerrain(float eye_x, float eye_z);

		const std::vector<int> getSeeds()
		{
			return seeds;
		}

		const std::vector<float> getRandomAngles()
		{
			return random_angle;
		}

		const std::vector<glm::vec2> getRandomAlphaBeta()
		{
			return random_alpha_beta;
		}

		bool updateCube(float eye_x, float eye_z);

		void createWaterPlane(std::vector<glm::vec4>& water_vertices, std::vector<glm::uvec3>& water_indices, const GUI& gui) const;


		void terrainMatrixToVector(std::vector<glm::vec4>& vertices,  std::vector<glm::uvec3>& indices, const GUI& gui,
																								bool gen_index, bool culling) const;

		size_t get_box_indices(const glm::vec3& pos) const;

		float getMinX() const
		{
			return BOX_SIZE * floor((_eye_x-kFloorXRender) /BOX_SIZE) - BOX_SIZE;

		}

		float getMinZ() const
		{
			return BOX_SIZE * floor((_eye_z-kFloorZRender) /BOX_SIZE) - BOX_SIZE;
		}

		float getMaxX() const
		{
			return  getMinX() + kFloorXRender * BOX_SIZE * (num_x + 1);
		}

		float getMaxZ() const
		{
			return getMinZ() + kFloorXRender * BOX_SIZE * (num_z + 1);
		}

		float getMaxY() const
		{
			return max_height;
		}
		float getMinY() const
		{
			return min_height;
		}

		template<typename T>
		bool is_colliding(const T& boxes, const glm::vec3& pos, float entity_height) const;
		bool is_valid(glm::vec3& proj_pos, float entity_height, float entity_width) const;
		float get_height(const glm::vec3& pos);
		float get_height(size_t index) const;

};

#endif