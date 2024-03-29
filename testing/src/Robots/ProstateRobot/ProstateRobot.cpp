//============================================================================
// Name:   ProstateRobot.cpp
// Author: Produced in the WPI AIM Lab
// Description:  This file defines the methods and parameters of the BRP Robot
//============================================================================

#include "ProstateRobot.hpp"

// Prostate Robot Constructor
// Requires Pointers to a Packets class (used for global access to SPI data)
// And an FPGA_Utility class (used for global access to FPGA specific functions ie LEDs)
ProstateRobot::ProstateRobot(unsigned int loopRate) 
{
	// Name of the robot used in main menus
	_name = "Prostate Robot";
	_timer = Timer();
	_loopRate = loopRate;
	// Current state at the startup is UNDEFINED
	current_state = kStateNames.UNDEFINED;
	// Robot Mode used to specify specific robot states for asynchronous functionalities
	_mode = kRobotModes.CLINICAL_MODE;
	// Initialize robot modes
	clinical_mode = make_shared<ProstateRobotClinicalMode>(&motion_controller_, &kinematics_controller_);
	clinical_mode->SetBaseToTreatmentRobotCoord(&base_to_treatment_robot_coord);
	clinical_mode->SetBaseToDesiredTargetRobotCoord(&base_to_desired_target_robot_coord);
	manual_mode = make_shared<ProstateRobotManualMode>(&motion_controller_);
	automated_homing_mode = make_shared<ProstateRobotAutomatedHomingMode>(&motion_controller_);
	frequency_sweep_mode = make_shared<ProstateRobotFrequencySweepMode>(&motion_controller_);
	active_steering_mode = make_shared<ProstateRobotActiveSteeringMode>(&motion_controller_);
	// Will populate the drop down menu at the Engineer's UI
	robot_modes_list_.push_back(clinical_mode);
	robot_modes_list_.push_back(manual_mode);
	robot_modes_list_.push_back(automated_homing_mode);
	robot_modes_list_.push_back(frequency_sweep_mode);
	robot_modes_list_.push_back(active_steering_mode);
	//=================== Prostate Robot Kinematics ==================
	// {_needleGauge,_needleLength,_bevelAngle}
	biopsy_needle = {18, 224.0, 22.5};
	prostate_kinematics_ = ProstateKinematics(&this->biopsy_needle);
	target_full_pose_image_coord.setIdentity();
	registration = Eigen::Matrix<double, 4, 4, Eigen::DontAlign>::Identity();
	base_to_treatment_robot_coord = Eigen::Matrix<double, 4, 4, Eigen::DontAlign>::Identity();
	base_to_desired_target_robot_coord = Eigen::Matrix<double, 4, 4, Eigen::DontAlign>::Identity();
	current_pose_image_coord = Eigen::Matrix<double, 4, 4, Eigen::DontAlign>::Identity();
	reachable_target_pose_imager_coord = Eigen::Matrix<double, 4, 4, Eigen::DontAlign>::Identity();
	robot_base_to_zframe = Eigen::Matrix<double, 4, 4, Eigen::DontAlign>::Identity();
	robot_base_to_zframe(1, 3) = kConstants.robot_base_to_zframe_y_offset;
	robot_base_to_zframe(2, 3) = kConstants.robot_base_to_zframe_z_offset;
}

ProstateRobot::~ProstateRobot()
{
}

// This is the central ProstateRobot method -- everything happens in this method
// This method should never be blocking, it must be asynchronous
void ProstateRobot::Update()
{
	// Update sensor and encoder readings via the SPI packets
	// motion_controller_.UpdateMotorReadings();
	// sensors_.UpdateSensorReadings();
	if (_mode == kRobotModes.CLINICAL_MODE)
	{
		// motors_queue.front()->_setpointList.pop();
		clinical_mode->Run(current_state); // Set the setpoints to the home configuration.
		// clinical_mode->Run(current_state, motors_queue); // Set the setpoints to the home configuration.
	}
	else if (_mode == kRobotModes.MANUAL_MODE)
	{
		manual_mode->Run(current_state);
	}
	else if (_mode == kRobotModes.AUTOMATED_HOMING_MODE)
	{
		automated_homing_mode->Run(current_state);
	}
	else if (_mode == kRobotModes.FREQUENCY_SWEEP_MODE)
	{
		frequency_sweep_mode->Run(current_state);
	}
	else if (_mode == kRobotModes.ACTIVE_STEERING_MODE)
	{
		active_steering_mode->Run(current_state);
	}
	// Update tip's position w.r.t image frame
	// UpdateNeedleTipPose();
}

