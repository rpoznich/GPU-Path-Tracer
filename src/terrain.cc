#include "terrain.h"
#include <limits>
#include "gui.h"
#include <unordered_set>
#include <math.h>
glm::vec2 Terrain::getGradientVector(float patch_x, float patch_y, float patch_z) const
{
	assert(seeds.size() == 512 && random_angle.size() == 256);
	int x = int(patch_x) % 256;
	int y = int(patch_y) % 256;

	int z = int(patch_z) % 256;
	x = x < 0 ? 256 - abs(x) : x;
	y = y < 0 ? 256 - abs(y) : y;
	z = z < 0 ? 256 - abs(z) : z;
	float theta = random_angle[seeds[x+seeds[y+seeds[z]]]];
	return glm::normalize(glm::vec2(sin(theta),cos(theta)));
}

const glm::vec2& Terrain::getAlphaBeta(float patch_x, float patch_z) const
{
	assert(seeds.size() == 512 && random_angle.size() == 256);

	int x = int(patch_x) % 256;
	int z = int(patch_z) % 256;
	x = x < 0 ? 256 - abs(x) : x;
	z = z < 0 ? 256 - abs(z) : z;
	const glm::vec2& r = random_alpha_beta[seeds[x+seeds[z]]];
	return r;
}


float Terrain::getNoise(float patch_size,float cube_x, float cube_y, float cube_z) const 
{
	float patch_start_x = patch_size * floor(cube_x / patch_size);
	float patch_start_z = patch_size * floor(cube_z / patch_size);
	const glm::vec2& alpha_beta = getAlphaBeta(patch_start_x, patch_start_z);
	float _u = (cube_x - patch_start_x)/patch_size;
	float _v = (cube_z - patch_start_z)/patch_size;
	float u = alpha_beta[1]*((cube_x - patch_start_x)/patch_size);
	float v = alpha_beta[1]*((cube_z - patch_start_z)/patch_size);
	glm::vec2 d = getGradientVector(patch_start_x, cube_y, patch_start_z);
	glm::vec2 c =  getGradientVector(patch_start_x + patch_size, cube_y, patch_start_z);
	glm::vec2 b = getGradientVector(patch_start_x + patch_size, cube_y, patch_start_z + patch_size);
	glm::vec2 a = getGradientVector(patch_start_x, cube_y, patch_start_z + patch_size);
	glm::vec2 _c = glm::normalize(glm::vec2(u,v)- glm::vec2(1,0));
	glm::vec2 _b = glm::normalize(glm::vec2(u,v)- glm::vec2(1,1));
	glm::vec2 _d = glm::normalize(glm::vec2(u,v)- glm::vec2(0,0));
	glm::vec2 _a = glm::normalize(glm::vec2(u,v)- glm::vec2(0,1));
	float noise =  alpha_beta[0]*((1-v)*((1-u)*glm::dot(a, _a)+ u * glm::dot(b,_b)) + v * ((1-u)*glm::dot(d,_d)+ u * glm::dot(c,_c)));
	return noise;
}

template<typename T>
void Terrain::_CreateCube(T& vertices, float start_x, float start_y, float start_z) const
{
	int start_size = vertices.size();
	vertices.push_back(glm::vec4(start_x ,  start_y, start_z, 1.0f));
	vertices.push_back(glm::vec4(start_x,  start_y + BOX_SIZE, start_z, 1.0f));
	vertices.push_back(glm::vec4(start_x + BOX_SIZE, start_y + BOX_SIZE, start_z, 1.0f));
	vertices.push_back(glm::vec4(start_x + BOX_SIZE, start_y, start_z, 1.0f));
	vertices.push_back(glm::vec4(start_x,  start_y, start_z + BOX_SIZE, 1.0f));
	vertices.push_back(glm::vec4(start_x, start_y + BOX_SIZE, start_z + BOX_SIZE, 1.0f));
	vertices.push_back(glm::vec4(start_x + BOX_SIZE, start_y + BOX_SIZE, start_z + BOX_SIZE, 1.0f));
	vertices.push_back(glm::vec4(start_x + BOX_SIZE, start_y, start_z + BOX_SIZE, 1.0f));

}

