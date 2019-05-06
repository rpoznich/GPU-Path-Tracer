#include "gui.h"
#include <jpegio.h>
#include <algorithm>
#include <debuggl.h>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/io.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include "terrain.h"
#include "jpegio.h"

namespace {
	// FIXME: Implement a function that performs proper
	//        ray-cylinder intersection detection
	// TIPS: The implement is provided by the ray-tracer starter code.
}

double getTime()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>
        		(std::chrono::system_clock::now().time_since_epoch()).count() / 1000.0;
}

GUI::GUI(GLFWwindow* window, int view_width, int view_height, int preview_height)
	:window_(window), preview_height_(preview_height)
{
	glfwSetWindowUserPointer(window_, this);
	glfwSetKeyCallback(window_, KeyCallback);
	glfwSetCursorPosCallback(window_, MousePosCallback);
	glfwSetMouseButtonCallback(window_, MouseButtonCallback);
	glfwSetScrollCallback(window_, MouseScrollCallback);

	glfwGetWindowSize(window_, &window_width_, &window_height_);
	if (view_width < 0 || view_height < 0) {
		view_width_ = window_width_;
		view_height_ = window_height_;
	} else {
		view_width_ = view_width;
		view_height_ = view_height;
	}
	float aspect_ = static_cast<float>(view_width_) / view_height_;
	projection_matrix_ = glm::perspective((float)(kFov * (M_PI / 180.0f)), aspect_, kNear, kFar);
	mvp = projection_matrix_* view_matrix_ * model_matrix_;
}

GUI::~GUI()
{
}




void GUI::keyCallback(int key, int scancode, int action, int mods)
{
#if 0
	if (action != 2)
		std::cerr << "Key: " << key << " action: " << action << std::endl;
#endif
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window_, GL_TRUE);
		return ;
	}
	if (key == GLFW_KEY_J && action == GLFW_RELEASE) {
		//FIXME save out a screenshot using SaveJPEG
		unsigned char colors[this->view_width_ * this->view_height_ * 3];
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glReadPixels(0, 0, this->view_width_, this->view_height_, GL_RGB, GL_UNSIGNED_BYTE, colors);
		
		SaveJPEG("screenshot.jpg", view_width_, view_height_, colors);
		
	}

	if (captureWASDUPDOWN(key, action))
		return ;
	if (key == GLFW_KEY_C && action != GLFW_RELEASE) {
		proj_pos_ = eye_;
		fps_mode_ = !fps_mode_;
	}
	// FIXME: implement other controls here.
}

float get360Angle(const glm::vec2& v1, const glm::vec2& v2)
{
	float dot = v1.x*v2.x + v1.y*v2.y;
	float det = v1.x*v2.y - v1.y*v2.x;     
	return atan2(det,dot);
}

void GUI::mousePosCallback(double mouse_x, double mouse_y)
{
	last_x_ = current_x_;
	last_y_ = current_y_;
	current_x_ = mouse_x;
	current_y_ = window_height_ - mouse_y;
	float delta_x = current_x_ - last_x_;
	float delta_y = current_y_ - last_y_;
	if (sqrt(delta_x * delta_x + delta_y * delta_y) < 1e-15)
		return;
	if (mouse_x > view_width_){
		return ;
	}
	glm::vec3 mouse_direction = glm::vec3(delta_x , delta_y, 0.0f);
	glm::vec2 mouse_start = glm::vec2(last_x_, last_y_);
	glm::vec2 mouse_end = glm::vec2(current_x_, current_y_);
	glm::uvec4 viewport = glm::uvec4(0, 0, view_width_, view_height_);	
	bool drag_camera = true;

	if (drag_camera) {
		float old_y = tangent_[1];
		glm::vec3 mouse_vec(glm::normalize(-delta_x* tangent_ - delta_y*up_));
		glm::vec3 rotate_vec(glm::normalize(glm::cross(mouse_vec, look_)));
		glm::mat4 R = glm::rotate(rotation_speed_,rotate_vec);
		tangent_ = glm::normalize(glm::vec3(R * glm::vec4(tangent_,0.0f)));
		tangent_[1] = old_y ; //make sure the rolling doesnt change
		up_ = glm::normalize(glm::vec3(R * glm::vec4(up_,0.0f)));
		up_ = up_ - glm::dot(up_,tangent_)*tangent_; //make up orthogonal to tangent_ again
		look_ = glm::normalize(glm::cross(up_,tangent_));
		is_dirty = true;
	}
	is_dirty = true;
	// FIXME: highlight bones that have been moused over
}




