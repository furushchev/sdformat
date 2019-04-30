/*
 * Copyright 2018 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/
#include <string>
#include <vector>
#include <ignition/math/Pose3.hh>
#include "sdf/AirPressure.hh"
#include "sdf/Altimeter.hh"
#include "sdf/Error.hh"
#include "sdf/Magnetometer.hh"
#include "sdf/Sensor.hh"
#include "sdf/Types.hh"
#include "Utils.hh"

using namespace sdf;

/// Sensor type strings. These should match the data in
/// `enum class SensorType` located in Sensor.hh.
const std::vector<std::string> sensorTypeStrs =
{
  "none",
  "altimeter",
  "camera",
  "contact",
  "depth_camera",
  "force_torque",
  "gps",
  "gpu_lidar",
  "imu",
  "logical_camera",
  "magnetometer",
  "multicamera",
  "lidar",
  "rfid",
  "rfidtag",
  "sonar",
  "wireless_receiver",
  "wireless_transmitter",
  "air_pressure"
};

class sdf::SensorPrivate
{
  /// \brief Default constructor
  public: SensorPrivate() = default;

  /// \brief Copy constructor
  public: explicit SensorPrivate(const SensorPrivate &_sensor)
          : type(_sensor.type),
            name(_sensor.name),
            topic(_sensor.topic),
            pose(_sensor.pose),
            poseFrame(_sensor.poseFrame)

  {
    if (_sensor.magnetometer)
    {
      this->magnetometer = std::make_unique<sdf::Magnetometer>(
          *_sensor.magnetometer);
    }
    if (_sensor.altimeter)
    {
      this->altimeter = std::make_unique<sdf::Altimeter>(*_sensor.altimeter);
    }
    if (_sensor.airPressure)
    {
      this->airPressure = std::make_unique<sdf::AirPressure>(
          *_sensor.airPressure);
    }
    // Developer note: If you add a new sensor type, make sure to also
    // update the Sensor::operator== function. Please bump this text down as
    // new sensors are added so that the next developer see the message.
  }
  // Delete copy assignment so it is not accidentally used
  public: SensorPrivate &operator=(const SensorPrivate &) = delete;

  // \brief The sensor type.
  //
  public: SensorType type = SensorType::NONE;

  /// \brief Name of the sensor.
  public: std::string name = "";

  /// \brief Sensor data topic.
  public: std::string topic = "";

  /// \brief Pose of the sensor
  public: ignition::math::Pose3d pose = ignition::math::Pose3d::Zero;

  /// \brief Frame of the pose.
  public: std::string poseFrame = "";

  /// \brief The SDF element pointer used during load.
  public: sdf::ElementPtr sdf;

  /// \brief Pointer to a magnetometer.
  public: std::unique_ptr<Magnetometer> magnetometer;

  /// \brief Pointer to an altimeter.
  public: std::unique_ptr<Altimeter> altimeter;

  /// \brief Pointer to an air pressure sensor.
  public: std::unique_ptr<AirPressure> airPressure;
  // Developer note: If you add a new sensor type, make sure to also
  // update the Sensor::operator== function. Please bump this text down as
  // new sensors are added so that the next developer see the message.

  /// \brief The frequency at which the sensor data is generated.
  /// If left unspecified (0.0), the sensor will generate data every cycle.
  public: double updateRate = 0.0;
};

/////////////////////////////////////////////////
Sensor::Sensor()
  : dataPtr(new SensorPrivate)
{
}

/////////////////////////////////////////////////
Sensor::Sensor(const Sensor &_sensor)
  : dataPtr(new SensorPrivate(*_sensor.dataPtr))
{
}

/////////////////////////////////////////////////
Sensor::Sensor(Sensor &&_sensor) noexcept
{
  this->dataPtr = _sensor.dataPtr;
  _sensor.dataPtr = nullptr;
}

/////////////////////////////////////////////////
Sensor::~Sensor()
{
  delete this->dataPtr;
  this->dataPtr = nullptr;
}

/////////////////////////////////////////////////
Sensor &Sensor::operator=(const Sensor &_sensor)
{
  if (!this->dataPtr)
  {
    this->dataPtr = new SensorPrivate(*_sensor.dataPtr);
  }
  else
  {
    this->dataPtr = new(this->dataPtr) SensorPrivate(*_sensor.dataPtr);
  }
  return *this;
}

/////////////////////////////////////////////////
Sensor &Sensor::operator=(Sensor &&_sensor)
{
  this->dataPtr = _sensor.dataPtr;
  _sensor.dataPtr = nullptr;
  return *this;
}

/////////////////////////////////////////////////
bool Sensor::operator==(const Sensor &_sensor) const
{
  // Check a few of the easy parameters.
  if (this->Name() != _sensor.Name() ||
      this->Type() != _sensor.Type() ||
      this->Topic() != _sensor.Topic() ||
      this->Pose() != _sensor.Pose() ||
      this->PoseFrame() != _sensor.PoseFrame() ||
      !ignition::math::equal(this->UpdateRate(), _sensor.UpdateRate()))
  {
    return false;
  }

  // Check the sensors
  switch (this->Type())
  {
    case SensorType::ALTIMETER:
      return *(this->dataPtr->altimeter) == *(_sensor.dataPtr->altimeter);
    case SensorType::MAGNETOMETER:
      return *(this->dataPtr->magnetometer) == *(_sensor.dataPtr->magnetometer);
    case SensorType::AIR_PRESSURE:
      return *(this->dataPtr->airPressure) == *(_sensor.dataPtr->airPressure);
    case SensorType::NONE:
    default:
      return true;
  }
}

/////////////////////////////////////////////////
bool Sensor::operator!=(const Sensor &_sensor) const
{
  return !(*this == _sensor);
}

/////////////////////////////////////////////////
Errors Sensor::Load(ElementPtr _sdf)
{
  Errors errors;

  this->dataPtr->sdf = _sdf;

  // Check that sdf is a valid pointer
  if (!_sdf)
  {
    errors.push_back({ErrorCode::ELEMENT_MISSING,
        "Attempting to load a Sensor, but the provided SDF "
        "element is null."});
    return errors;
  }

  // Check that the provided SDF element is a <sensor>
  // This is an error that cannot be recovered, so return an error.
  if (_sdf->GetName() != "sensor")
  {
    errors.push_back({ErrorCode::ELEMENT_INCORRECT_TYPE,
        "Attempting to load a Sensor, but the provided SDF element is not a "
        "<sensor>."});
    return errors;
  }

  // Read the sensors's name
  if (!loadName(_sdf, this->dataPtr->name))
  {
    errors.push_back({ErrorCode::ATTRIBUTE_MISSING,
                     "A sensor name is required, but the name is not set."});
    return errors;
  }

  this->dataPtr->updateRate = _sdf->Get<double>("update_rate",
      this->dataPtr->updateRate).first;
  this->dataPtr->topic = _sdf->Get<std::string>("topic");
  if (this->dataPtr->topic == "__default__")
    this->dataPtr->topic = "";

  std::string type = _sdf->Get<std::string>("type");
  if (type == "air_pressure")
  {
    this->dataPtr->type = SensorType::AIR_PRESSURE;
    this->dataPtr->airPressure.reset(new AirPressure());
    Errors err = this->dataPtr->airPressure->Load(
        _sdf->GetElement("air_pressure"));
    errors.insert(errors.end(), err.begin(), err.end());
  }
  else if (type == "altimeter")
  {
    this->dataPtr->type = SensorType::ALTIMETER;
    this->dataPtr->altimeter.reset(new Altimeter());
    Errors err = this->dataPtr->altimeter->Load(_sdf->GetElement("altimeter"));
    errors.insert(errors.end(), err.begin(), err.end());
  }
  else if (type == "camera")
  {
    this->dataPtr->type = SensorType::CAMERA;
  }
  else if (type == "contact")
  {
    this->dataPtr->type = SensorType::CONTACT;
  }
  else if (type == "depth" || type == "depth_camera")
  {
    this->dataPtr->type = SensorType::DEPTH_CAMERA;
  }
  else if (type == "force_torque")
  {
    this->dataPtr->type = SensorType::FORCE_TORQUE;
  }
  else if (type == "gps")
  {
    this->dataPtr->type = SensorType::GPS;
  }
  else if (type == "gpu_ray" || type == "gpu_lidar")
  {
    this->dataPtr->type = SensorType::GPU_LIDAR;
  }
  else if (type == "imu")
  {
    this->dataPtr->type = SensorType::IMU;
  }
  else if (type == "logical_camera")
  {
    this->dataPtr->type = SensorType::LOGICAL_CAMERA;
  }
  else if (type == "magnetometer")
  {
    this->dataPtr->type = SensorType::MAGNETOMETER;
    this->dataPtr->magnetometer.reset(new Magnetometer());
    Errors err = this->dataPtr->magnetometer->Load(
        _sdf->GetElement("magnetometer"));
    errors.insert(errors.end(), err.begin(), err.end());
  }
  else if (type == "multicamera")
  {
    this->dataPtr->type = SensorType::MULTICAMERA;
  }
  else if (type == "ray" || type == "lidar")
  {
    this->dataPtr->type = SensorType::LIDAR;
  }
  else if (type == "rfid")
  {
    this->dataPtr->type = SensorType::RFID;
  }
  else if (type == "rfidtag")
  {
    this->dataPtr->type = SensorType::RFIDTAG;
  }
  else if (type == "sonar")
  {
    this->dataPtr->type = SensorType::SONAR;
  }
  else if (type == "wireless_receiver")
  {
    this->dataPtr->type = SensorType::WIRELESS_RECEIVER;
  }
  else if (type == "wireless_transmitter")
  {
    this->dataPtr->type = SensorType::WIRELESS_TRANSMITTER;
  }
  else
  {
    errors.push_back({ErrorCode::ATTRIBUTE_INVALID,
        "Attempting to load a Sensor, but the provided sensor type is missing "
        "or invalid."});
    return errors;
  }

  // Load the pose. Ignore the return value since the sensor pose is optional.
  loadPose(_sdf, this->dataPtr->pose, this->dataPtr->poseFrame);

  return errors;
}

/////////////////////////////////////////////////
std::string Sensor::Name() const
{
  return this->dataPtr->name;
}

/////////////////////////////////////////////////
void Sensor::SetName(const std::string &_name)
{
  this->dataPtr->name = _name;
}

/////////////////////////////////////////////////
std::string Sensor::Topic() const
{
  return this->dataPtr->topic;
}

/////////////////////////////////////////////////
void Sensor::SetTopic(const std::string &_topic)
{
  this->dataPtr->topic = _topic;
}

/////////////////////////////////////////////////
const ignition::math::Pose3d &Sensor::Pose() const
{
  return this->dataPtr->pose;
}

/////////////////////////////////////////////////
const std::string &Sensor::PoseFrame() const
{
  return this->dataPtr->poseFrame;
}

/////////////////////////////////////////////////
void Sensor::SetPose(const ignition::math::Pose3d &_pose)
{
  this->dataPtr->pose = _pose;
}

/////////////////////////////////////////////////
void Sensor::SetPoseFrame(const std::string &_frame)
{
  this->dataPtr->poseFrame = _frame;
}

/////////////////////////////////////////////////
sdf::ElementPtr Sensor::Element() const
{
  return this->dataPtr->sdf;
}

/////////////////////////////////////////////////
SensorType Sensor::Type() const
{
  return this->dataPtr->type;
}

/////////////////////////////////////////////////
void Sensor::SetType(const SensorType _type)
{
  this->dataPtr->type = _type;
}

/////////////////////////////////////////////////
bool Sensor::SetType(const std::string &_typeStr)
{
  for (size_t i = 0; i < sensorTypeStrs.size(); ++i)
  {
    if (_typeStr == sensorTypeStrs[i])
    {
      this->dataPtr->type = static_cast<SensorType>(i);
      return true;
    }
  }
  return false;
}

/////////////////////////////////////////////////
const Magnetometer *Sensor::MagnetometerSensor() const
{
  return this->dataPtr->magnetometer.get();
}

/////////////////////////////////////////////////
void Sensor::SetMagnetometerSensor(const Magnetometer &_mag)
{
  this->dataPtr->magnetometer = std::make_unique<Magnetometer>(_mag);
}

/////////////////////////////////////////////////
const Altimeter *Sensor::AltimeterSensor() const
{
  return this->dataPtr->altimeter.get();
}

/////////////////////////////////////////////////
void Sensor::SetAltimeterSensor(const Altimeter &_alt)
{
  this->dataPtr->altimeter = std::make_unique<Altimeter>(_alt);
}

/////////////////////////////////////////////////
const AirPressure *Sensor::AirPressureSensor() const
{
  return this->dataPtr->airPressure.get();
}

/////////////////////////////////////////////////
void Sensor::SetAirPressureSensor(const AirPressure &_air)
{
  this->dataPtr->airPressure = std::make_unique<AirPressure>(_air);
}

/////////////////////////////////////////////////
double Sensor::UpdateRate() const
{
  return this->dataPtr->updateRate;
}

/////////////////////////////////////////////////
void Sensor::SetUpdateRate(double _hz)
{
  this->dataPtr->updateRate = _hz;
}

/////////////////////////////////////////////////
std::string Sensor::TypeStr() const
{
  size_t index = static_cast<int>(this->dataPtr->type);
  if (index > 0 && index < sensorTypeStrs.size())
    return sensorTypeStrs[index];
  return "none";
}