template<typename T,typename T2>
void Terrain::CreateCube(T& vertices, T2&indices, float start_x, float start_y, float start_z) const
{
	int start_size = vertices.size();
	vertices.push_back(glm::vec4(start_x ,  start_y, start_z, 1.0f));
	vertices.push_back(glm::vec4(start_x,  start_y + BOX_SIZE, start_z, 1.0f));
	vertices.push_back(glm::vec4(start_x + BOX_SIZE, start_y + BOX_SIZE, start_z, 1.0f));
	vertices.push_back(glm::vec4(start_x + BOX_SIZE, start_y, start_z, 1.0f));
	vertices.push_back(glm::vec4(start_x,  start_y, start_z + BOX_SIZE, 1.0f));
	vertices.push_back(glm::vec4(start_x, start_y + BOX_SIZE, start_z + BOX_SIZE, 1.0f));
	vertices.push_back(glm::vec4(start_x + BOX_SIZE, start_y + BOX_SIZE, start_z + BOX_SIZE, 1.0f));
	vertices.push_back(glm::vec4(start_x + BOX_SIZE, start_y, start_z + BOX_SIZE, 1.0f));
	indices.push_back(glm::uvec3(start_size, start_size+1, start_size+2));
	indices.push_back(glm::uvec3(start_size, start_size+2, start_size+3));

	indices.push_back(glm::uvec3(start_size+6, start_size+ 5, start_size+4));
	indices.push_back(glm::uvec3(start_size+7, start_size+6, start_size+4));

	indices.push_back(glm::uvec3(start_size+3, start_size+2, start_size+6));
	indices.push_back(glm::uvec3(start_size+3, start_size+6, start_size+7));

	indices.push_back(glm::uvec3(start_size+4, start_size+5, start_size+1));
	indices.push_back(glm::uvec3(start_size+4, start_size+1, start_size));

	indices.push_back(glm::uvec3(start_size+1, start_size+5, start_size+2));
	indices.push_back(glm::uvec3(start_size+5, start_size+6, start_size+2));

	indices.push_back(glm::uvec3(start_size+4, start_size+0, start_size+3));
	indices.push_back(glm::uvec3(start_size+4, start_size+3, start_size+7));
}

void Terrain::InitializeTerrain(float eye_x, float eye_z){
	cube_matrix.clear();

	float min_x = BOX_SIZE * floor((eye_x-kFloorXRender) /BOX_SIZE);
	float min_z = BOX_SIZE * floor((eye_z-kFloorZRender) /BOX_SIZE);

	for(size_t r = 0;  r < num_z; ++r)
	{
		cube_matrix.push_back(std::deque<glm::vec4>());
		for(size_t c = 0;  c < num_x; ++c)
		{
			float start_x = min_x + BOX_SIZE * c;
			float start_z = min_z  + BOX_SIZE * r;
			_CreateCube(cube_matrix[r],  start_x,  GROUND,  start_z);
		}
	}
	displaceCube();
}


