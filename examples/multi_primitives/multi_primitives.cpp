/*
 * Example of a controller for a Kuka arm made with the motion arm primitive for redundant arms
 *
 */

#include <iostream>
#include <string>
#include <thread>
#include <math.h>
#include "redis/RedisClient.h"

#include "Sai2Model.h"
#include "Sai2Graphics.h"
#include "Sai2Simulation.h"
#include <dynamics3d.h>

#include "primitives/RedundantArmMotion.h"
#include "timer/LoopTimer.h"

#include <GLFW/glfw3.h> //must be loaded after loading opengl/glew as part of graphicsinterface

#include <signal.h>
bool fSimulationRunning = false;
void sighandler(int){fSimulationRunning = false;}

using namespace std;

const string world_file = "resources/world.urdf";
const string robot_file = "resources/kuka_iiwa.urdf";
const string robot_name = "Kuka-IIWA";

const string camera_name = "camera";

RedisClient redis_client;

// control init
const string CONTROL_STATE_KEY = "sai2::sai2Interfaces::control_state";
const string CONTROL_STATE_INITIALIZING = "initializing";
const string CONTROL_STATE_INITIALIZED = "initialized";
const string CONTROL_STATE_READY = "ready";

// operational space control
const string DESIRED_POS_KEY_X = "sai2::sai2Interfaces::desired_position::x";
const string DESIRED_POS_KEY_Y = "sai2::sai2Interfaces::desired_position::y";
const string DESIRED_POS_KEY_Z = "sai2::sai2Interfaces::desired_position::z";
const string DESIRED_ORI_KEY_X = "sai2::sai2Interfaces::desired_orientation::x";
const string DESIRED_ORI_KEY_Y = "sai2::sai2Interfaces::desired_orientation::y";
const string DESIRED_ORI_KEY_Z = "sai2::sai2Interfaces::desired_orientation::z";
const string KP_POS_KEY = "sai2::sai2Interfaces::kp_pos";
const string KV_POS_KEY = "sai2::sai2Interfaces::kv_pos";
const string KP_ORI_KEY = "sai2::sai2Interfaces::kp_ori";
const string KV_ORI_KEY = "sai2::sai2Interfaces::kv_ori";

// joint space control
const string DESIRED_JOINT_POS_KEY = "sai2::sai2Interfaces::desired_joint_position";
const string KP_JOINT_KEY = "sai2::sai2Interfaces::kp_joint";
const string KV_JOINT_KEY = "sai2::sai2Interfaces::kv_joint";

// pick and place control
const string GRASP_COMMAND = "sai2::sai2Interfaces::grasp";
const string GRASP_OPEN = "o";
const string GRASP_CLOSE = "c";

// simulation and control loop
void control(Sai2Model::Sai2Model* robot, Simulation::Sai2Simulation* sim);
void simulation(Sai2Model::Sai2Model* robot, Simulation::Sai2Simulation* sim);

// initialize window manager
GLFWwindow* glfwInitialize();

// callback to print glfw errors
void glfwError(int error, const char* description);

// callback when a key is pressed
void keySelect(GLFWwindow* window, int key, int scancode, int action, int mods);

// callback when a mouse button is pressed
void mouseClick(GLFWwindow* window, int button, int action, int mods);

// flags for scene camera movement
bool fTransXp = false;
bool fTransXn = false;
bool fTransYp = false;
bool fTransYn = false;
bool fTransZp = false;
bool fTransZn = false;
bool fRotPanTilt = false;

string dtos(double x) {
    std::stringstream s;  // Allocates memory on stack
    s << x;
    return s.str();       // returns a s.str() as a string by value
                          // Frees allocated memory of s
} 