// Calculates reachable target point based on the kinematics constraints of the robot
ProstateRobotMotorSetpointMap ProstateRobot::RunAxisSetpointValidator()
{
	ProstateRobotMotorSetpointMap valid_axis_setpoints = kinematics_controller_.AxisSetpointValidator();
	return valid_axis_setpoints;
}

void ProstateRobot::RunInverseKinematics()
{
	// Define the desired target w.r.t robot base
	base_to_desired_target_robot_coord = ConvertFromImagerToRobotBase(target_full_pose_image_coord);
	// Find kinematically valid setpoints based on the received target
	ProstateRobotMotorSetpointMap valid_axis_setpoints = kinematics_controller_.CalculateKinematicallyValidSetpoints(base_to_desired_target_robot_coord);
	Logger &log = Logger::GetInstance();
	// Update motor values based on validated setpoints
	if (_mode == kRobotModes.CLINICAL_MODE)
	{	
		// Logger &log = Logger::GetInstance();
		// log.Log("In Clinical Mode, run Syn Motion.", logger::INFO, true);
		// InitSynMotion(valid_axis_setpoints);
	}
	else
	{	
		// set setpoints
		// for (auto it = valid_axis_setpoints.begin(); it != valid_axis_setpoints.end(); it++)
		// {
		// 	motors_.SetDesiredSetpoint(it->first, it->second);
		// }
	}
	// Log the IK output and print it to the console
	LogInverseKinematicsOutput();
	// Calculate the reachable target based on the validated setpoints
	ProstateRobotForwardKinematicsInput fk_input = GetAllMotorsSetpointPositionUnit();
	Eigen::Matrix<double, 4, 4, Eigen::DontAlign> reachable_target_pose_robot_coord = prostate_kinematics_.ForwardKinematics(fk_input).BaseToTreatment;
	reachable_target_pose_imager_coord = ConvertFromRobotBaseToImager(reachable_target_pose_robot_coord);
}

void ProstateRobot::InitSynMotion(ProstateRobotMotorSetpointMap valid_axis_setpoints)
{
	Logger &log = Logger::GetInstance();
	log.Log("Syn Motion", logger::INFO, true);

	// // empty motors_queue
	// while (!motors_queue.empty())
	// {
	// 	motors_queue.pop();
	// }

	// // Find the step number based on max diff
	// vector<int> diff_list;
	// for (auto it = valid_axis_setpoints.begin(); it != valid_axis_setpoints.end(); it++)
	// {
	// 	diff_list.push_back(motors_.CalculateMotionDiff(it->first, it->second));
	// }
	// int max_diff = *max_element(diff_list.begin(), diff_list.end());

	// int step_num = 45;
	// if (max_diff <= 1e5)
	// {
	// 	step_num = 10;
	// }
	// else if (max_diff <= 2e5)
	// {
	// 	step_num = 20;
	// }
	// else if (max_diff <= 3e5)
	// {
	// 	step_num = 30;
	// }
	// else if (max_diff <= 4e5)
	// {
	// 	step_num = 40;
	// }
	// // Generate the setpoint lists
	// for (auto it = valid_axis_setpoints.begin(); it != valid_axis_setpoints.end(); it++)
	// {
	// 	if  (it->first == ProstateRobotMotor::INSERTION)
	// 	{
	// 		motors_.SetDesiredSetpoint(it->first, it->second);
	// 	}
	// 	else
	// 	{
	// 		motors_.SaveSetpointList(it->first, it->second, step_num);
	// 		motors_queue.push(motors_.GetMotor(it->first));
	// 	}
	// }


}

void ProstateRobot::LogInverseKinematicsOutput()
{
	// Profiling the output of the IK
	Logger &log = Logger::GetInstance();
	// log.Log("Inverse Kinematics -- Front Left: " + to_string(motors_.GetMotor(ProstateRobotMotor::FRONT_LEFT)->GetSetPointInPositionUnit()) +
	// 			" mm | Front Right: " + to_string(motors_.GetMotor(ProstateRobotMotor::FRONT_RIGHT)->GetSetPointInPositionUnit()) +
	// 			" mm | Back Left: " + to_string(motors_.GetMotor(ProstateRobotMotor::BACK_LEFT)->GetSetPointInPositionUnit()) +
	// 			" mm | Back Right: " + to_string(motors_.GetMotor(ProstateRobotMotor::BACK_RIGHT)->GetSetPointInPositionUnit()) +
	// 			" mm | Needle Insertion: " + to_string(motors_.GetMotor(ProstateRobotMotor::INSERTION)->GetSetPointInPositionUnit()) +
	// 			" mm | Needle Rotation: " + to_string((motors_.GetMotor(ProstateRobotMotor::ROTATION)->GetSetPointInPositionUnit()) * (180 / 3.14)) + " deg.",
	// 		logger::INFO, true);
}