bool Terrain::updateCube(float eye_x, float eye_z)
{

	float d_x =   (eye_x - cube_matrix[0][0].x) - kFloorXRender;
	float d_z = (eye_z - cube_matrix[0][0].z)- kFloorZRender;
	if(abs(d_x) >= BOX_SIZE || abs(d_z) >= BOX_SIZE)
	{
		
		for(size_t r = 0;  r < num_z; ++r)
		{
			if(d_x < 0 && abs(d_x) >= BOX_SIZE )
			{
				std::reverse(cube_matrix[r].begin(),cube_matrix[r].end());
			}
		} //create an easy access mat representation of the vertices
		if(d_z <  0 && abs(d_z) >= BOX_SIZE)
			std::reverse(cube_matrix.begin(),cube_matrix.end());

		if(abs(d_x) >= BOX_SIZE)
		{
			int diff_boxes = floor(abs(d_x / BOX_SIZE));
			int j = 0;
			while(j < diff_boxes)
			{
				for(size_t r = 0;  r < num_z; ++r)
				{
					size_t i = 0;
					while(i < 8)
					{
						cube_matrix[r].erase(cube_matrix[r].begin()+0);
						++i;
					}
					float incr = BOX_SIZE;
					if(d_x < 0)
						incr = -BOX_SIZE;
					float start_x = cube_matrix[r][cube_matrix[r].size()-8].x;
					float start_z = cube_matrix[r][cube_matrix[r].size()-8].z;

					if(d_x < 0){
						start_z = cube_matrix[r][cube_matrix[r].size()-1].z;
						start_x = cube_matrix[r][cube_matrix[r].size()-1].x;
					}
					start_x += incr;
					std::deque<glm::vec4> appending;

					_CreateCube(appending, start_x  , GROUND, start_z);
					if(d_x < 0)
						std::reverse(appending.begin(),appending.end());
					cube_matrix[r].insert( cube_matrix[r].end(),  appending.begin(),appending.end()); // preallocate memory

					float avg_height_neighbors = 0;
					float min_height_neighbors = std::numeric_limits<float>::max();
					float max_height_neighbors = -std::numeric_limits<float>::max();

					int num_neighbors = 0;
					size_t col = cube_matrix[r].size() / 8 - 1;
					if(r >= 1)
					{
						avg_height_neighbors += cube_matrix[r-1][col*VERTICES_PER_CUBE].y - (GROUND);
						min_height_neighbors = cube_matrix[r-1][col*VERTICES_PER_CUBE].y - (GROUND) < min_height_neighbors ? 
										cube_matrix[r-1][col*VERTICES_PER_CUBE].y - (GROUND) : min_height_neighbors;
						max_height_neighbors = cube_matrix[r-1][col*VERTICES_PER_CUBE].y - (GROUND) > max_height_neighbors ? 
										cube_matrix[r-1][col*VERTICES_PER_CUBE].y - (GROUND) : max_height_neighbors;
						++num_neighbors;
						if( col >= 1)
						{
							avg_height_neighbors += cube_matrix[r-1][(col-1)*VERTICES_PER_CUBE].y - (GROUND);
							min_height_neighbors = cube_matrix[r-1][(col-1)*VERTICES_PER_CUBE].y - (GROUND) < min_height_neighbors ? 
										cube_matrix[r-1][(col-1)*VERTICES_PER_CUBE].y - (GROUND) : min_height_neighbors;
							max_height_neighbors = cube_matrix[r-1][(col-1)*VERTICES_PER_CUBE].y - (GROUND) > max_height_neighbors ? 
										cube_matrix[r-1][(col-1)*VERTICES_PER_CUBE].y - (GROUND) : max_height_neighbors;
							++num_neighbors;
						}
						if(col < num_x - 1)
						{
							avg_height_neighbors += cube_matrix[r-1][(col+1)*VERTICES_PER_CUBE].y - (GROUND);
							min_height_neighbors = cube_matrix[r-1][(col+1)*VERTICES_PER_CUBE].y - (GROUND) < min_height_neighbors ? 
										cube_matrix[r-1][(col+1)*VERTICES_PER_CUBE].y - (GROUND) : min_height_neighbors;

							max_height_neighbors = cube_matrix[r-1][(col+1)*VERTICES_PER_CUBE].y - (GROUND) > max_height_neighbors ? 
										cube_matrix[r-1][(col+1)*VERTICES_PER_CUBE].y - (GROUND) : max_height_neighbors;
							++num_neighbors;
						}

					}
					if(col >= 1)
					{
						avg_height_neighbors += cube_matrix[r][(col-1)*VERTICES_PER_CUBE].y - (GROUND);
						min_height_neighbors = cube_matrix[r][(col-1)*VERTICES_PER_CUBE].y - (GROUND) < min_height_neighbors ? 
										cube_matrix[r][(col-1)*VERTICES_PER_CUBE].y - (GROUND) : min_height_neighbors;

						max_height_neighbors = cube_matrix[r][(col-1)*VERTICES_PER_CUBE].y - (GROUND) > max_height_neighbors ? 
										cube_matrix[r][(col-1)*VERTICES_PER_CUBE].y - (GROUND) : max_height_neighbors;
						++num_neighbors;
					}
					min_height_neighbors = min_height_neighbors == std::numeric_limits<float>::max() ? 0 : min_height_neighbors;
					max_height_neighbors =  max_height_neighbors == -std::numeric_limits<float>::max() ? 0 : max_height_neighbors;
					avg_height_neighbors = (min_height_neighbors + max_height_neighbors) / 2.0;
					avg_height_neighbors = avg_height_neighbors == 0 ? STANDARD_TERRAIN_HEIGHT : avg_height_neighbors;
					float noise =  getNoise(patch_size,cube_matrix[r][col* VERTICES_PER_CUBE].x, GROUND, cube_matrix[r][col* VERTICES_PER_CUBE].z);
					cube_matrix[r][col* VERTICES_PER_CUBE].y += std::max(max_height_neighbors-BOX_SIZE,std::min(min_height_neighbors+BOX_SIZE,std::max(minTerrainHeight,std::min(maxTerrainHeight,avg_height_neighbors * (1 + noise)))));
					cube_matrix[r][col* VERTICES_PER_CUBE + 1].y += std::max(max_height_neighbors-BOX_SIZE,std::min(min_height_neighbors+BOX_SIZE,std::max(minTerrainHeight,std::min(maxTerrainHeight,avg_height_neighbors * (1 + noise)))));
					cube_matrix[r][col* VERTICES_PER_CUBE + 2].y += std::max(max_height_neighbors-BOX_SIZE,std::min(min_height_neighbors+BOX_SIZE,std::max(minTerrainHeight,std::min(maxTerrainHeight,avg_height_neighbors * (1 + noise)))));
					cube_matrix[r][col* VERTICES_PER_CUBE + 3].y += std::max(max_height_neighbors-BOX_SIZE,std::min(min_height_neighbors+BOX_SIZE,std::max(minTerrainHeight,std::min(maxTerrainHeight,avg_height_neighbors * (1 + noise)))));
					cube_matrix[r][col* VERTICES_PER_CUBE + 4].y += std::max(max_height_neighbors-BOX_SIZE,std::min(min_height_neighbors+BOX_SIZE,std::max(minTerrainHeight,std::min(maxTerrainHeight,avg_height_neighbors * (1 + noise)))));
					cube_matrix[r][col* VERTICES_PER_CUBE + 5].y += std::max(max_height_neighbors-BOX_SIZE,std::min(min_height_neighbors+BOX_SIZE,std::max(minTerrainHeight,std::min(maxTerrainHeight,avg_height_neighbors * (1 + noise)))));
					cube_matrix[r][col* VERTICES_PER_CUBE + 6].y += std::max(max_height_neighbors-BOX_SIZE,std::min(min_height_neighbors+BOX_SIZE,std::max(minTerrainHeight,std::min(maxTerrainHeight,avg_height_neighbors * (1 + noise)))));
					cube_matrix[r][col* VERTICES_PER_CUBE + 7].y += std::max(max_height_neighbors-BOX_SIZE,std::min(min_height_neighbors+BOX_SIZE,std::max(minTerrainHeight,std::min(maxTerrainHeight,avg_height_neighbors * (1 + noise)))));
				}
				++j;
			}
		}

		//reverse back
		if(d_x < 0 && abs(d_x) >= BOX_SIZE )
		{
			for(size_t r = 0;  r < num_z; ++r)
			{
				std::reverse(cube_matrix[r].begin(),cube_matrix[r].end());
			}

		}

		if(abs(d_z) >= BOX_SIZE)
		{
			int diff_boxes = floor(abs(d_z / BOX_SIZE));
			int j = 0;
			while(diff_boxes)
			{
				cube_matrix.erase(cube_matrix.begin()+0);
				cube_matrix.push_back(std::deque<glm::vec4>()); //add one more row
				size_t r = cube_matrix.size() - 1; //new added row
				for(size_t col = 0;  col < num_x; ++col)
				{
					float incr = BOX_SIZE;
					if(d_z < 0)
						incr = -BOX_SIZE;
					float start_x = cube_matrix[r-1][col * VERTICES_PER_CUBE].x;
					float start_z = cube_matrix[r-1][col * VERTICES_PER_CUBE].z + incr;

					_CreateCube(cube_matrix[r], start_x  , GROUND, start_z);

					float avg_height_neighbors = 0;
					int num_neighbors = 0;
					float min_height_neighbors = std::numeric_limits<float>::max();
					float max_height_neighbors = -std::numeric_limits<float>::max();

					if(r >= 1)
					{
						avg_height_neighbors += cube_matrix[r-1][col*VERTICES_PER_CUBE].y - (GROUND);
						min_height_neighbors = cube_matrix[r-1][col*VERTICES_PER_CUBE].y - (GROUND) < min_height_neighbors ? 
										cube_matrix[r-1][col*VERTICES_PER_CUBE].y - (GROUND) : min_height_neighbors;
						max_height_neighbors = cube_matrix[r-1][col*VERTICES_PER_CUBE].y - (GROUND) > max_height_neighbors ? 
										cube_matrix[r-1][col*VERTICES_PER_CUBE].y - (GROUND) : max_height_neighbors;
						++num_neighbors;
						if( col >= 1)
						{
							avg_height_neighbors += cube_matrix[r-1][(col-1)*VERTICES_PER_CUBE].y - (GROUND);
							min_height_neighbors = cube_matrix[r-1][(col-1)*VERTICES_PER_CUBE].y - (GROUND) < min_height_neighbors ? 
										cube_matrix[r-1][(col-1)*VERTICES_PER_CUBE].y - (GROUND) : min_height_neighbors;
							max_height_neighbors = cube_matrix[r-1][(col-1)*VERTICES_PER_CUBE].y - (GROUND) > max_height_neighbors ? 
										cube_matrix[r-1][(col-1)*VERTICES_PER_CUBE].y - (GROUND) : max_height_neighbors;
							++num_neighbors;
						}
						if(col < num_x - 1)
						{
							avg_height_neighbors += cube_matrix[r-1][(col+1)*VERTICES_PER_CUBE].y - (GROUND);
							min_height_neighbors = cube_matrix[r-1][(col+1)*VERTICES_PER_CUBE].y - (GROUND) < min_height_neighbors ? 
										cube_matrix[r-1][(col+1)*VERTICES_PER_CUBE].y - (GROUND) : min_height_neighbors;

							max_height_neighbors = cube_matrix[r-1][(col+1)*VERTICES_PER_CUBE].y - (GROUND) > max_height_neighbors ? 
										cube_matrix[r-1][(col+1)*VERTICES_PER_CUBE].y - (GROUND) : max_height_neighbors;
							++num_neighbors;
						}

					}
					if(col >= 1)
					{
						avg_height_neighbors += cube_matrix[r][(col-1)*VERTICES_PER_CUBE].y - (GROUND);
						min_height_neighbors = cube_matrix[r][(col-1)*VERTICES_PER_CUBE].y - (GROUND) < min_height_neighbors ? 
										cube_matrix[r][(col-1)*VERTICES_PER_CUBE].y - (GROUND) : min_height_neighbors;

						max_height_neighbors = cube_matrix[r][(col-1)*VERTICES_PER_CUBE].y - (GROUND) > max_height_neighbors ? 
										cube_matrix[r][(col-1)*VERTICES_PER_CUBE].y - (GROUND) : max_height_neighbors;
						++num_neighbors;
					}
					min_height_neighbors = min_height_neighbors == std::numeric_limits<float>::max() ? 0 : min_height_neighbors;
					max_height_neighbors =  max_height_neighbors == -std::numeric_limits<float>::max() ? 0 : max_height_neighbors;
					avg_height_neighbors = (min_height_neighbors + max_height_neighbors) / 2.0;
					avg_height_neighbors = avg_height_neighbors == 0 ? STANDARD_TERRAIN_HEIGHT : avg_height_neighbors;

					float noise =  getNoise(patch_size,cube_matrix[r][col* VERTICES_PER_CUBE].x, GROUND, cube_matrix[r][col* VERTICES_PER_CUBE].z);
					cube_matrix[r][col* VERTICES_PER_CUBE].y += std::max(max_height_neighbors-BOX_SIZE,std::min(min_height_neighbors+BOX_SIZE,std::max(minTerrainHeight,std::min(maxTerrainHeight,avg_height_neighbors * (1 + noise)))));
					cube_matrix[r][col* VERTICES_PER_CUBE + 1].y += std::max(max_height_neighbors-BOX_SIZE,std::min(min_height_neighbors+BOX_SIZE,std::max(minTerrainHeight,std::min(maxTerrainHeight,avg_height_neighbors * (1 + noise)))));
					cube_matrix[r][col* VERTICES_PER_CUBE + 2].y += std::max(max_height_neighbors-BOX_SIZE,std::min(min_height_neighbors+BOX_SIZE,std::max(minTerrainHeight,std::min(maxTerrainHeight,avg_height_neighbors * (1 + noise)))));
					cube_matrix[r][col* VERTICES_PER_CUBE + 3].y += std::max(max_height_neighbors-BOX_SIZE,std::min(min_height_neighbors+BOX_SIZE,std::max(minTerrainHeight,std::min(maxTerrainHeight,avg_height_neighbors * (1 + noise)))));
					cube_matrix[r][col* VERTICES_PER_CUBE + 4].y += std::max(max_height_neighbors-BOX_SIZE,std::min(min_height_neighbors+BOX_SIZE,std::max(minTerrainHeight,std::min(maxTerrainHeight,avg_height_neighbors * (1 + noise)))));
					cube_matrix[r][col* VERTICES_PER_CUBE + 5].y += std::max(max_height_neighbors-BOX_SIZE,std::min(min_height_neighbors+BOX_SIZE,std::max(minTerrainHeight,std::min(maxTerrainHeight,avg_height_neighbors * (1 + noise)))));
					cube_matrix[r][col* VERTICES_PER_CUBE + 6].y += std::max(max_height_neighbors-BOX_SIZE,std::min(min_height_neighbors+BOX_SIZE,std::max(minTerrainHeight,std::min(maxTerrainHeight,avg_height_neighbors * (1 + noise)))));
					cube_matrix[r][col* VERTICES_PER_CUBE + 7].y += std::max(max_height_neighbors-BOX_SIZE,std::min(min_height_neighbors+BOX_SIZE,std::max(minTerrainHeight,std::min(maxTerrainHeight,avg_height_neighbors * (1 + noise)))));
				}
				--diff_boxes;
			}
		}
		//reverse back
		if(d_z <  0 && abs(d_z) >= BOX_SIZE)
			std::reverse(cube_matrix.begin(),cube_matrix.end());

		return true;

	}
	return false;
}

