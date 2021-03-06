#include "jam-engine/Core/Controller.hpp"

#include <cassert>

#include "jam-engine/Core/Level.hpp"
#include "jam-engine/Utility/Math.hpp"
#include "jam-engine/Utility/Trig.hpp"

namespace je
{
/*			bind			*/
Controller::Bind::Bind()
	:key(0)
	,device(Device::Invalid)
	,reversed(false)
{
}
Controller::Bind::Bind(sf::Keyboard::Key key)
	:key(key)
	,device(Device::Keyboard)
	,reversed(false)
{
}

Controller::Bind::Bind(sf::Mouse::Button button)
	:key(button)
	,device(Device::Mouse)
	,reversed(false)
{
}

Controller::Bind::Bind(unsigned int joystickButton)
	:key(joystickButton)
	,device(Device::Joystick)
	,reversed(false)
{
}

Controller::Bind::Bind(sf::Joystick::Axis axis, bool reversed)
	:key(axis)
	,device(Device::JoyAxis)
	,reversed(reversed)
{
}

/*			axis bind			*/
Controller::AxisBind::AxisBind()
	:device(Device::Invalid)
	,reversed(false)
	,interval(0.f, 0.f)
	,pos(nullptr)
{
}

Controller::AxisBind::AxisBind(MouseAxis axis, bool rev, Interval interval, const float *pos)
	:device(Device::Mouse)
	,mAxis(axis)
	,reversed(rev)
	,interval(interval)
	,pos(pos)
{
}

Controller::AxisBind::AxisBind(sf::Joystick::Axis axis, bool rev, Interval interval)
	:device(Device::JoyAxis)
	,jAxis(axis)
	,reversed(rev)
	,interval(interval)
	,pos(nullptr)
{
}

Controller::AxisBind::AxisBind(const Bind& negative, const Bind& positive)
	:device(Device::Buttons)
	,bAxis({negative, positive})
	,reversed(false)
	,interval()
	,pos(nullptr)
{
}

Controller::AxisBind::Interval::Interval()
	:min(-1.f)
	,max(1.f)
{
}

Controller::AxisBind::Interval::Interval(float min, float max)
	:min(min)
	,max(max)
{
}


/*			controller			*/
Controller::Controller(Input& input, unsigned int joyID)
	:input(input)
	,joyID(joyID)
{
}


bool Controller::isActionPressed(const std::string& action) const
{
	auto it = binds.find(action);
	if (it != binds.end())
	{
		for (const Bind& bind : it->second)
		{
			if ((bind.device == Bind::Device::Keyboard && input.isKeyPressed((sf::Keyboard::Key) bind.key)) ||
				(bind.device == Bind::Device::Mouse && input.isButtonPressed((sf::Mouse::Button) bind.key)) ||
				(bind.device == Bind::Device::Joystick && input.isJoyButtonPressed(joyID, bind.key)) ||
				(bind.device == Bind::Device::JoyAxis && input.isJoyAxisPressed(joyID, (sf::Joystick::Axis) bind.key, bind.reversed)))
				return true;
		}
	}
	return false;
}

bool Controller::isActionReleased(const std::string& action) const
{
	auto it = binds.find(action);
	if (it != binds.end())
	{
		for (const Bind& bind : it->second)
		{
			if ((bind.device == Bind::Device::Keyboard && input.isKeyReleased((sf::Keyboard::Key) bind.key)) ||
				(bind.device == Bind::Device::Mouse && input.isButtonReleased((sf::Mouse::Button) bind.key)) ||
				(bind.device == Bind::Device::Joystick && input.isJoyButtonReleased(joyID, bind.key)) ||
				(bind.device == Bind::Device::JoyAxis && input.isJoyAxisReleased(joyID, (sf::Joystick::Axis) bind.key, bind.reversed)))
				return true;
		}
	}
	return false;
}

bool Controller::isActionHeld(const std::string& action) const
{
	auto it = binds.find(action);
	if (it != binds.end())
	{

		for (const Bind& bind : it->second)
		{
			if (isBindHeld(bind))
				return true;
		}
	}
	return false;
}

bool Controller::isBindHeld(const Bind& bind) const
{
	return (bind.device == Bind::Device::Keyboard && input.isKeyHeld((sf::Keyboard::Key) bind.key)) ||
		   (bind.device == Bind::Device::Mouse && input.isButtonHeld((sf::Mouse::Button) bind.key)) ||
	       (bind.device == Bind::Device::Joystick && input.isJoyButtonHeld(joyID, bind.key)) ||
	       (bind.device == Bind::Device::JoyAxis && input.isJoyAxisHeld(joyID, (sf::Joystick::Axis) bind.key, bind.reversed));
}



void Controller::addKeybind(const std::string& action, Bind bind)
{
	binds[action].push_back(bind);
}

void Controller::setKeybinds(const std::string& action, std::initializer_list<Bind> binds)
{
	this->binds[action].clear();
	for (Bind bind : binds)
		this->binds[action].push_back(bind);
}

void Controller::removeKeybinds(const std::string& action)
{
	binds[action].clear();
}

void Controller::removeKeybinds()
{
	binds.clear();
}


/*		joystick specific		*/
void Controller::setJoystickID(unsigned int id)
{
	joyID = id;
}

void Controller::setAxis(const std::string& name, const AxisBind& bind)
{
	auto it = boundAxes.find(name);
	if (it != boundAxes.end())
	{
		boundAxes.erase(it);
	}
	boundAxes.insert(std::make_pair(name, bind));
}

float Controller::axisPos(const std::string& axis, je::Level *level) const
{
	float origin = 0;
	auto it = boundAxes.find(axis);
	if (it != boundAxes.end())
	{
		assert(it->second.pos != nullptr);
		origin = *(it->second.pos);
	}
	return axisPos(axis, origin, level);
}

float Controller::axisPos(const std::string& axis, float origin, je::Level *level) const
{
	auto it = boundAxes.find(axis);
	if (it == boundAxes.end())
	{
		return 0;
	}
	else
	{
		float ret = 0;
		AxisBind bind = it->second;
		switch (bind.device)
		{
			case AxisBind::Device::Mouse:
				assert(level != nullptr);
				switch (bind.mAxis)
				{
					case AxisBind::MouseAxis::X:
						ret = level->getCursorPos().x - origin;
						break;
					case AxisBind::MouseAxis::Y:
						ret = level->getCursorPos().y - origin;
						break;
					default:
						ret = (bind.interval.max - bind.interval.min) / 2;
						break;
				}
				break;
			case AxisBind::Device::JoyAxis:
				ret = input.axisPos(joyID, bind.jAxis);
				break;
			case AxisBind::Device::Buttons:
				{
					const bool negHeld = isBindHeld(bind.bAxis.neg);
					const bool posHeld = isBindHeld(bind.bAxis.pos);
					if (posHeld && !negHeld)
						ret = 1.f;//bind.interval.max;
					else if (negHeld && !posHeld)
						ret = -1.f;//bind.interval.min;
					else
						ret = 0.f;//(bind.interval.max - bind.interval.min) / 2;
				}
				break;
			default:
				ret = 0;
				break;
		}
		//	cap the value to the interval
		if (ret > bind.interval.max)
			ret = bind.interval.max;
		if (ret < bind.interval.min)
			ret = bind.interval.min;
		//	adjust the value to the interval by finding how far along the interval is
		//	then dividing that by the interval absolute difference to give us a number in [0, 1]
		const float intervalDif = (bind.interval.max - bind.interval.min);
		ret = (ret - bind.interval.min) / (intervalDif);
		//	now adjust that to [-1, 1]
		ret = 2.f * ret - 1.f;

		return bind.reversed ? -ret : ret;
	}
}

Controller::Bind Controller::getLastInputAsBind() const
{
	//	check keys
	sf::Keyboard::Key key;
	if (input.testKey(key))
		return Bind(key);
	//	check mouse buttons
	sf::Mouse::Button button;
	if (input.testButton(button))
		return Bind(button);
	//	check joypad buttons
	unsigned int joyButton;
	if (input.testJoyButton(joyID, joyButton))
		return Bind(joyButton);
	//	check joypad axes
	sf::Joystick::Axis axis;
	bool reversed;
	if (input.testAxis(joyID, axis, reversed))
		return Bind(axis, reversed);
	//	nothing was pressed, return empty bind (invalid)
	return Bind();
}

Controller::AxisBind Controller::getLastAxisMovementAsBind() const
{
	sf::Joystick::Axis axis;
	bool reversed;
	if (input.testAxis(joyID, axis, reversed))
		return AxisBind(axis, reversed);
	//	nothing was pressed, return empty bind (invalid)
	return AxisBind();
}

/*			axes			*/
Axes::Axes(Controller& controller, const std::string& xAxis, const std::string& yAxis)
	:controller(&controller)
	,xAxis(xAxis)
	,yAxis(yAxis)
{
}

sf::Vector2f Axes::getPos(const sf::Vector2f& origin, je::Level *level) const
{
	sf::Vector2f pos(controller->axisPos(xAxis, origin.x, level), controller->axisPos(yAxis, origin.y, level));
	if (je::abs(je::length(pos)) > 1.f)
	{
		return je::lengthdir(1.f, je::direction(pos));
	}
	return pos;
}


/*			axesset			*/
AxesSet::AxesSet()
	:lastUsedIndex(-1)
{
}

AxesSet::AxesSet(std::initializer_list<Axes> axesList)
	:lastUsedIndex(0)
	,lastValues(axesList.size()) // (0.f, 0.f)
{
	for (const Axes& axes : axesList)
	{
		this->axesList.push_back(axes);
	}
}

sf::Vector2f AxesSet::getPos(const sf::Vector2f& origin, je::Level *level) const
{
	if (axesList.empty())
	{
		return sf::Vector2f(0.f, 0.f);
	}
	sf::Vector2f result;
	for (std::size_t i = 0; i < axesList.size(); ++i)
	{
		result = axesList[i].getPos(origin, level);
		if (result != lastValues[i])
		{
			lastUsedIndex = i;
			lastValues[i] = result;
			break;
		}
	}
	return lastValues[lastUsedIndex];
}


} // je