int main (int argc, char** argv) {
	cout << "Loading URDF world model file: " << world_file << endl;

	// start redis client
	HiredisServerInfo info;
	info.hostname_ = "127.0.0.1";
	info.port_ = 6379;
	info.timeout_ = { 1, 500000 }; // 1.5 seconds
	redis_client = RedisClient();
	redis_client.serverIs(info);

	// set up signal handler
	signal(SIGABRT, &sighandler);
	signal(SIGTERM, &sighandler);
	signal(SIGINT, &sighandler);

	// load graphics scene
	auto graphics = new Sai2Graphics::Sai2Graphics(world_file, false);
	Eigen::Vector3d camera_pos, camera_lookat, camera_vertical;
	graphics->getCameraPose(camera_name, camera_pos, camera_vertical, camera_lookat);

	// load simulation world
	auto sim = new Simulation::Sai2Simulation(world_file, false);

	// load robots
	Eigen::Vector3d world_gravity = sim->_world->getGravity().eigen();
	auto robot = new Sai2Model::Sai2Model(robot_file, false, world_gravity, sim->getRobotBaseTransform(robot_name));

	sim->getJointPositions(robot_name, robot->_q);
	robot->updateModel();

	// initialize GLFW window
	GLFWwindow* window = glfwInitialize();

	double last_cursorx, last_cursory;

    // set callbacks
	glfwSetKeyCallback(window, keySelect);
	glfwSetMouseButtonCallback(window, mouseClick);

	// start the simulation thread first
	fSimulationRunning = true;
	thread sim_thread(simulation, robot, sim);

	// next start the control thread
	thread ctrl_thread(control, robot, sim);
	
    // while window is open:
    while (!glfwWindowShouldClose(window)) {
		// update kinematic models
		// robot->updateModel();

		// update graphics. this automatically waits for the correct amount of time
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		graphics->updateGraphics(robot_name, robot);
		graphics->render(camera_name, width, height);
		glfwSwapBuffers(window);
		glFinish();

	    // poll for events
	    glfwPollEvents();

		// move scene camera as required
    	// graphics->getCameraPose(camera_name, camera_pos, camera_vertical, camera_lookat);
    	Eigen::Vector3d cam_depth_axis;
    	cam_depth_axis = camera_lookat - camera_pos;
    	cam_depth_axis.normalize();
    	Eigen::Vector3d cam_up_axis;
    	// cam_up_axis = camera_vertical;
    	// cam_up_axis.normalize();
    	cam_up_axis << 0.0, 0.0, 1.0; //TODO: there might be a better way to do this
	    Eigen::Vector3d cam_roll_axis = (camera_lookat - camera_pos).cross(cam_up_axis);
    	cam_roll_axis.normalize();
    	Eigen::Vector3d cam_lookat_axis = camera_lookat;
    	cam_lookat_axis.normalize();
    	if (fTransXp) {
	    	camera_pos = camera_pos + 0.05*cam_roll_axis;
	    	camera_lookat = camera_lookat + 0.05*cam_roll_axis;
	    }
	    if (fTransXn) {
	    	camera_pos = camera_pos - 0.05*cam_roll_axis;
	    	camera_lookat = camera_lookat - 0.05*cam_roll_axis;
	    }
	    if (fTransYp) {
	    	// camera_pos = camera_pos + 0.05*cam_lookat_axis;
	    	camera_pos = camera_pos + 0.05*cam_up_axis;
	    	camera_lookat = camera_lookat + 0.05*cam_up_axis;
	    }
	    if (fTransYn) {
	    	// camera_pos = camera_pos - 0.05*cam_lookat_axis;
	    	camera_pos = camera_pos - 0.05*cam_up_axis;
	    	camera_lookat = camera_lookat - 0.05*cam_up_axis;
	    }
	    if (fTransZp) {
	    	camera_pos = camera_pos + 0.1*cam_depth_axis;
	    	camera_lookat = camera_lookat + 0.1*cam_depth_axis;
	    }	    
	    if (fTransZn) {
	    	camera_pos = camera_pos - 0.1*cam_depth_axis;
	    	camera_lookat = camera_lookat - 0.1*cam_depth_axis;
	    }
	    if (fRotPanTilt) {
	    	// get current cursor position
	    	double cursorx, cursory;
			glfwGetCursorPos(window, &cursorx, &cursory);
			//TODO: might need to re-scale from screen units to physical units
			double compass = 0.006*(cursorx - last_cursorx);
			double azimuth = 0.006*(cursory - last_cursory);
			double radius = (camera_pos - camera_lookat).norm();
			Eigen::Matrix3d m_tilt; m_tilt = Eigen::AngleAxisd(azimuth, -cam_roll_axis);
			camera_pos = camera_lookat + m_tilt*(camera_pos - camera_lookat);
			Eigen::Matrix3d m_pan; m_pan = Eigen::AngleAxisd(compass, -cam_up_axis);
			camera_pos = camera_lookat + m_pan*(camera_pos - camera_lookat);
	    }
	    graphics->setCameraPose(camera_name, camera_pos, cam_up_axis, camera_lookat);
	    glfwGetCursorPos(window, &last_cursorx, &last_cursory);
	}

	// stop simulation
	fSimulationRunning = false;
	sim_thread.join();
	ctrl_thread.join();

    // destroy context
    glfwDestroyWindow(window);

    // terminate
    glfwTerminate();

	return 0;
}