void Terrain::createWaterPlane(std::vector<glm::vec4>& water_vertices, std::vector<glm::uvec3>& water_indices, const GUI& gui) const
{
	assert(cube_matrix.size() == num_z && cube_matrix[0].size() == num_x * VERTICES_PER_CUBE);

	water_vertices.clear();
	water_vertices.reserve(num_x * num_z * VERTICES_PER_CUBE);
	for(size_t r = 0; r <  num_z; ++r)
	{
		for(size_t c = 0; c < num_x; ++c)
		{
			if(cube_matrix[r][c * VERTICES_PER_CUBE].y < GROUND +  BOX_SIZE * 3)
			{
				/*fixme : add depth checking stuff here */
				bool check = false;
				for(size_t _ = 0; _ < 8 && !check; ++_)
				{
					glm::vec3 dir = glm::normalize(glm::vec3(cube_matrix[r][c * VERTICES_PER_CUBE + _]) - gui.getCamera());
					check = gui.inViewFrustum(dir); 
				}
				if(check)
				{
					auto b = cube_matrix[r].begin() + c * VERTICES_PER_CUBE;
					auto e = b + 8;
					size_t start_size = water_vertices.size();
					water_vertices.insert(water_vertices.end(), b, e);
					float elev = (GROUND + BOX_SIZE * 3 - cube_matrix[r][c * VERTICES_PER_CUBE].y);
					for(size_t _ = 0; _ < 8; ++_){
						water_vertices[water_vertices.size() - (_+1)].y += elev;
					}
				}

			}
		}
	}
}


