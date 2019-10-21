#ifndef BVHTREE_H
#define BVHTREE_H

#include <list>
#include <vector>
#include <stack>   
#include <glm/gtc/matrix_transform.hpp>
#include <glm/vec3.hpp>
#include <limits>
#include <iostream>
#include <thread>

const float RAY_EPSILON = 0.00001;
class Box
{
	public:
		glm::vec3 bmin;
		glm::vec3 bmax;
		int index;
		Box(const glm::vec3& _bmin,const glm::vec3& _bmax) : bmin(_bmin), bmax(_bmax) {}

		glm::vec3 getCentroid() const
		{
			return bmin + glm::normalize(bmax - bmin) * (glm::distance(bmax, bmin) * 0.5f);
		}
};


class BoxSet
{
	public:
		std::stack<const Box*> boxes;
		std::stack<float> min_stacks[3] = {std::stack<float>(), std::stack<float>(), std::stack<float>()};
		std::stack<float> max_stacks[3] = {std::stack<float>(), std::stack<float>(), std::stack<float>()};

		glm::vec3 bmin;
		glm::vec3 bmax;
		bool dirty = true;
		float bArea = std::numeric_limits<float>::max();
		BoxSet(): bmin(), bmax(), boxes()
		{
		}

		BoxSet(std::vector<const Box*>& _boxes): boxes(), bmin(), bmax()
		{
			for(int j = _boxes.size()-1; j >=0; --j)
				addBox(_boxes[j]);
				
		}

		BoxSet(std::vector<const Box*>& _boxes, int b, int e): boxes(), bmin(), bmax()
		{
			for(int j = e-1; j >=b; --j)
				addBox(_boxes[j]);
				
		}

		void addBox(const Box* box)
		{
			glm::vec3 boxBMin = box->bmin;
			glm::vec3 boxBMax = box->bmax;
			if(empty())
			{
				bmin = boxBMin;
				bmax = boxBMax;
				dirty = true;
			} else
			{
				for(int i = 0; i < 3; ++i)
				{
					if(boxBMin[i] <= bmin[i])
					{
						dirty = boxBMin[i] != bmin[i];
						min_stacks[i].push(bmin[i]);
						bmin[i] = boxBMin[i];
					}
					if(boxBMax[i] >= bmax[i])
					{
						dirty = boxBMax[i] != bmax[i];
						max_stacks[i].push(bmax[i]);
						bmax[i] = boxBMax[i];
					}
				}
			}
			boxes.push(box);
		}

		const Box* popBox()
		{
			if(boxes.empty()){
				dirty = true;
				return NULL;
			}

			const Box* box = boxes.top();
			glm::vec3 boxBMin = box->bmin;
			glm::vec3 boxBMax = box->bmax;


			for(int i = 0; i < 3; ++i)
			{
				if(boxBMin[i] == bmin[i] && !min_stacks[i].empty())
				{
					dirty = true;
					min_stacks[i].pop();
					if(!min_stacks[i].empty()){
						bmin[i] = min_stacks[i].top();
						min_stacks[i].pop();
					}
				}
				if(boxBMax[i] == bmax[i] && !max_stacks[i].empty())
				{
					dirty = true;
					max_stacks[i].pop();
					if(!max_stacks[i].empty()){
						bmax[i] = max_stacks[i].top();
						max_stacks[i].pop();
					}	
				}
			}
			boxes.pop();
			if(boxes.empty()){
				dirty = true;
			}
			return box;
		}

		int size()
		{
			return boxes.size();
		}

		float empty()
		{
			return boxes.empty();
		}

		float area()
		{
			if (empty())
				return std::numeric_limits<float>::max();
			else if (dirty) {
			
				dirty = false;
				bArea = 2.0 * ((bmax[0] - bmin[0]) * (bmax[1] - bmin[1]) +
				               (bmax[1] - bmin[1]) * (bmax[2] - bmin[2]) +
				               (bmax[2] - bmin[2]) * (bmax[0] - bmin[0]));
			}
			return bArea;
		}
};