//------------------------------------------------------------------------------
void control(Sai2Model::Sai2Model* robot, Simulation::Sai2Simulation* sim) {
	
	// IMPORTANT: first thing to do in controller
	// this informs the UI, that controller is not ready yet,
	// and user is disabled from updating redis values
	redis_client.set(CONTROL_STATE_KEY, CONTROL_STATE_INITIALIZING);

	robot->updateModel();
	int dof = robot->dof();
	Eigen::VectorXd command_torques = Eigen::VectorXd::Zero(dof);

	string link_name = "link6";
	Eigen::Vector3d pos_in_link = Eigen::Vector3d(0.0, 0.0, 0.0);

	string redis_buffer;

	// Motion arm primitive
	Sai2Primitives::RedundantArmMotion* motion_primitive = new Sai2Primitives::RedundantArmMotion(robot, link_name, pos_in_link);
	Eigen::VectorXd motion_primitive_torques;
	motion_primitive->enableGravComp();

	Eigen::Matrix3d desired_rmat;
	double desired_euler_x;
	double desired_euler_y;
	double desired_euler_z;
	Eigen::Vector3d desired_pos;
	double desired_pos_x;
	double desired_pos_y;
	double desired_pos_z;

	// set initial position, orientation
	Eigen::Matrix3d initial_rmat;	
	Eigen::Vector3d initial_euler;	
	Eigen::Vector3d initial_position;	
	robot->rotation(initial_rmat, motion_primitive->_link_name);	
	robot->position(initial_position, motion_primitive->_link_name, motion_primitive->_control_frame.translation());
	initial_euler = initial_rmat.eulerAngles(2, 1, 0);

	redis_client.set(DESIRED_POS_KEY_X, std::to_string(initial_position(0)));
	redis_client.set(DESIRED_POS_KEY_Y, std::to_string(initial_position(1)));
	redis_client.set(DESIRED_POS_KEY_Z, std::to_string(initial_position(2)));
	redis_client.set(DESIRED_ORI_KEY_X, std::to_string(initial_euler(0)));
	redis_client.set(DESIRED_ORI_KEY_Y, std::to_string(initial_euler(1)));
	redis_client.set(DESIRED_ORI_KEY_Z, std::to_string(initial_euler(2)));
	
	redis_client.set(KP_POS_KEY, std::to_string(motion_primitive->_posori_task->_kp_pos));	
	redis_client.set(KV_POS_KEY, std::to_string(motion_primitive->_posori_task->_kv_pos));	
	redis_client.set(KP_ORI_KEY, std::to_string(motion_primitive->_posori_task->_kp_ori));	
	redis_client.set(KV_ORI_KEY, std::to_string(motion_primitive->_posori_task->_kv_ori));	
	redis_client.set(KP_JOINT_KEY, std::to_string(motion_primitive->_joint_task->_kp));	
	redis_client.set(KV_JOINT_KEY, std::to_string(motion_primitive->_joint_task->_kv));
	
	// IMPORTANT: after controller is initialized
	// this informs the UI, that controller is ready,
	// UI will fetch new value from redis,
	// and user is allowed to update redis values
	redis_client.set(CONTROL_STATE_KEY, CONTROL_STATE_INITIALIZED);
	
	// create a loop timer
	double control_freq = 1000;
	LoopTimer timer;
	timer.setLoopFrequency(control_freq);   // 1 KHz
	double last_time = timer.elapsedTime(); //secs
	bool fTimerDidSleep = true;
	timer.initializeTimer(1000000); // 1 ms pause before starting loop

	unsigned long long controller_counter = 0;

	while (fSimulationRunning) { //automatically set to false when simulation is quit
		fTimerDidSleep = timer.waitForNextLoop();

		// update time
		double curr_time = timer.elapsedTime();
		double loop_dt = curr_time - last_time;

		// read joint positions, velocities, update model
		sim->getJointPositions(robot_name, robot->_q);
		sim->getJointVelocities(robot_name, robot->_dq);
		robot->updateModel();

		// update tasks model
		motion_primitive->updatePrimitiveModel();

		// -------------------------------------------
		////////////////////////////// Compute joint torques
		double time = controller_counter/control_freq;

		// read gains from redis
		redis_client.getCommandIs(KP_POS_KEY, redis_buffer);
		motion_primitive->_posori_task->_kp_pos = std::stod(redis_buffer);
		
		redis_client.getCommandIs(KV_POS_KEY, redis_buffer);
		motion_primitive->_posori_task->_kv_pos = std::stod(redis_buffer);
		
		redis_client.getCommandIs(KP_ORI_KEY, redis_buffer);
		motion_primitive->_posori_task->_kp_ori = std::stod(redis_buffer);
		
		redis_client.getCommandIs(KV_ORI_KEY, redis_buffer);
		motion_primitive->_posori_task->_kv_ori = std::stod(redis_buffer);
		
		redis_client.getCommandIs(KP_JOINT_KEY, redis_buffer);
		motion_primitive->_joint_task->_kp = std::stod(redis_buffer);
		
		redis_client.getCommandIs(KV_JOINT_KEY, redis_buffer);
		motion_primitive->_joint_task->_kv = std::stod(redis_buffer);

		// orientation part
		redis_client.getCommandIs(DESIRED_ORI_KEY_X, redis_buffer);
		desired_euler_x = std::stod(redis_buffer);
		redis_client.getCommandIs(DESIRED_ORI_KEY_Y, redis_buffer);
		desired_euler_y = std::stod(redis_buffer);
		redis_client.getCommandIs(DESIRED_ORI_KEY_Z, redis_buffer);
		desired_euler_z = std::stod(redis_buffer);
		desired_rmat = Eigen::AngleAxisd(desired_euler_z, Eigen::Vector3d::UnitZ())
						 * Eigen::AngleAxisd(desired_euler_y, Eigen::Vector3d::UnitY())
						 * Eigen::AngleAxisd(desired_euler_x, Eigen::Vector3d::UnitX());
		motion_primitive->_desired_orientation = desired_rmat;

		// position part
		redis_client.getCommandIs(DESIRED_POS_KEY_X, redis_buffer);
		desired_pos_x = std::stod(redis_buffer);
		redis_client.getCommandIs(DESIRED_POS_KEY_Y, redis_buffer);
		desired_pos_y = std::stod(redis_buffer);
		redis_client.getCommandIs(DESIRED_POS_KEY_Z, redis_buffer);
		desired_pos_z = std::stod(redis_buffer);
		desired_pos = Eigen::Vector3d(desired_pos_x, desired_pos_y, desired_pos_z);
		motion_primitive->_desired_position = desired_pos;

		// joint part
		redis_client.getEigenMatrixDerived(DESIRED_JOINT_POS_KEY, motion_primitive->_joint_task->_desired_position);

		// torques
		motion_primitive->computeTorques(motion_primitive_torques);

		//------ Final torques
		command_torques = motion_primitive_torques;
		// command_torques.setZero();

		// -------------------------------------------
		sim->setJointTorques(robot_name, command_torques);
		

		// -------------------------------------------
		if(controller_counter % 500 == 0)
		{
			// cout << time << endl;
			// cout << endl;
		}

		controller_counter++;

		// -------------------------------------------
		// update last time
		last_time = curr_time;
	}

	double end_time = timer.elapsedTime();
    std::cout << "\n";
    std::cout << "Control Loop run time  : " << end_time << " seconds\n";
    std::cout << "Control Loop updates   : " << timer.elapsedCycles() << "\n";
    std::cout << "Control Loop frequency : " << timer.elapsedCycles()/end_time << "Hz\n";

}