ProstateRobotForwardKinematicsInput ProstateRobot::GetAllMotorsSetpointPositionUnit()
{
	ProstateRobotForwardKinematicsInput fk_input;
	// fk_input.front_left = motors_.GetMotor(ProstateRobotMotor::FRONT_LEFT)->GetSetPointInPositionUnit();
	// fk_input.front_right = motors_.GetMotor(ProstateRobotMotor::FRONT_RIGHT)->GetSetPointInPositionUnit();
	// fk_input.back_left = motors_.GetMotor(ProstateRobotMotor::BACK_LEFT)->GetSetPointInPositionUnit();
	// fk_input.back_right = motors_.GetMotor(ProstateRobotMotor::BACK_RIGHT)->GetSetPointInPositionUnit();
	// fk_input.insertion = motors_.GetMotor(ProstateRobotMotor::INSERTION)->GetSetPointInPositionUnit();
	return fk_input;
}

ProstateRobotForwardKinematicsInput ProstateRobot::GetAllMotorsCurrentPositionUnit()
{
	ProstateRobotForwardKinematicsInput fk_input;
	// fk_input.front_left = motors_.GetMotor(ProstateRobotMotor::FRONT_LEFT)->GetEncoderPositionUnit();
	// fk_input.front_right = motors_.GetMotor(ProstateRobotMotor::FRONT_RIGHT)->GetEncoderPositionUnit();
	// fk_input.back_left = motors_.GetMotor(ProstateRobotMotor::BACK_LEFT)->GetEncoderPositionUnit();
	// fk_input.back_right = motors_.GetMotor(ProstateRobotMotor::BACK_RIGHT)->GetEncoderPositionUnit();
	// fk_input.insertion = motors_.GetMotor(ProstateRobotMotor::INSERTION)->GetEncoderPositionUnit();
	return fk_input;
}

// vector<Motor *> ProstateRobot::ListMotors()
// {
// 	return motors_.GetMotorsListVector();
// }

// Motor *ProstateRobot::GetMotor(int cardID)
// {
// 	vector<Motor *> motors = ListMotors();
// 	for (Motor *motor : motors)
// 	{
// 		if (motor->_cardID == cardID)
// 		{
// 			return motor;
// 		}
// 	}

// 	return NULL;
// }

bool ProstateRobot::CheckForStalls()
{
	// return motors_.CheckForStalls();
	return false;
}

// Method for stopping the robot
void ProstateRobot::StopRobot()
{
	motion_controller_.StopRobotMotion();
}

// Method for zeroing the robot
void ProstateRobot::ZeroRobot()
{
	// motors_.SetCurrentEncoderReadingAsHomeForAllMotors();
	// sensors_.GetSensor(ProstateRobotAnalogDigitalId::INPUT_SENSOR)->SetForceReference();
	// sensors_.GetSensor(ProstateRobotAnalogDigitalId::NEEDLE_SENSOR)->SetForceReference();
}

// This function will update the current position of the needle tip using the FK of the robot
void ProstateRobot::SetNeedleLength(const double &needle_length)
{
	biopsy_needle._needleLength = needle_length;
	prostate_kinematics_.UpdateNeedleLength();
	if (_mode == kRobotModes.CLINICAL_MODE)
	{
		if (GetCalibrationFlag() && GetTargetFlag())
		{
			RunInverseKinematics();
		}
	}
	else
	{
		RunInverseKinematics();
	}
}

Eigen::Matrix<double, 4, 4, Eigen::DontAlign> ProstateRobot::ConvertFromImagerToRobotBase(const Eigen::Matrix<double, 4, 4, Eigen::DontAlign> &matrix_img_coord)
{
	return robot_base_to_zframe * GetRegistration().inverse() * matrix_img_coord;
}