void GUI::mouseButtonCallback(int button, int action, int mods)
{
	if (current_x_ <= view_width_) {
		drag_state_ = (action == GLFW_PRESS);
		current_button_ = button;
		return ;
	}
}



void GUI::mouseScrollCallback(double dx, double dy)
{

}

void GUI::updateMatrices(bool hard_update)
{
	// Compute our view, and projection matrices.
	center_ = eye_ + camera_distance_ * look_;

	view_matrix_ = glm::lookAt(eye_, center_, up_);
	light_position_ = glm::vec4(eye_, 1.0f);

	if(hard_update)
	{
		aspect_ = static_cast<float>(view_width_) / view_height_;
		projection_matrix_ =
			glm::perspective((float)(kFov * (M_PI / 180.0f)), aspect_, kNear, kFar);
	}
	model_matrix_ = glm::mat4(1.0f);
	mvp = projection_matrix_* view_matrix_ * model_matrix_;
	is_dirty = false;
}

MatrixPointers GUI::getMatrixPointers() const
{
	MatrixPointers ret;
	ret.projection = &projection_matrix_;
	ret.model= &model_matrix_;
	ret.view = &view_matrix_;
	ret.mvp = &mvp;
	return ret;
}



float GUI::getCurrentPlayTime() const
{
	return 0.0f;
}


bool GUI::captureWASDUPDOWN(int key, int action)
{
	is_dirty = true;

	if (key == GLFW_KEY_W) {
		if (fps_mode_){
			z_force += 0.5;
		}
		else
			eye_ += zoom_speed_ * look_;
		return true;
	} else if (key == GLFW_KEY_S) {
		if (fps_mode_){
			z_force -= 0.5;
		}
		else
			eye_ -= zoom_speed_ * look_;
		return true;
	} else if (key == GLFW_KEY_A) {
		if (fps_mode_){
			x_force -= 0.5;
			
		}
		else
			eye_ -= zoom_speed_ * tangent_;
		return true;
	} else if (key == GLFW_KEY_D) {
		if (fps_mode_){
			x_force += 0.5;
		}
		else
			eye_ += zoom_speed_ * tangent_;
		return true;
	} else if (key == GLFW_KEY_DOWN) {
		if (fps_mode_)
			proj_pos_ -= pan_speed_ * up_;
		else
			eye_ -= zoom_speed_ * look_;
		return true;
	} else if (key == GLFW_KEY_SPACE) {
		if (fps_mode_){
			//proj_pos_ =eye_ +  4.0f * glm::vec3(0,1.0,0);
			if(terra->get_height(eye_) + BOX_SIZE >= eye_.y - 1.75 * BOX_SIZE)
				y_force = 4.0f;
		}else
			eye_ -= zoom_speed_ * look_;
		return true;
	}
	return false;
}


// Delegrate to the actual GUI object.
void GUI::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	GUI* gui = (GUI*)glfwGetWindowUserPointer(window);
	gui->keyCallback(key, scancode, action, mods);
}

void GUI::MousePosCallback(GLFWwindow* window, double mouse_x, double mouse_y)
{
	GUI* gui = (GUI*)glfwGetWindowUserPointer(window);
	gui->mousePosCallback(mouse_x, mouse_y);
}

void GUI::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	GUI* gui = (GUI*)glfwGetWindowUserPointer(window);
	gui->mouseButtonCallback(button, action, mods);
}

void GUI::MouseScrollCallback(GLFWwindow* window, double dx, double dy)
{
	GUI* gui = (GUI*)glfwGetWindowUserPointer(window);
	gui->mouseScrollCallback(dx, dy);
}
