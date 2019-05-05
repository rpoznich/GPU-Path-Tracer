#ifndef SKINNING_GUI_H
#define SKINNING_GUI_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/vector_angle.hpp>
#include "config.h"
#include <glm/gtx/io.hpp>

#include <iostream>
#include <GLFW/glfw3.h>
#include <vector>
#include <chrono>
struct Mesh;
struct Terrain;
/*
 * Hint: call glUniformMatrix4fv on thest pointers
 */
struct MatrixPointers {
	const glm::mat4 *projection, *model, *view, *mvp;
};

class GUI {
public:
	GUI(GLFWwindow*, int view_width = -1, int view_height = -1, int preview_height = -1);
	~GUI();
	void assignMesh(Mesh*);

	void keyCallback(int key, int scancode, int action, int mods);
	void mousePosCallback(double mouse_x, double mouse_y);
	void mouseButtonCallback(int button, int action, int mods);
	void mouseScrollCallback(double dx, double dy);
	void updateMatrices(bool hard_update);
	MatrixPointers getMatrixPointers() const;

	static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void MousePosCallback(GLFWwindow* window, double mouse_x, double mouse_y);
	static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
	static void MouseScrollCallback(GLFWwindow* window, double dx, double dy);

	glm::vec3 getCenter() const { return center_; }
	bool inViewFrustum  (const glm::vec3& other) const{ 
		return glm::angle(look_, other) < (kFov * (M_PI / 180.0f)  );
	}
	const glm::vec3& getCamera() const { return eye_; }
	const float* getLightPositionPtr() const { return &light_position_[0]; }
	
	bool isDirty() const { return is_dirty;}
	bool no_clip() const { return !fps_mode_;}

	const glm::vec3& getLook() const { return look_;}

	const glm::vec3& getTangent() const { return tangent_;}
	void assignTerrain(Terrain* t) { terra = t;}
	void clean() { is_dirty = true;}
	bool isTransparent() const { return transparent_; }
	bool isPlaying() const { return play_; }
	float getCurrentPlayTime() const;

	glm::vec3 getProjectedPos() const
	{
		return proj_pos_;
	}

	void updateEye(const glm::vec3& proj_pos)
	{
		eye_ = proj_pos;
	}

	glm::vec3 getEyeRay(float x, float y) {
		glm::vec4 temp(x,y,0,1.0f);
		glm::vec4 res = glm::inverse(mvp) * temp;
		return glm::normalize(glm::vec3(res / res.w) - eye_);
	}

	

	void updateProjectedPos(const glm::vec3& _p)
	{
		proj_pos_ = _p;
	}
	bool snap_shot = false;
	double offset = 0; //offset for rendering the sidebar snapshots
	unsigned int ss_index = -1; //index for which sidebar snapshot the user has selected
	bool u_key_pressed = false;
	bool delete_key_pressed = false;
	float y_force = 0.0f;
	float x_force = 0.0f;
	float z_force = 0.0f;
private:
	GLFWwindow* window_;
	Terrain* terra;

	int window_width_, window_height_;
	int view_width_, view_height_;
	int preview_height_;

	bool drag_state_ = false;
	bool fps_mode_ = true;
	bool pose_changed_ = true;
	bool transparent_ = false;
	int current_button_ = -1;
	bool is_dirty = true;
	float roll_speed_ = M_PI / 64.0f;
	float last_x_ = 0.0f, last_y_ = 0.0f, current_x_ = 0.0f, current_y_ = 0.0f;
	float camera_distance_ = 2.0;
	float pan_speed_ = 0.1f;
	float rotation_speed_ = 0.02f;
	float zoom_speed_ = 0.1f;
	float aspect_;

	glm::vec3 look_ = glm::normalize(glm::vec3(0.0f, 1.0/sqrt(2), 1.0/sqrt(2)));
	glm::vec3 up_ = glm::vec3(0.0f, 1.0/sqrt(2), -1.0/sqrt(2));
	glm::vec3 eye_ = glm::vec3(43.0f, camera_distance_, 43.0f);
	glm::vec3 proj_pos_ = glm::vec3(eye_);
	glm::vec3 tangent_ = glm::cross(look_, up_);
	glm::vec3 center_ = eye_ - camera_distance_ * look_;
	glm::mat3 orientation_ = glm::mat3(tangent_, up_, look_);
	glm::vec4 light_position_;
	glm::vec3 rotate_vec;
	
	bool squad = false; //quads quaternion interpolation ( more advanced  interpolation)
	glm::mat4 view_matrix_ = glm::lookAt(eye_, center_, up_);
	glm::mat4 projection_matrix_;
	glm::mat4 model_matrix_ = glm::mat4(1.0f);
	glm::mat4 mvp;
	bool captureWASDUPDOWN(int key, int action);

	bool play_ = false;
};

#endif