void Terrain::displaceCube()
{
	for(size_t r = 0; r < num_z ; ++r)
	{
		for(size_t col = 0; col < num_x; ++col)
		{
			float avg_height_neighbors = 0;
			float min_height_neighbors = std::numeric_limits<float>::max();
			float max_height_neighbors = -std::numeric_limits<float>::max();
			int num_neighbors = 0;
			if(r >= 1)
			{
				avg_height_neighbors += cube_matrix[r-1][ col* VERTICES_PER_CUBE][1] - (GROUND);
				min_height_neighbors = cube_matrix[r-1][ col* VERTICES_PER_CUBE][1] - (GROUND) < min_height_neighbors ? 
										cube_matrix[r-1][ col* VERTICES_PER_CUBE][1] - (GROUND) : min_height_neighbors;
				max_height_neighbors = cube_matrix[r-1][ col* VERTICES_PER_CUBE][1] - (GROUND) > max_height_neighbors ? 
										cube_matrix[r-1][ col* VERTICES_PER_CUBE][1] - (GROUND) : max_height_neighbors;
				++num_neighbors;
				if( col >= 1)
				{
					avg_height_neighbors += cube_matrix[r-1][(col-1) * VERTICES_PER_CUBE][1] - (GROUND);
					min_height_neighbors = cube_matrix[r-1][(col-1) * VERTICES_PER_CUBE][1] - (GROUND) < min_height_neighbors ? 
										cube_matrix[r-1][(col-1) * VERTICES_PER_CUBE][1] - (GROUND) : min_height_neighbors;
					max_height_neighbors = cube_matrix[r-1][(col-1) * VERTICES_PER_CUBE][1] - (GROUND) > max_height_neighbors ? 
										cube_matrix[r-1][(col-1) * VERTICES_PER_CUBE][1] - (GROUND) : max_height_neighbors;
					++num_neighbors;
				}
				if(col < num_x - 1)
				{
					avg_height_neighbors += cube_matrix[r-1][(col+1) * VERTICES_PER_CUBE][1] - (GROUND);
					min_height_neighbors =  cube_matrix[r-1][(col+1) * VERTICES_PER_CUBE][1] - (GROUND) < min_height_neighbors ? 
										 cube_matrix[r-1][(col+1) * VERTICES_PER_CUBE][1] - (GROUND) : min_height_neighbors;

					max_height_neighbors = cube_matrix[r-1][(col+1) * VERTICES_PER_CUBE][1] - (GROUND) > max_height_neighbors ? 
										cube_matrix[r-1][(col+1) * VERTICES_PER_CUBE][1] - (GROUND) : max_height_neighbors;
					++num_neighbors;
				}
			}
			if(col >= 1)
			{
				avg_height_neighbors += cube_matrix[r][(col-1) * VERTICES_PER_CUBE][1] - (GROUND);
				min_height_neighbors =  cube_matrix[r][(col-1) * VERTICES_PER_CUBE][1] - (GROUND) < min_height_neighbors ? 
										 cube_matrix[r][(col-1) * VERTICES_PER_CUBE][1] - (GROUND) : min_height_neighbors;

				max_height_neighbors = cube_matrix[r][(col-1) * VERTICES_PER_CUBE][1] - (GROUND) > max_height_neighbors ? 
										cube_matrix[r][(col-1) * VERTICES_PER_CUBE][1] - (GROUND) : max_height_neighbors;
				++num_neighbors;
			}
			min_height_neighbors = min_height_neighbors == std::numeric_limits<float>::max() ? 0 : min_height_neighbors;
			max_height_neighbors =  max_height_neighbors == -std::numeric_limits<float>::max() ? 0 : max_height_neighbors;
			avg_height_neighbors = (min_height_neighbors + max_height_neighbors) / 2.0;
			avg_height_neighbors = avg_height_neighbors == 0 ? STANDARD_TERRAIN_HEIGHT : avg_height_neighbors;
			float noise =  getNoise(patch_size, cube_matrix[r][col * VERTICES_PER_CUBE].x, GROUND, cube_matrix[r][col * VERTICES_PER_CUBE].z);
			cube_matrix[r][col * VERTICES_PER_CUBE].y += std::max(max_height_neighbors-BOX_SIZE,std::min(min_height_neighbors+BOX_SIZE,std::max(minTerrainHeight,std::min(maxTerrainHeight,avg_height_neighbors * (1 + noise)))));
			cube_matrix[r][col * VERTICES_PER_CUBE + 1].y += std::max(max_height_neighbors-BOX_SIZE,std::min(min_height_neighbors+BOX_SIZE,std::max(minTerrainHeight,std::min(maxTerrainHeight,avg_height_neighbors * (1 + noise)))));
			cube_matrix[r][col * VERTICES_PER_CUBE + 2].y += std::max(max_height_neighbors-BOX_SIZE,std::min(min_height_neighbors+BOX_SIZE,std::max(minTerrainHeight,std::min(maxTerrainHeight,avg_height_neighbors * (1 + noise)))));
			cube_matrix[r][col * VERTICES_PER_CUBE + 3].y += std::max(max_height_neighbors-BOX_SIZE,std::min(min_height_neighbors+BOX_SIZE,std::max(minTerrainHeight,std::min(maxTerrainHeight,avg_height_neighbors * (1 + noise)))));
			cube_matrix[r][col * VERTICES_PER_CUBE + 4].y += std::max(max_height_neighbors-BOX_SIZE,std::min(min_height_neighbors+BOX_SIZE,std::max(minTerrainHeight,std::min(maxTerrainHeight,avg_height_neighbors * (1 + noise)))));
			cube_matrix[r][col * VERTICES_PER_CUBE + 5].y += std::max(max_height_neighbors-BOX_SIZE,std::min(min_height_neighbors+BOX_SIZE,std::max(minTerrainHeight,std::min(maxTerrainHeight,avg_height_neighbors * (1 + noise)))));
			cube_matrix[r][col * VERTICES_PER_CUBE + 6].y += std::max(max_height_neighbors-BOX_SIZE,std::min(min_height_neighbors+BOX_SIZE,std::max(minTerrainHeight,std::min(maxTerrainHeight,avg_height_neighbors * (1 + noise)))));
			cube_matrix[r][col * VERTICES_PER_CUBE + 7].y += std::max(max_height_neighbors-BOX_SIZE,std::min(min_height_neighbors+BOX_SIZE,std::max(minTerrainHeight,std::min(maxTerrainHeight,avg_height_neighbors * (1 + noise)))));
		}
	}
}