//------------------------------------------------------------------------------
void simulation(Sai2Model::Sai2Model* robot, Simulation::Sai2Simulation* sim) {
	fSimulationRunning = true;

	// create a timer
	LoopTimer timer;
	timer.initializeTimer();
	timer.setLoopFrequency(2000); 
	double last_time = timer.elapsedTime(); //secs
	bool fTimerDidSleep = true;

	while (fSimulationRunning) {
		fTimerDidSleep = timer.waitForNextLoop();

		// integrate forward
		sim->integrate(0.0005);

	}

	double end_time = timer.elapsedTime();
    std::cout << "\n";
    std::cout << "Simulation Loop run time  : " << end_time << " seconds\n";
    std::cout << "Simulation Loop updates   : " << timer.elapsedCycles() << "\n";
    std::cout << "Simulation Loop frequency : " << timer.elapsedCycles()/end_time << "Hz\n";
}


//------------------------------------------------------------------------------
GLFWwindow* glfwInitialize() {
		/*------- Set up visualization -------*/
    // set up error callback
    glfwSetErrorCallback(glfwError);

    // initialize GLFW
    glfwInit();

    // retrieve resolution of computer display and position window accordingly
    GLFWmonitor* primary = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primary);

    // information about computer screen and GLUT display window
	int screenW = mode->width;
    int screenH = mode->height;
    int windowW = 0.8 * screenH;
    int windowH = 0.5 * screenH;
    int windowPosY = (screenH - windowH) / 2;
    int windowPosX = windowPosY;

    // create window and make it current
    glfwWindowHint(GLFW_VISIBLE, 0);
    GLFWwindow* window = glfwCreateWindow(windowW, windowH, "SAI2.0 - CS327a HW2", NULL, NULL);
	glfwSetWindowPos(window, windowPosX, windowPosY);
	glfwShowWindow(window);
    glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	return window;
}