Eigen::Matrix<double, 4, 4, Eigen::DontAlign> ProstateRobot::ConvertFromRobotBaseToImager(const Eigen::Matrix<double, 4, 4, Eigen::DontAlign> &matrix_rbt_coord)
{
	return GetRegistration() * robot_base_to_zframe.inverse() * matrix_rbt_coord;
}

Eigen::Matrix<double, 3, 1, Eigen::DontAlign> ProstateRobot::GetTargetPointPosVectorImageCoord()
{
	return Eigen::Matrix<double, 3, 1, Eigen::DontAlign>(target_full_pose_image_coord.block(0, 3, 3, 1));
}

void ProstateRobot::SetTargetPointPosVectorImageCoord(const Eigen::Matrix<double, 3, 1, Eigen::DontAlign> &tgt_vector)
{
	target_full_pose_image_coord.block(0, 3, 3, 1) = tgt_vector;
	if (_mode == kRobotModes.CLINICAL_MODE)
	{
		if (GetCalibrationFlag() && GetTargetFlag())
		{
			RunInverseKinematics();
		}
	}
	else
	{
		RunInverseKinematics();
	}
}

/* Use the forward kinematics to calculated needle tip postion */
void ProstateRobot::UpdateNeedleTipPose()
{
	// ProstateRobotForwardKinematicsInput fk_input = GetAllMotorsCurrentPositionUnit();
	// base_to_treatment_robot_coord = prostate_kinematics_.ForwardKinematics(fk_input).BaseToTreatment;
	// current_pose_image_coord = ConvertFromRobotBaseToImager(base_to_treatment_robot_coord);
}

/* This function will reset the planning attributes of the robot upon discontenting from the planning software.*/
void ProstateRobot::Reset()
{
	// Reset planning attribues
	target_full_pose_image_coord.setIdentity();
	reachable_target_pose_imager_coord.setIdentity();
	registration.setIdentity();
	// Reset the flags related to registration and target
	SetTargetFlag(false);
	SetCalibrationFlag(false);
	// Cleanup reported needle tip positions
	clinical_mode->CleanUp();
	// Display the reset to the user
	Logger &log = Logger::GetInstance();
	std::string message{"========= Lost connection with the planning software ========"};
	log.Log(message, logger::WARNING, true);
	message.clear();
	message = {"========= Robot planning attributes have been reset ========="};
	log.Log(message, logger::INFO, true);
}

void ProstateRobot::SetRegistration(const Eigen::Matrix<double, 4, 4, Eigen::DontAlign> &matrix)
{
	SetCalibrationFlag(true);
	this->registration = matrix;
	if (_mode == kRobotModes.CLINICAL_MODE)
	{
		if (GetTargetFlag())
		{
			RunInverseKinematics();
		}
	}
	else
	{
		RunInverseKinematics();
	}
}

void ProstateRobot::SetTargetImageCoord(const Eigen::Matrix<double, 4, 4, Eigen::DontAlign> &matrix)
{
	SetTargetFlag(true);
	target_full_pose_image_coord = matrix;
	Logger &log = Logger::GetInstance();
	// if (GetCalibrationFlag())
	// {
	// 	RunInverseKinematics();
	// }
}

void ProstateRobot::UpdateTargetImageCoord(const Eigen::Matrix<double, 4, 4, Eigen::DontAlign> &matrix)
{
	target_full_pose_image_coord = matrix;
	UpdateBaseToTarget();
}

void ProstateRobot::UpdateBaseToTarget()
{
	base_to_desired_target_robot_coord = ConvertFromImagerToRobotBase(target_full_pose_image_coord);
}

Eigen::Matrix<double, 4, 4, Eigen::DontAlign> ProstateRobot::GetCurrentPositionKinematicImageCoord()
{
	Eigen::Matrix<double, 4, 4, Eigen::DontAlign> kinematic_tip_image_coord;
	return kinematic_tip_image_coord = ConvertFromRobotBaseToImager(clinical_mode->GetBaseToTreatmentRobotCoordKinematic());
}

bool ProstateRobot::isHomed()
{
	// // Is true ONLY if all motors are homed.
	// for (auto motor : motors_.GetMotorsListMap())
	// {
	// 	if (!motor.second->_homed)
	// 	{
	// 		return false;
	// 	}
	// }
	return true;
}

vector<string> ProstateRobot::GetRobotModesList()
{
	vector<string> list;
	for (const auto &ptr : robot_modes_list_)
	{
		if (auto mode = ptr.lock())
		{
			list.push_back(mode->GetName());
		}
	}
	return list;
}