void Terrain::terrainMatrixToVector(std::vector<glm::vec4>& vertices,  std::vector<glm::uvec3>& indices, const GUI& gui,
																								bool gen_index) const 
{
	assert(cube_matrix.size() == num_z && cube_matrix[0].size() == num_x * VERTICES_PER_CUBE);

	vertices.clear();
	vertices.reserve(num_z * num_x * VERTICES_PER_CUBE);
	if(gen_index)
	{
		indices.clear();
		indices.reserve(num_z * num_x * 12);
	}

	for(size_t r = 0; r <  num_z; ++r)
	{
		for(size_t c = 0; c < num_x; ++c)
		{
			/*fixme : add depth checking stuff here */
			bool check = false || gen_index;
			for(size_t _ = 0; _ < 8 && !check; ++_) //culling
			{
				glm::vec3 dir = glm::normalize(glm::vec3(cube_matrix[r][c * VERTICES_PER_CUBE + _]) - gui.getCamera());
				check = gui.inViewFrustum(dir); 
			}
			if(check)
			{
				auto b = cube_matrix[r].begin() + c * VERTICES_PER_CUBE;
				auto e = b + 8;
				size_t start_size = vertices.size();
				vertices.insert(vertices.end(), b, e);
				if(gen_index)
				{
					indices.push_back(glm::uvec3(start_size, start_size+1, start_size+2));
					indices.push_back(glm::uvec3(start_size, start_size+2, start_size+3));

					indices.push_back(glm::uvec3(start_size+6, start_size+ 5, start_size+4));
					indices.push_back(glm::uvec3(start_size+7, start_size+6, start_size+4));

					indices.push_back(glm::uvec3(start_size+3, start_size+2, start_size+6));
					indices.push_back(glm::uvec3(start_size+3, start_size+6, start_size+7));

					indices.push_back(glm::uvec3(start_size+4, start_size+5, start_size+1));
					indices.push_back(glm::uvec3(start_size+4, start_size+1, start_size));

					indices.push_back(glm::uvec3(start_size+1, start_size+5, start_size+2));
					indices.push_back(glm::uvec3(start_size+5, start_size+6, start_size+2));

					indices.push_back(glm::uvec3(start_size+4, start_size+0, start_size+3));
					indices.push_back(glm::uvec3(start_size+4, start_size+3, start_size+7));
				}
			}
			
		}
	}
}