class BVHNode
{
    public:
        glm::vec3 bmin;
        glm::vec3 bmax;
        int left; //index to left box
        int right; //index to right box
        int box_id;

	    BVHNode(const glm::vec3& _bmin, const glm::vec3& _bmax) : bmin(_bmin), bmax(_bmax), left(-1), right(-1), box_id(-1) {}
	    BVHNode() : bmin(0), bmax(0), left(-1), right(-1), box_id(-1) {}
	    BVHNode(const BVHNode&) = default;
	    BVHNode(BVHNode&&) = default;
};





template<int I=0>
struct axis_sort
{
    inline bool operator() (const Box* struct1, const Box* struct2)
    {
        return (struct1->getCentroid()[I] < struct2->getCentroid()[I]);
    }
};

class BVHTree
{
	public:
		std::vector<BVHNode*> nodes;
		BVHTree(std::vector<const Box*>& boxes, int depth = 15) : nodes()
		{

			nodes.push_back(new BVHNode()); //root always occupy index 0
			std::cout << nodes.size() <<std::endl;
			helperTree(0, boxes, 0, boxes.size());
		}


		void helperTree(int node_id, std::vector<const Box*>& boxes, int b, int e)
		{
			float bestCost = e-b;
			int bestAxis = -1;
			int bestIndex = -1; 
			std::vector<const Box*> part(begin(boxes)+b, begin(boxes)+e);
			BoxSet S(boxes,b,e);
			nodes[node_id]->bmin = S.bmin;
			nodes[node_id]->bmax = S.bmax;
			for(int axis = 0; axis < 3; ++axis)
			{
				//sort accross the axis of the centroid
				if(axis == 0)
					std::sort(begin(boxes) +b, begin(boxes) + e, axis_sort<0>());
				else if(axis == 1)
					std::sort(begin(boxes) +b, begin(boxes) + e, axis_sort<1>());
				else if(axis == 2)
					std::sort(begin(boxes) +b, begin(boxes) + e, axis_sort<2>());


				BoxSet S1;
				BoxSet S2(boxes,b,e);
				float leftArea[e-b];
				int i = 0;
				while(!S2.empty())
				{
					leftArea[i] =  S1.area(); //stores the area at this point
					//std::cout << S1.area() << std::endl;
					float cost = leftArea[i] / S.area() * S1.size() +  S2.area() / S.area() * S2.size();
					if(bestCost > cost)
					{
						bestCost = cost;
						bestIndex = i;
						bestAxis = axis;
					}
					const Box* split = S2.popBox();
					S1.addBox(split);	 
					++i;
				}
			}	
			//std::cout <<bestAxis << std::endl;
			if(bestAxis == -1)
			{
				nodes[node_id]->left = -1;
				nodes[node_id]->right = -1;
				assert(part.size() == 1 && part[0]->index != -1);
				nodes[node_id]->box_id = part[0]->index;
			} else 
			{
				if(bestAxis == 0)
					std::sort(begin(boxes) + b, begin(boxes) + e, axis_sort<0>());
				else if(bestAxis == 1)
					std::sort(begin(boxes) + b, begin(boxes) +  e, axis_sort<1>());
				else if(bestAxis == 2)
					std::sort(begin(boxes) + b, begin(boxes) + e, axis_sort<2>());
				
				nodes.push_back(new BVHNode());
				nodes.push_back(new BVHNode());
				nodes[node_id]->left = nodes.size() - 2;
				nodes[node_id]->right = nodes.size() - 1;
				std::cout<< nodes[node_id]->left << " - "  << nodes[node_id]->right << std::endl;
				nodes[node_id]->box_id = -1;
				std::cout << nodes[node_id]->left << std::endl;
				helperTree(nodes[node_id]->left, boxes, b, b + bestIndex);
				helperTree(nodes[node_id]->right, boxes, b+bestIndex, e);	
			}
		}

};

#endif