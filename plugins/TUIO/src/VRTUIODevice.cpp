﻿/*
 * Copyright Regents of the University of Minnesota, 2015.  This software is released under the following license: http://opensource.org/licenses/GPL-2.0.
 * Source code originally developed at the University of Minnesota Interactive Visualization Lab (http://ivlab.cs.umn.edu).
 *
 * Code author(s):
 * 		Dan Orban (dtorban)
 */


#include "VRTUIODevice.h"

#include <list>

using namespace TUIO;

namespace MinVR {

VRTUIODevice::VRTUIODevice(int port, double xScale, double yScale)
{
	_xScale = xScale;
	_yScale = yScale;
	_tuioClient = new TuioClient(port);
	_tuioClient->connect();

	if (!_tuioClient->isConnected())
	{  
		std::cout << "InputDeviceTUIOClient: Cannot connect on port " << port << "." << std::endl;
	}
}

VRTUIODevice::~VRTUIODevice()
{
	if (_tuioClient) {
		_tuioClient->disconnect();
		delete _tuioClient;
	}
}

void VRTUIODevice::appendNewInputEventsSinceLastCall(VRDataQueue *inputEvents)
{

	// Send out events for TUIO "cursors" by polling the TuioClient for the current state
	std::list<TuioCursor*> cursorList = _tuioClient->getTuioCursors();
	_tuioClient->lockCursorList();

	// Send "button" up events for cursors that were down last frame, but are now up.
	std::vector<int> downLast;
	for (std::set<int>::iterator downLast_it = _cursorsDown.begin(); downLast_it!= _cursorsDown.end(); ++downLast_it ) {
		downLast.push_back(*downLast_it);
	}

	for (int i=0;i<downLast.size();i++) {
		bool stillDown = false;
		for (std::list<TuioCursor*>::iterator iter = cursorList.begin(); iter!=cursorList.end(); iter++) {
			TuioCursor *tcur = (*iter);
			if (tcur->getCursorID() == downLast[i]) {
				stillDown = true;
			}
		}
		if (!stillDown) {
			std::string event = "Touch_Cursor_Up";
			_dataIndex.addData(event + "/Id", downLast[i]);
			inputEvents->push(_dataIndex.serialize(event));
			_cursorsDown.erase(downLast[i]);
		}
	}

	// Send "button" down events for cursors that are new and updated positions for all cursors
	for (std::list<TuioCursor*>::iterator iter = cursorList.begin(); iter!=cursorList.end(); iter++) {
		TuioCursor *tcur = (*iter);

		if (_cursorsDown.find(tcur->getCursorID()) == _cursorsDown.end()) {
			std::string event = "Touch_Cursor_Down";
			_dataIndex.addData(event + "/Id", tcur->getCursorID());
			_dataIndex.addData(event + "/XPos", (float)_xScale*tcur->getX());
			_dataIndex.addData(event + "/YPos", (float)_yScale*tcur->getY());
			inputEvents->push(_dataIndex.serialize(event));
			_cursorsDown.insert(tcur->getCursorID());
		}

		if (tcur->getMotionSpeed() > 0.0) {
			std::string event = "Touch_Cursor_Move";
			_dataIndex.addData(event + "/Id", tcur->getCursorID());
			_dataIndex.addData(event + "/XPos", (float)_xScale*tcur->getX());
			_dataIndex.addData(event + "/YPos", (float)_yScale*tcur->getY());
			inputEvents->push(_dataIndex.serialize(event));
		}

		// Can also access several other properties of cursors (speed, acceleration, path followed, etc.)
		//std::cout << "cur " << tcur->getCursorID() << " (" <<  tcur->getSessionID() << ") " << tcur->getX() << " " << tcur->getY()
		// << " " << tcur->getMotionSpeed() << " " << tcur->getMotionAccel() << " " << std::endl;

		// This is how to access all the points in the path that a cursor follows:
		//std::list<TuioPoint> path = tuioCursor->getPath();
		//if (path.size() > 0) {
		//  TuioPoint last_point = path.front();
		//  for (std::list<TuioPoint>::iterator point = path.begin(); point!=path.end(); point++) {
		//    last_point.update(point->getX(),point->getY());
		//  }
		//}
	}

	_tuioClient->unlockCursorList();
}

VRInputDevice* VRTUIODevice::create(VRMainInterface* vrMain,
		VRDataIndex* config, const std::string& nameSpace) {
	int port = TUIO_PORT;
	if (config->exists("Port", nameSpace)) {
		port = config->getValue("Port", nameSpace);
	}
	int xs = 1.0;
	if (config->exists("XScaleFactor", nameSpace)) {
		xs = config->getValue("XScaleFactor", nameSpace);
	}
	int ys = 1.0;
	if (config->exists("YScaleFactor", nameSpace)) {
		ys = config->getValue("YScaleFactor", nameSpace);
	}

	return new VRTUIODevice(port, xs, ys);
}

};         // end namespace