//------------------------------------------------------------------------------

void glfwError(int error, const char* description) {
	cerr << "GLFW Error: " << description << endl;
	exit(1);
}

//------------------------------------------------------------------------------

void keySelect(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	bool set = (action != GLFW_RELEASE);
    switch(key) {
		case GLFW_KEY_ESCAPE:
			// exit application
			glfwSetWindowShouldClose(window,GL_TRUE);
			break;
		case GLFW_KEY_RIGHT:
			fTransXp = set;
			break;
		case GLFW_KEY_LEFT:
			fTransXn = set;
			break;
		case GLFW_KEY_UP:
			fTransYp = set;
			break;
		case GLFW_KEY_DOWN:
			fTransYn = set;
			break;
		case GLFW_KEY_A:
			fTransZp = set;
			break;
		case GLFW_KEY_Z:
			fTransZn = set;
			break;
		default:
			break;
    }
}

//------------------------------------------------------------------------------

void mouseClick(GLFWwindow* window, int button, int action, int mods) {
	bool set = (action != GLFW_RELEASE);
	//TODO: mouse interaction with robot
		switch (button) {
		// left click pans and tilts
		case GLFW_MOUSE_BUTTON_LEFT:
			fRotPanTilt = set;
			// NOTE: the code below is recommended but doesn't work well
			// if (fRotPanTilt) {
			// 	// lock cursor
			// 	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			// } else {
			// 	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			// }
			break;
		// if right click: don't handle. this is for menu selection
		case GLFW_MOUSE_BUTTON_RIGHT:
			//TODO: menu
			break;
		// if middle click: don't handle. doesn't work well on laptops
		case GLFW_MOUSE_BUTTON_MIDDLE:
			break;
		default:
			break;
	}
}