bool Terrain::is_valid(glm::vec3& proj_pos, float entity_height, float entity_width) const {
	float height_margin = entity_height / 3.0f;
	float min_x = cube_matrix[0][0].x;
	float min_z = cube_matrix[0][0].z;
	int row = (proj_pos.z - min_z) / BOX_SIZE;
	int col = (proj_pos.x - min_x) / BOX_SIZE;
	const glm::vec4& proj_box = cube_matrix[row][col * VERTICES_PER_CUBE];
	if(proj_pos.y - entity_height < proj_box.y + BOX_SIZE - height_margin){
		return false;
	}else{
		if(proj_pos.y - entity_height< proj_box.y + 2 * BOX_SIZE) //if not in midair
		{
			//determine quadrant and box intersect
			float box_x_offset = proj_pos.x - proj_box.x;
			float box_z_offset = proj_pos.z - proj_box.z;
			std::unordered_set<size_t> box_indices;
			if(box_x_offset > BOX_SIZE / 2.0f && box_z_offset > BOX_SIZE / 2.0f){
				//call intersect of bottom right quadrant
				glm::vec3 box_1(proj_box[0] + (entity_width/2+ kNear), proj_box[1], proj_box[2]);
				glm::vec3 box_2(proj_box[0] + (entity_width/2+ kNear), proj_box[1], proj_box[2] - (entity_width/2+ kNear));
				glm::vec3 box_3(proj_box[0], proj_box[1], proj_box[2] - (entity_width/2+ kNear)); 
				box_indices.insert(get_box_indices(box_1));
				box_indices.insert(get_box_indices(box_2));
				box_indices.insert(get_box_indices(box_3));
			}else if(box_x_offset > BOX_SIZE / 2.0f && box_z_offset <= BOX_SIZE / 2.0f){
				//call intersect of top right corner
				glm::vec3 box_1(proj_box[0] + (entity_width/2+ kNear), proj_box[1], proj_box[2]);
				glm::vec3 box_2(proj_box[0] + (entity_width/2+ kNear), proj_box[1], proj_box[2] + (entity_width/2+ kNear));
				glm::vec3 box_3(proj_box[0], proj_box[1], proj_box[2] + (entity_width/2+ kNear)); 
				box_indices.insert(get_box_indices(box_1));
				box_indices.insert(get_box_indices(box_2));
				box_indices.insert(get_box_indices(box_3));
			}else if(box_x_offset <= BOX_SIZE / 2.0f && box_z_offset <= BOX_SIZE / 2.0f){
				//call intersect of top left corner
				glm::vec3 box_1(proj_box[0] - (entity_width/2+ kNear), proj_box[1], proj_box[2]);
				glm::vec3 box_2(proj_box[0] - (entity_width/2+ kNear), proj_box[1], proj_box[2] + (entity_width/2+ kNear));
				glm::vec3 box_3(proj_box[0], proj_box[1], proj_box[2] + (entity_width/2+ kNear)); 
				box_indices.insert(get_box_indices(box_1));
				box_indices.insert(get_box_indices(box_2));
				box_indices.insert(get_box_indices(box_3));
			}else if(box_x_offset <= BOX_SIZE / 2.0f && box_z_offset > BOX_SIZE / 2.0f){
				//call intersect of bottom left corner
				glm::vec3 box_1(proj_box[0] - (entity_width/2+ kNear), proj_box[1], proj_box[2]);
				glm::vec3 box_2(proj_box[0] - (entity_width/2+ kNear), proj_box[1], proj_box[2] - (entity_width/2+ kNear));
				glm::vec3 box_3(proj_box[0], proj_box[1], proj_box[2] - (entity_width/2+ kNear)); 
				box_indices.insert(get_box_indices(box_1));
				box_indices.insert(get_box_indices(box_2));
				box_indices.insert(get_box_indices(box_3));
			}
			
			proj_pos.y = std::max(proj_pos.y, proj_box.y + BOX_SIZE + entity_height);
			return true;
			
		} else
		{
			return true;
		}
	}
	return false;
}

size_t Terrain::get_box_indices(const glm::vec3& pos) const{
	float min_x = cube_matrix[0][0].x;
	float min_z = cube_matrix[0][0].z;
	int row = (pos[2] - min_z) / BOX_SIZE;
	int col = (pos[0] - min_x) / BOX_SIZE;
	return row * num_x + col;
}

float Terrain::get_height(const glm::vec3& pos)
{
	return get_height(get_box_indices(pos));
}

template<typename T>
bool Terrain::is_colliding(const T& boxes, const glm::vec3& pos, float entity_height) const{
	float height_margin = entity_height / 3.0f;
	for(size_t index : boxes){
		if(get_height(index) - (pos.y)> height_margin){
			return true;
		}
	}
	return false;
}

float Terrain::get_height(size_t index) const{
	size_t row = index / num_x;
	size_t col = index % num_x;

	return cube_matrix[row][col * VERTICES_PER_CUBE].y + BOX_SIZE;
}