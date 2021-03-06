/****************************************************************************
 *
 *   Copyright (c) 2018 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/
/**
 * @file FlightTaskManual.cpp
 */

#include "FlightTaskManual.hpp"
#include <mathlib/mathlib.h>
#include <float.h>

using namespace matrix;

FlightTaskManual::FlightTaskManual(control::SuperBlock *parent, const char *name) :
	FlightTask(parent, name),
	_stick_dz(parent, "MPC_HOLD_DZ", false),
	_xy_vel_man_expo(parent, "MPC_XY_MAN_EXPO", false),
	_z_vel_man_expo(parent, "MPC_Z_MAN_EXPO", false)
{
}

bool FlightTaskManual::initializeSubscriptions(SubscriptionArray &subscription_array)
{
	if (!FlightTask::initializeSubscriptions(subscription_array)) {
		return false;
	}

	if (!subscription_array.get(ORB_ID(manual_control_setpoint), _sub_manual_control_setpoint)) {
		return false;
	}

	return true;
}

bool FlightTaskManual::updateInitialize()
{
	bool ret = FlightTask::updateInitialize();
	const bool sticks_available = _evaluateSticks();

	if (_sticks_data_required) {
		ret = ret && sticks_available;
	}

	return ret;
}

bool FlightTaskManual::_evaluateSticks()
{
	/* Sticks are rescaled linearly and exponentially to [-1,1] */
	if ((_time_stamp_current - _sub_manual_control_setpoint->get().timestamp) < _timeout) {

		/* Linear scale  */
		_sticks(0) = _sub_manual_control_setpoint->get().x; /* NED x, "pitch" [-1,1] */
		_sticks(1) = _sub_manual_control_setpoint->get().y; /* NED y, "roll" [-1,1] */
		_sticks(2) = -(_sub_manual_control_setpoint->get().z - 0.5f) * 2.f; /* NED z, "thrust" resacaled from [0,1] to [-1,1] */
		_sticks(3) = _sub_manual_control_setpoint->get().r; /* "yaw" [-1,1] */

		/* Exponential scale */
		_sticks_expo(0) = math::expo_deadzone(_sticks(0), _xy_vel_man_expo.get(), _stick_dz.get());
		_sticks_expo(1) = math::expo_deadzone(_sticks(1), _xy_vel_man_expo.get(), _stick_dz.get());
		_sticks_expo(2) = math::expo_deadzone(_sticks(2), _z_vel_man_expo.get(), _stick_dz.get());

		return true;

	} else {
		/* Timeout: set all sticks to zero */
		_sticks.zero();
		_sticks_expo.zero();
		return false;
	}
}
