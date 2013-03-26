/*
 * Copyright 2012 Open Source Robotics Foundation
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
#include <urdf_model/model.h>
#include <urdf_model/link.h>
#include <urdf_parser/urdf_parser.h>

#include <fstream>
#include <sstream>
#include <algorithm>
#include <string>

#include "sdf/parser_urdf.hh"
#include "sdf/sdf.hh"

using namespace sdf;

typedef boost::shared_ptr<urdf::Collision> UrdfCollisionPtr;
typedef boost::shared_ptr<urdf::Visual> UrdfVisualPtr;
typedef boost::shared_ptr<urdf::Link> UrdfLinkPtr;
typedef boost::shared_ptr<const urdf::Link> ConstUrdfLinkPtr;

/// create SDF geometry block based on URDF
std::map<std::string, std::vector<SDFExtension*> > g_extensions;
bool g_reduceFixedJoints;
bool g_enforceLimits;

/// \brief parser xml string into urdf::Vector3
/// \param[in] _key XML key where vector3 value might be
/// \param[in] _scale scalar scale for the vector3
/// \return a urdf::Vector3
urdf::Vector3 ParseVector3(TiXmlNode* _key, double _scale = 1.0);

/// insert extensions into collision geoms
void InsertSDFExtensionCollision(TiXmlElement *_elem,
    const std::string &_linkName);

/// insert extensions into model
void InsertSDFExtensionRobot(TiXmlElement *_elem);

/// insert extensions into visuals
void InsertSDFExtensionVisual(TiXmlElement *_elem,
    const std::string &_linkName);


/// insert extensions into joints
void InsertSDFExtensionJoint(TiXmlElement *_elem,
    const std::string &_jointName);

/// reduced fixed joints:  apply transform reduction for ray sensors
///   in extensions when doing fixed joint reduction
void ReduceSDFExtensionSensorTransformReduction(
      std::vector<TiXmlElement*>::iterator _blobIt,
      sdf::Pose _reductionTransform);

/// reduced fixed joints:  apply transform reduction for projectors in
///   extensions when doing fixed joint reduction
void ReduceSDFExtensionProjectorTransformReduction(
      std::vector<TiXmlElement*>::iterator _blobIt,
      sdf::Pose _reductionTransform);


/// reduced fixed joints:  apply transform reduction to extensions
///   when doing fixed joint reduction
void ReduceSDFExtensionsTransform(SDFExtension* _ge);

/// reduce fixed joints:  lump joints to parent link
void ReduceJointsToParent(UrdfLinkPtr _link);

/// reduce fixed joints:  lump collisions to parent link
void ReduceCollisionsToParent(UrdfLinkPtr _link);

/// reduce fixed joints:  lump visuals to parent link
void ReduceVisualsToParent(UrdfLinkPtr _link);

/// reduce fixed joints:  lump inertial to parent link
void ReduceInertialToParent(UrdfLinkPtr /*_link*/);

/// create SDF Collision block based on URDF
void CreateCollision(TiXmlElement* _elem, ConstUrdfLinkPtr _link,
      UrdfCollisionPtr _collision,
      const std::string &_oldLinkName = std::string(""));

/// create SDF Visual block based on URDF
void CreateVisual(TiXmlElement *_elem, ConstUrdfLinkPtr _link,
      UrdfVisualPtr _visual, const std::string &_oldLinkName = std::string(""));

/// create SDF Joint block based on URDF
void CreateJoint(TiXmlElement *_root, ConstUrdfLinkPtr _link,
      sdf::Pose &_currentTransform);

/// insert extensions into links
void InsertSDFExtensionLink(TiXmlElement *_elem, const std::string &_linkName);

/// create visual blocks from urdf visuals
void CreateVisuals(TiXmlElement* _elem, ConstUrdfLinkPtr _link);

/// create collision blocks from urdf collisions
void CreateCollisions(TiXmlElement* _elem, ConstUrdfLinkPtr _link);

/// create SDF Inertial block based on URDF
void CreateInertial(TiXmlElement *_elem, ConstUrdfLinkPtr _link);

/// append transform (pose) to the end of the xml element
void AddTransform(TiXmlElement *_elem, const sdf::Pose &_transform);

/// create SDF from URDF link
void CreateSDF(TiXmlElement *_root, ConstUrdfLinkPtr _link,
      const sdf::Pose &_transform);

/// create SDF Link block based on URDF
void CreateLink(TiXmlElement *_root, ConstUrdfLinkPtr _link,
      sdf::Pose &_currentTransform);


/// print collision groups for debugging purposes
void PrintCollisionGroups(UrdfLinkPtr _link);

/// reduced fixed joints:  apply appropriate frame updates in joint
///   inside urdf extensions when doing fixed joint reduction
void ReduceSDFExtensionJointFrameReplace(
    std::vector<TiXmlElement*>::iterator _blobIt, UrdfLinkPtr _link);

/// reduced fixed joints:  apply appropriate frame updates in gripper
///   inside urdf extensions when doing fixed joint reduction
void ReduceSDFExtensionGripperFrameReplace(
    std::vector<TiXmlElement*>::iterator _blobIt, UrdfLinkPtr _link);

/// reduced fixed joints:  apply appropriate frame updates in projector
/// inside urdf extensions when doing fixed joint reduction
void ReduceSDFExtensionProjectorFrameReplace(
    std::vector<TiXmlElement*>::iterator _blobIt, UrdfLinkPtr _link);

/// reduced fixed joints:  apply appropriate frame updates in plugins
///   inside urdf extensions when doing fixed joint reduction
void ReduceSDFExtensionPluginFrameReplace(
      std::vector<TiXmlElement*>::iterator _blobIt,
      UrdfLinkPtr _link, const std::string &_pluginName,
      const std::string &_elementName, sdf::Pose _reductionTransform);

/// reduced fixed joints:  apply appropriate frame updates in urdf
///   extensions when doing fixed joint reduction
void ReduceSDFExtensionContactSensorFrameReplace(
    std::vector<TiXmlElement*>::iterator _blobIt, UrdfLinkPtr _link);

/// \brief reduced fixed joints:  apply appropriate updates to urdf
///   extensions when doing fixed joint reduction
///
/// Take the link's existing list of gazebo extensions, transfer them
/// into parent link.  Along the way, update local transforms by adding
/// the additional transform to parent.  Also, look through all
/// referenced link names with plugins and update references to current
/// link to the parent link. (ReduceSDFExtensionFrameReplace())
///
/// \param[in] _link pointer to urdf link, its extensions will be reduced
void ReduceSDFExtensionToParent(UrdfLinkPtr _link);

/// reduced fixed joints:  apply appropriate frame updates
///   in urdf extensions when doing fixed joint reduction
void ReduceSDFExtensionFrameReplace(SDFExtension* _ge, UrdfLinkPtr _link);


/// get value from <key value="..."/> pair and return it as string
std::string GetKeyValueAsString(TiXmlElement* _elem);


/// \brief append key value pair to the end of the xml element
/// \param[in] _elem pointer to xml element
/// \param[in] _key string containing key to add to xml element
/// \param[in] _value string containing value for the key added
void AddKeyValue(TiXmlElement *_elem, const std::string &_key,
                     const std::string &_value);

/// \brief convert values to string
/// \param[in] _count number of values in _values array
/// \param[in] _values array of double values
/// \return a string
std::string Values2str(unsigned int _count, const double *_values);


void CreateGeometry(TiXmlElement* _elem,
    boost::shared_ptr<urdf::Geometry> _geometry);

std::string GetGeometryBoundingBox(boost::shared_ptr<urdf::Geometry> _geometry,
    double *_sizeVals);

sdf::Pose inverseTransformToParentFrame(sdf::Pose _transformInLinkFrame,
    urdf::Pose _parentToLinkTransform);

/// reduced fixed joints: transform to parent frame
urdf::Pose TransformToParentFrame(urdf::Pose _transformInLinkFrame,
    urdf::Pose _parentToLinkTransform);

/// reduced fixed joints: transform to parent frame
sdf::Pose TransformToParentFrame(sdf::Pose _transformInLinkFrame,
    urdf::Pose _parentToLinkTransform);

/// reduced fixed joints: transform to parent frame
sdf::Pose TransformToParentFrame(sdf::Pose _transformInLinkFrame,
    sdf::Pose _parentToLinkTransform);

/// reduced fixed joints: utility to copy between urdf::Pose and
///   math::Pose
sdf::Pose CopyPose(urdf::Pose _pose);

/// reduced fixed joints: utility to copy between urdf::Pose and
///   math::Pose
urdf::Pose CopyPose(sdf::Pose _pose);

/////////////////////////////////////////////////
urdf::Vector3 ParseVector3(TiXmlNode* _key, double _scale)
{
  if (_key != NULL)
  {
    std::string str = _key->Value();
    std::vector<std::string> pieces;
    std::vector<double> vals;

    boost::split(pieces, str, boost::is_any_of(" "));
    for (unsigned int i = 0; i < pieces.size(); ++i)
    {
      if (pieces[i] != "")
      {
        try
        {
          vals.push_back(_scale
              * boost::lexical_cast<double>(pieces[i].c_str()));
        }
        catch(boost::bad_lexical_cast &e)
        {
          sdferr << "xml key [" << str
            << "][" << i << "] value [" << pieces[i]
            << "] is not a valid double from a 3-tuple\n";
          return urdf::Vector3(0, 0, 0);
        }
      }
    }
    return urdf::Vector3(vals[0], vals[1], vals[3]);
  }
  else
    return urdf::Vector3(0, 0, 0);
}

/////////////////////////////////////////////////
/// \brief convert Vector3 to string
/// \param[in] _vector a urdf::Vector3
/// \return a string
std::string Vector32Str(const urdf::Vector3 _vector)
{
  std::stringstream ss;
  ss << _vector.x;
  ss << " ";
  ss << _vector.y;
  ss << " ";
  ss << _vector.z;
  return ss.str();
}

/////////////////////////////////////////////////
/// print mass for link for debugging
void PrintMass(const std::string &/*_linkName*/, sdf::Mass /*_mass*/)
{
  // \TODO
  /*  sdfdbg << "LINK NAME: [" << _linkName << "] from dMass\n";
      sdfdbg << "     MASS: [" << _mass.mass << "]\n";
      sdfdbg << "       CG: [" << _mass.c[0] << ", " << _mass.c[1] << ", "
      << _mass.c[2] << "]\n";
      sdfdbg << "        I: [" << _mass.I[0] << ", " << _mass.I[1] << ", "
      << _mass.I[2] << "]\n";
      sdfdbg << "           [" << _mass.I[4] << ", " << _mass.I[5] << ", "
      << _mass.I[6] << "]\n";
      sdfdbg << "           [" << _mass.I[8] << ", " << _mass.I[9] << ", "
      << _mass.I[10] << "]\n";
      */
}

/////////////////////////////////////////////////
/// print mass for link for debugging
void PrintMass(UrdfLinkPtr _link)
{
  sdfdbg << "LINK NAME: [" << _link->name << "] from dMass\n";
  sdfdbg << "     MASS: [" << _link->inertial->mass << "]\n";
  sdfdbg << "       CG: [" << _link->inertial->origin.position.x << ", "
    << _link->inertial->origin.position.y << ", "
    << _link->inertial->origin.position.z << "]\n";
  sdfdbg << "        I: [" << _link->inertial->ixx << ", "
    << _link->inertial->ixy << ", "
    << _link->inertial->ixz << "]\n";
  sdfdbg << "           [" << _link->inertial->ixy << ", "
    << _link->inertial->iyy << ", "
    << _link->inertial->iyz << "]\n";
  sdfdbg << "           [" << _link->inertial->ixz << ", "
    << _link->inertial->iyz << ", "
    << _link->inertial->izz << "]\n";
}

////////////////////////////////////////////////////////////////////////////////
void ReduceCollisionToParent(UrdfLinkPtr _link,
    const std::string &_groupName, UrdfCollisionPtr _collision)
{
  boost::shared_ptr<std::vector<UrdfCollisionPtr> >
    cols = _link->getCollisions(_groupName);
  if (!cols)
  {
    // group does not exist, create one and add to map
    cols.reset(new std::vector<UrdfCollisionPtr>);
    // new group name, create add vector to map and add Collision to the vector
    _link->collision_groups.insert(make_pair(_groupName, cols));
  }

  // group exists, add Collision to the vector in the map
  std::vector<UrdfCollisionPtr>::iterator colIt =
    find(cols->begin(), cols->end(), _collision);
  if (colIt != cols->end())
    sdfwarn << "attempted to add collision to link ["
      << _link->name
      << "], but it already exists under group ["
      << _groupName << "]\n";
  else
    cols->push_back(_collision);
}

////////////////////////////////////////////////////////////////////////////////
void ReduceVisualToParent(UrdfLinkPtr _link,
    const std::string &_groupName, UrdfVisualPtr _visual)
{
  boost::shared_ptr<std::vector<UrdfVisualPtr> > viss
    = _link->getVisuals(_groupName);
  if (!viss)
  {
    // group does not exist, create one and add to map
    viss.reset(new std::vector<UrdfVisualPtr>);
    // new group name, create vector, add vector to map and
    //   add Visual to the vector
    _link->visual_groups.insert(make_pair(_groupName, viss));
    // sdfdbg << "successfully added a new visual group name ["
    //       << _groupName << "]\n";
  }

  // group exists, add Visual to the vector in the map if it's not there
  std::vector<UrdfVisualPtr>::iterator visIt
    = find(viss->begin(), viss->end(), _visual);
  if (visIt != viss->end())
    sdfwarn << "attempted to add visual to link ["
      << _link->name
      << "], but it already exists under group ["
      << _groupName << "]\n";
  else
    viss->push_back(_visual);
}

////////////////////////////////////////////////////////////////////////////////
/// reduce fixed joints by lumping inertial, visual and
// collision elements of the child link into the parent link
void ReduceFixedJoints(TiXmlElement *_root, UrdfLinkPtr _link)
{
  // if child is attached to self by fixed _link first go up the tree,
  //   check it's children recursively
  for (unsigned int i = 0 ; i < _link->child_links.size() ; ++i)
    if (_link->child_links[i]->parent_joint->type == urdf::Joint::FIXED)
      ReduceFixedJoints(_root, _link->child_links[i]);

  // reduce this _link's stuff up the tree to parent but skip first joint
  //   if it's the world
  if (_link->getParent() && _link->getParent()->name != "world" &&
      _link->parent_joint && _link->parent_joint->type == urdf::Joint::FIXED)
  {
    // sdfdbg << "Fixed Joint Reduction: extension lumping from ["
    //       << _link->name << "] to [" << _link->getParent()->name << "]\n";

    // lump sdf extensions to parent, (give them new reference _link names)
    ReduceSDFExtensionToParent(_link);

    // reduce _link elements to parent
    ReduceInertialToParent(_link);
    ReduceVisualsToParent(_link);
    ReduceCollisionsToParent(_link);
    ReduceJointsToParent(_link);
  }

  // continue down the tree for non-fixed joints
  for (unsigned int i = 0 ; i < _link->child_links.size() ; ++i)
    if (_link->child_links[i]->parent_joint->type != urdf::Joint::FIXED)
      ReduceFixedJoints(_root, _link->child_links[i]);
}

/////////////////////////////////////////////////
/// reduce fixed joints:  lump inertial to parent link
void ReduceInertialToParent(UrdfLinkPtr /*_link*/)
{
  /// \TODO
  /*
  // sdfdbg << "TREE:   mass lumping from [" << link->name
  //      << "] to [" << _link->getParent()->name << "]\n.";
  // now lump all contents of this _link to parent
  if (_link->inertial)
  {
    // get parent mass (in parent link frame)
    dMass parentMass;
    if (!_link->getParent()->inertial)
      _link->getParent()->inertial.reset(new urdf::Inertial);
    dMassSetParameters(&parentMass, _link->getParent()->inertial->mass,
        _link->getParent()->inertial->origin.position.x,
        _link->getParent()->inertial->origin.position.y,
        _link->getParent()->inertial->origin.position.z,
        _link->getParent()->inertial->ixx, _link->getParent()->inertial->iyy,
        _link->getParent()->inertial->izz, _link->getParent()->inertial->ixy,
        _link->getParent()->inertial->ixz, _link->getParent()->inertial->iyz);
    // PrintMass(_link->getParent()->name, parentMass);
    // PrintMass(_link->getParent());
    // set _link mass (in _link frame)
    dMass linkMass;
    dMassSetParameters(&linkMass, _link->inertial->mass,
        _link->inertial->origin.position.x,
        _link->inertial->origin.position.y,
        _link->inertial->origin.position.z,
        _link->inertial->ixx, _link->inertial->iyy, _link->inertial->izz,
        _link->inertial->ixy, _link->inertial->ixz, _link->inertial->iyz);
    // PrintMass(_link->name, linkMass);
    // PrintMass(_link);
    // un-rotate _link mass into parent link frame
    dMatrix3 R;
    double phi, theta, psi;
    _link->parent_joint->parent_to_joint_origin_transform.rotation.getRPY(
        phi, theta, psi);
    dRFromEulerAngles(R, phi, theta, psi);
    dMassRotate(&linkMass, R);
    // PrintMass(_link->name, linkMass);
    // un-translate _link mass into parent link frame
    dMassTranslate(&linkMass,
        _link->parent_joint->parent_to_joint_origin_transform.position.x,
        _link->parent_joint->parent_to_joint_origin_transform.position.y,
        _link->parent_joint->parent_to_joint_origin_transform.position.z);
    // PrintMass(_link->name, linkMass);
    // now linkMass is in the parent frame, add linkMass to parentMass
    dMassAdd(&parentMass, &linkMass);
    // PrintMass(_link->getParent()->name, parentMass);
    // update parent mass
    _link->getParent()->inertial->mass = parentMass.mass;
    _link->getParent()->inertial->ixx  = parentMass.I[0+4*0];
    _link->getParent()->inertial->iyy  = parentMass.I[1+4*1];
    _link->getParent()->inertial->izz  = parentMass.I[2+4*2];
    _link->getParent()->inertial->ixy  = parentMass.I[0+4*1];
    _link->getParent()->inertial->ixz  = parentMass.I[0+4*2];
    _link->getParent()->inertial->iyz  = parentMass.I[1+4*2];
    _link->getParent()->inertial->origin.position.x  = parentMass.c[0];
    _link->getParent()->inertial->origin.position.y  = parentMass.c[1];
    _link->getParent()->inertial->origin.position.z  = parentMass.c[2];
    // PrintMass(_link->getParent());
  }
  */
}

/////////////////////////////////////////////////
/// reduce fixed joints:  lump visuals to parent link
void ReduceVisualsToParent(UrdfLinkPtr _link)
{
  // lump visual to parent
  // lump all visual to parent, assign group name
  // "lump::"+group name+"::'+_link name
  // lump but keep the _link name in(/as) the group name,
  // so we can correlate visuals to visuals somehow.
  for (std::map<std::string,
      boost::shared_ptr<std::vector<UrdfVisualPtr> > >::iterator
      visualsIt = _link->visual_groups.begin();
      visualsIt != _link->visual_groups.end(); ++visualsIt)
  {
    if (visualsIt->first.find(std::string("lump::")) == 0)
    {
      // it's a previously lumped mesh, re-lump under same _groupName
      std::string lumpGroupName = visualsIt->first;
      // sdfdbg << "re-lumping group name [" << lumpGroupName
      //       << "] to link [" << _link->getParent()->name << "]\n";
      for (std::vector<UrdfVisualPtr>::iterator
          visualIt = visualsIt->second->begin();
          visualIt != visualsIt->second->end(); ++visualIt)
      {
        // transform visual origin from _link frame to parent link
        // frame before adding to parent
        (*visualIt)->origin = TransformToParentFrame((*visualIt)->origin,
            _link->parent_joint->parent_to_joint_origin_transform);
        // add the modified visual to parent
        ReduceVisualToParent(_link->getParent(), lumpGroupName,
            *visualIt);
      }
    }
    else
    {
      // default and any other groups meshes
      std::string lumpGroupName = std::string("lump::")+_link->name;
      // sdfdbg << "adding modified lump group name [" << lumpGroupName
      //       << "] to link [" << _link->getParent()->name << "]\n.";
      for (std::vector<UrdfVisualPtr>::iterator
          visualIt = visualsIt->second->begin();
          visualIt != visualsIt->second->end(); ++visualIt)
      {
        // transform visual origin from _link frame to
        // parent link frame before adding to parent
        (*visualIt)->origin = TransformToParentFrame((*visualIt)->origin,
            _link->parent_joint->parent_to_joint_origin_transform);
        // add the modified visual to parent
        ReduceVisualToParent(_link->getParent(), lumpGroupName,
            *visualIt);
      }
    }
  }
}

/////////////////////////////////////////////////
/// reduce fixed joints:  lump collisions to parent link
void ReduceCollisionsToParent(UrdfLinkPtr _link)
{
  // lump collision parent
  // lump all collision to parent, assign group name
  // "lump::"+group name+"::'+_link name
  // lump but keep the _link name in(/as) the group name,
  // so we can correlate visuals to collisions somehow.
  for (std::map<std::string,
      boost::shared_ptr<std::vector<UrdfCollisionPtr> > >::iterator
      collisionsIt = _link->collision_groups.begin();
      collisionsIt != _link->collision_groups.end(); ++collisionsIt)
  {
    if (collisionsIt->first.find(std::string("lump::")) == 0)
    {
      // if it's a previously lumped mesh, relump under same _groupName
      std::string lumpGroupName = collisionsIt->first;
      // sdfdbg << "re-lumping collision [" << collisionsIt->first
      //       << "] for link [" << _link->name
      //       << "] to parent [" << _link->getParent()->name
      //       << "] with group name [" << lumpGroupName << "]\n";
      for (std::vector<UrdfCollisionPtr>::iterator
          collisionIt = collisionsIt->second->begin();
          collisionIt != collisionsIt->second->end(); ++collisionIt)
      {
        // transform collision origin from _link frame to
        // parent link frame before adding to parent
        (*collisionIt)->origin = TransformToParentFrame(
            (*collisionIt)->origin,
            _link->parent_joint->parent_to_joint_origin_transform);
        // add the modified collision to parent
        ReduceCollisionToParent(_link->getParent(), lumpGroupName,
            *collisionIt);
      }
    }
    else
    {
      // default and any other group meshes
      std::string lumpGroupName = std::string("lump::")+_link->name;
      // sdfdbg << "lumping collision [" << collisionsIt->first
      //       << "] for link [" << _link->name
      //       << "] to parent [" << _link->getParent()->name
      //       << "] with group name [" << lumpGroupName << "]\n";
      for (std::vector<UrdfCollisionPtr>::iterator
          collisionIt = collisionsIt->second->begin();
          collisionIt != collisionsIt->second->end(); ++collisionIt)
      {
        // transform collision origin from _link frame to
        // parent link frame before adding to parent
        (*collisionIt)->origin = TransformToParentFrame(
            (*collisionIt)->origin,
            _link->parent_joint->parent_to_joint_origin_transform);

        // add the modified collision to parent
        ReduceCollisionToParent(_link->getParent(), lumpGroupName,
            *collisionIt);
      }
    }
  }
  // this->PrintCollisionGroups(_link->getParent());
}

/////////////////////////////////////////////////
/// reduce fixed joints:  lump joints to parent link
void ReduceJointsToParent(UrdfLinkPtr _link)
{
  // set child link's parentJoint's parent link to
  // a parent link up stream that does not have a fixed parentJoint
  for (unsigned int i = 0 ; i < _link->child_links.size() ; ++i)
  {
    boost::shared_ptr<urdf::Joint> parentJoint =
      _link->child_links[i]->parent_joint;
    if (parentJoint->type != urdf::Joint::FIXED)
    {
      // go down the tree until we hit a parent joint that is not fixed
      UrdfLinkPtr newParentLink = _link;
      sdf::Pose jointAnchorTransform;
      while (newParentLink->parent_joint &&
          newParentLink->getParent()->name != "world" &&
          newParentLink->parent_joint->type == urdf::Joint::FIXED)
      {
        jointAnchorTransform = jointAnchorTransform * jointAnchorTransform;
        parentJoint->parent_to_joint_origin_transform =
          TransformToParentFrame(
              parentJoint->parent_to_joint_origin_transform,
              newParentLink->parent_joint->parent_to_joint_origin_transform);
        newParentLink = newParentLink->getParent();
      }
      // now set the _link->child_links[i]->parent_joint's parent link to
      // the newParentLink
      _link->child_links[i]->setParent(newParentLink);
      parentJoint->parent_link_name = newParentLink->name;
      // and set the _link->child_links[i]->parent_joint's
      // parent_to_joint_origin_transform as the aggregated anchor transform?
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
std::string lowerStr(std::string str)
{
  std::string out = str;
  std::transform(out.begin(), out.end(), out.begin(), ::tolower);
  return out;
}

////////////////////////////////////////////////////////////////////////////////
URDF2SDF::URDF2SDF()
{
  // default options
  g_enforceLimits = true;
  g_reduceFixedJoints = true;
}

////////////////////////////////////////////////////////////////////////////////
URDF2SDF::~URDF2SDF()
{
}



////////////////////////////////////////////////////////////////////////////////
std::string Values2str(unsigned int _count, const double *_values)
{
  std::stringstream ss;
  for (unsigned int i = 0 ; i < _count ; ++i)
  {
    if (i > 0)
      ss << " ";
    ss << _values[i];
  }
  return ss.str();
}

////////////////////////////////////////////////////////////////////////////////
void AddKeyValue(TiXmlElement *_elem, const std::string &_key,
    const std::string &_value)
{
  TiXmlElement* childElem = _elem->FirstChildElement(_key);
  if (childElem)
  {
    std::string oldValue = GetKeyValueAsString(childElem);
    if (oldValue != _value)
      sdfwarn << "multiple inconsistent <" << _key
        << "> exists due to fixed joint reduction"
        << " overwriting previous value [" << oldValue
        << "] with [" << _value << "].\n";
    // else
    //   sdfdbg << "multiple consistent <" << _key
    //          << "> exists with [" << _value
    //          << "] due to fixed joint reduction.\n";
    _elem->RemoveChild(childElem);  // remove old _elem
  }

  TiXmlElement *ekey = new TiXmlElement(_key);
  TiXmlText *textEkey = new TiXmlText(_value);
  ekey->LinkEndChild(textEkey);
  _elem->LinkEndChild(ekey);
}

////////////////////////////////////////////////////////////////////////////////
void AddTransform(TiXmlElement *_elem, const sdf::Pose &_transform)
{
  sdf::Vector3 e = _transform.rot.GetAsEuler();
  double cpose[6] = { _transform.pos.x, _transform.pos.y,
    _transform.pos.z, e.x, e.y, e.z };

  /* set geometry transform */
  AddKeyValue(_elem, "pose", Values2str(6, cpose));
}

////////////////////////////////////////////////////////////////////////////////
std::string GetKeyValueAsString(TiXmlElement* _elem)
{
  std::string valueStr;
  if (_elem->Attribute("value"))
  {
    valueStr = _elem->Attribute("value");
  }
  else if (_elem->FirstChild())
    /// @todo: FIXME: comment out check for now, different tinyxml
    /// versions fails to compile:
    //  && _elem->FirstChild()->Type() == TiXmlNode::TINYXML_TEXT)
  {
    valueStr = _elem->FirstChild()->ValueStr();
  }
  return valueStr;
}

////////////////////////////////////////////////////////////////////////////////
void URDF2SDF::ParseSDFExtension(TiXmlDocument &_urdfXml)
{
  TiXmlElement* robotXml = _urdfXml.FirstChildElement("robot");

  // Get all SDF extension elements, put everything in
  //   g_extensions map, containing a key string
  //   (link/joint name) and values
  for (TiXmlElement* sdfXml = robotXml->FirstChildElement("sdf");
      sdfXml; sdfXml = sdfXml->NextSiblingElement("sdf"))
  {
    const char* ref = sdfXml->Attribute("reference");
    std::string refStr;
    if (!ref)
    {
      // copy extensions for robot (outside of link/joint)
      refStr.clear();
    }
    else
    {
      // copy extensions for link/joint
      refStr = std::string(ref);
    }

    if (g_extensions.find(refStr) ==
        g_extensions.end())
    {
      // create extension map for reference
      std::vector<SDFExtension*> ge;
      g_extensions.insert(std::make_pair(refStr, ge));
    }

    // create and insert a new SDFExtension into the map
    SDFExtension* sdf = new SDFExtension();

    // begin parsing xml node
    for (TiXmlElement *childElem = sdfXml->FirstChildElement();
        childElem; childElem = childElem->NextSiblingElement())
    {
      sdf->oldLinkName = refStr;

      // go through all elements of the extension,
      //   extract what we know, and save the rest in blobs
      // @todo:  somehow use sdf definitions here instead of hard coded
      //         objects

      // material
      if (childElem->ValueStr() == "material")
      {
        sdf->material = GetKeyValueAsString(childElem);
      }
      else if (childElem->ValueStr() == "static")
      {
        std::string valueStr = GetKeyValueAsString(childElem);

        // default of setting static flag is false
        if (lowerStr(valueStr) == "true" || lowerStr(valueStr) == "yes" ||
            valueStr == "1")
          sdf->setStaticFlag = true;
        else
          sdf->setStaticFlag = false;
      }
      else if (childElem->ValueStr() == "gravity")
      {
        std::string valueStr = GetKeyValueAsString(childElem);

        // default of gravity is true
        if (lowerStr(valueStr) == "false" || lowerStr(valueStr) == "no" ||
            valueStr == "0")
          sdf->gravity = false;
        else
          sdf->gravity = true;
      }
      else if (childElem->ValueStr() == "dampingFactor")
      {
        sdf->isDampingFactor = true;
        sdf->dampingFactor = boost::lexical_cast<double>(
            GetKeyValueAsString(childElem).c_str());
      }
      else if (childElem->ValueStr() == "maxVel")
      {
        sdf->isMaxVel = true;
        sdf->maxVel = boost::lexical_cast<double>(
            GetKeyValueAsString(childElem).c_str());
      }
      else if (childElem->ValueStr() == "minDepth")
      {
        sdf->isMinDepth = true;
        sdf->minDepth = boost::lexical_cast<double>(
            GetKeyValueAsString(childElem).c_str());
      }
      else if (childElem->ValueStr() == "mu1")
      {
        sdf->isMu1 = true;
        sdf->mu1 = boost::lexical_cast<double>(
            GetKeyValueAsString(childElem).c_str());
      }
      else if (childElem->ValueStr() == "mu2")
      {
        sdf->isMu2 = true;
        sdf->mu2 = boost::lexical_cast<double>(
            GetKeyValueAsString(childElem).c_str());
      }
      else if (childElem->ValueStr() == "fdir1")
      {
        sdf->fdir1 = GetKeyValueAsString(childElem);
      }
      else if (childElem->ValueStr() == "kp")
      {
        sdf->isKp = true;
        sdf->kp = boost::lexical_cast<double>(
            GetKeyValueAsString(childElem).c_str());
      }
      else if (childElem->ValueStr() == "kd")
      {
        sdf->isKd = true;
        sdf->kd = boost::lexical_cast<double>(
            GetKeyValueAsString(childElem).c_str());
      }
      else if (childElem->ValueStr() == "selfCollide")
      {
        std::string valueStr = GetKeyValueAsString(childElem);

        // default of selfCollide is false
        if (lowerStr(valueStr) == "true" || lowerStr(valueStr) == "yes" ||
            valueStr == "1")
          sdf->selfCollide = true;
        else
          sdf->selfCollide = false;
      }
      else if (childElem->ValueStr() == "laserRetro")
      {
        sdf->isLaserRetro = true;
        sdf->laserRetro = boost::lexical_cast<double>(
            GetKeyValueAsString(childElem).c_str());
      }
      else if (childElem->ValueStr() == "stopCfm")
      {
        sdf->isStopCfm = true;
        sdf->stopCfm = boost::lexical_cast<double>(
            GetKeyValueAsString(childElem).c_str());
      }
      else if (childElem->ValueStr() == "stopErp")
      {
        sdf->isStopErp = true;
        sdf->stopErp = boost::lexical_cast<double>(
            GetKeyValueAsString(childElem).c_str());
      }
      else if (childElem->ValueStr() == "initialJointPosition")
      {
        sdf->isInitialJointPosition = true;
        sdf->initialJointPosition = boost::lexical_cast<double>(
            GetKeyValueAsString(childElem).c_str());
      }
      else if (childElem->ValueStr() == "fudgeFactor")
      {
        sdf->isFudgeFactor = true;
        sdf->fudgeFactor = boost::lexical_cast<double>(
            GetKeyValueAsString(childElem).c_str());
      }
      else if (childElem->ValueStr() == "provideFeedback")
      {
        std::string valueStr = GetKeyValueAsString(childElem);

        if (lowerStr(valueStr) == "true" || lowerStr(valueStr) == "yes" ||
            valueStr == "1")
          sdf->provideFeedback = true;
        else
          sdf->provideFeedback = false;
      }
      else if (childElem->ValueStr() == "cfmDamping")
      {
        std::string valueStr = GetKeyValueAsString(childElem);

        if (lowerStr(valueStr) == "true" || lowerStr(valueStr) == "yes" ||
            valueStr == "1")
          sdf->cfmDamping = true;
        else
          sdf->cfmDamping = false;
      }
      else
      {
        std::ostringstream stream;
        stream << *childElem;
        // save all unknown stuff in a vector of blobs
        TiXmlElement *blob = new TiXmlElement(*childElem);
        sdf->blobs.push_back(blob);
      }
    }

    // insert into my map
    (g_extensions.find(refStr))->second.push_back(sdf);
  }
}

////////////////////////////////////////////////////////////////////////////////
void InsertSDFExtensionCollision(TiXmlElement *_elem,
    const std::string &_linkName)
{
  for (std::map<std::string, std::vector<SDFExtension*> >::iterator
      sdfIt = g_extensions.begin();
      sdfIt != g_extensions.end(); ++sdfIt)
  {
    for (std::vector<SDFExtension*>::iterator ge = sdfIt->second.begin();
        ge != sdfIt->second.end(); ++ge)
    {
      if ((*ge)->oldLinkName == _linkName)
      {
        TiXmlElement *surface = new TiXmlElement("surface");
        TiXmlElement *friction = new TiXmlElement("friction");
        TiXmlElement *frictionOde = new TiXmlElement("ode");
        TiXmlElement *contact = new TiXmlElement("contact");
        TiXmlElement *contactOde = new TiXmlElement("ode");

        // insert mu1, mu2, kp, kd for collision
        if ((*ge)->isMu1)
          AddKeyValue(frictionOde, "mu",
              Values2str(1, &(*ge)->mu1));
        if ((*ge)->isMu2)
          AddKeyValue(frictionOde, "mu2",
              Values2str(1, &(*ge)->mu2));
        if (!(*ge)->fdir1.empty())
          AddKeyValue(frictionOde, "fdir1", (*ge)->fdir1);
        if ((*ge)->isKp)
          AddKeyValue(contactOde, "kp", Values2str(1, &(*ge)->kp));
        if ((*ge)->isKd)
          AddKeyValue(contactOde, "kd", Values2str(1, &(*ge)->kd));
        // max contact interpenetration correction velocity
        if ((*ge)->isMaxVel)
          AddKeyValue(contactOde, "max_vel",
              Values2str(1, &(*ge)->maxVel));
        // contact interpenetration margin tolerance
        if ((*ge)->isMinDepth)
          AddKeyValue(contactOde, "min_depth",
              Values2str(1, &(*ge)->minDepth));
        if ((*ge)->isLaserRetro)
          AddKeyValue(_elem, "laser_retro",
              Values2str(1, &(*ge)->laserRetro));

        contact->LinkEndChild(contactOde);
        surface->LinkEndChild(contact);
        friction->LinkEndChild(frictionOde);
        surface->LinkEndChild(friction);
        _elem->LinkEndChild(surface);
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
void InsertSDFExtensionVisual(TiXmlElement *_elem,
    const std::string &_linkName)
{
  for (std::map<std::string, std::vector<SDFExtension*> >::iterator
      sdfIt = g_extensions.begin();
      sdfIt != g_extensions.end(); ++sdfIt)
  {
    for (std::vector<SDFExtension*>::iterator ge = sdfIt->second.begin();
        ge != sdfIt->second.end(); ++ge)
    {
      if ((*ge)->oldLinkName == _linkName)
      {
        // insert material block
        if (!(*ge)->material.empty())
          AddKeyValue(_elem, "material", (*ge)->material);
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
void InsertSDFExtensionLink(TiXmlElement *_elem, const std::string &_linkName)
{
  for (std::map<std::string, std::vector<SDFExtension*> >::iterator
       sdfIt = g_extensions.begin();
       sdfIt != g_extensions.end(); ++sdfIt)
  {
    if (sdfIt->first == _linkName)
    {
      // sdfdbg << "inserting extension with reference ["
      //       << _linkName << "] into link.\n";
      for (std::vector<SDFExtension*>::iterator ge =
          sdfIt->second.begin(); ge != sdfIt->second.end(); ++ge)
      {
        // insert gravity
        if ((*ge)->gravity)
          AddKeyValue(_elem, "gravity", "true");
        else
          AddKeyValue(_elem, "gravity", "false");

        // damping factor
        TiXmlElement *velocityDecay = new TiXmlElement("velocity_decay");
        if ((*ge)->isDampingFactor)
        {
          /// @todo separate linear and angular velocity decay
          AddKeyValue(velocityDecay, "linear",
              Values2str(1, &(*ge)->dampingFactor));
          AddKeyValue(velocityDecay, "angular",
              Values2str(1, &(*ge)->dampingFactor));
        }
        _elem->LinkEndChild(velocityDecay);
        // selfCollide tag
        if ((*ge)->selfCollide)
          AddKeyValue(_elem, "self_collide", "true");
        else
          AddKeyValue(_elem, "self_collide", "false");
        // insert blobs into body
        for (std::vector<TiXmlElement*>::iterator
            blobIt = (*ge)->blobs.begin();
            blobIt != (*ge)->blobs.end(); ++blobIt)
        {
          _elem->LinkEndChild((*blobIt)->Clone());
        }
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
void InsertSDFExtensionJoint(TiXmlElement *_elem,
    const std::string &_jointName)
{
  for (std::map<std::string, std::vector<SDFExtension*> >::iterator
      sdfIt = g_extensions.begin();
      sdfIt != g_extensions.end(); ++sdfIt)
  {
    if (sdfIt->first == _jointName)
    {
      for (std::vector<SDFExtension*>::iterator
          ge = sdfIt->second.begin();
          ge != sdfIt->second.end(); ++ge)
      {
        TiXmlElement *physics = new TiXmlElement("physics");
        TiXmlElement *physicsOde = new TiXmlElement("ode");
        TiXmlElement *limit = new TiXmlElement("limit");

        // insert stopCfm, stopErp, fudgeFactor
        if ((*ge)->isStopCfm)
        {
          AddKeyValue(limit, "erp", Values2str(1, &(*ge)->stopCfm));
        }
        if ((*ge)->isStopErp)
        {
          AddKeyValue(limit, "cfm", Values2str(1, &(*ge)->stopErp));
        }
        /* gone
           if ((*ge)->isInitialJointPosition)
           AddKeyValue(_elem, "initialJointPosition",
           Values2str(1, &(*ge)->initialJointPosition));
           */

        // insert provideFeedback
        if ((*ge)->provideFeedback)
          AddKeyValue(physicsOde, "provide_feedback", "true");
        else
          AddKeyValue(physicsOde, "provide_feedback", "false");

        // insert cfmDamping
        if ((*ge)->cfmDamping)
          AddKeyValue(physicsOde, "cfm_damping", "true");
        else
          AddKeyValue(physicsOde, "cfm_damping", "false");

        // insert fudgeFactor
        if ((*ge)->isFudgeFactor)
          AddKeyValue(physicsOde, "fudge_factor",
              Values2str(1, &(*ge)->fudgeFactor));

        physics->LinkEndChild(physicsOde);
        physicsOde->LinkEndChild(limit);
        _elem->LinkEndChild(physics);
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
void InsertSDFExtensionRobot(TiXmlElement *_elem)
{
  for (std::map<std::string, std::vector<SDFExtension*> >::iterator
      sdfIt = g_extensions.begin();
      sdfIt != g_extensions.end(); ++sdfIt)
  {
    if (sdfIt->first.empty())
    {
      // no reference specified
      for (std::vector<SDFExtension*>::iterator
          ge = sdfIt->second.begin(); ge != sdfIt->second.end(); ++ge)
      {
        // insert static flag
        if ((*ge)->setStaticFlag)
          AddKeyValue(_elem, "static", "true");
        else
          AddKeyValue(_elem, "static", "false");

        // copy extension containing blobs and without reference
        for (std::vector<TiXmlElement*>::iterator
            blobIt = (*ge)->blobs.begin();
            blobIt != (*ge)->blobs.end(); ++blobIt)
        {
          std::ostringstream streamIn;
          streamIn << *(*blobIt);
          _elem->LinkEndChild((*blobIt)->Clone());
        }
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
void CreateGeometry(TiXmlElement* _elem,
    boost::shared_ptr<urdf::Geometry> _geom)
{
  int sizeCount;
  double sizeVals[3];

  TiXmlElement *sdfGeometry = new TiXmlElement("geometry");

  std::string type;
  TiXmlElement *geometryType = NULL;

  switch (_geom->type)
  {
    case urdf::Geometry::BOX:
      type = "box";
      sizeCount = 3;
      {
        boost::shared_ptr<const urdf::Box> box;
        box = boost::dynamic_pointer_cast< const urdf::Box >(_geom);
        sizeVals[0] = box->dim.x;
        sizeVals[1] = box->dim.y;
        sizeVals[2] = box->dim.z;
        geometryType = new TiXmlElement(type);
        AddKeyValue(geometryType, "size",
            Values2str(sizeCount, sizeVals));
      }
      break;
    case urdf::Geometry::CYLINDER:
      type = "cylinder";
      sizeCount = 2;
      {
        boost::shared_ptr<const urdf::Cylinder> cylinder;
        cylinder = boost::dynamic_pointer_cast<const urdf::Cylinder >(_geom);
        geometryType = new TiXmlElement(type);
        AddKeyValue(geometryType, "length",
            Values2str(1, &cylinder->length));
        AddKeyValue(geometryType, "radius",
            Values2str(1, &cylinder->radius));
      }
      break;
    case urdf::Geometry::SPHERE:
      type = "sphere";
      sizeCount = 1;
      {
        boost::shared_ptr<const urdf::Sphere> sphere;
        sphere = boost::dynamic_pointer_cast<const urdf::Sphere >(_geom);
        geometryType = new TiXmlElement(type);
        AddKeyValue(geometryType, "radius",
            Values2str(1, &sphere->radius));
      }
      break;
    case urdf::Geometry::MESH:
      type = "mesh";
      sizeCount = 3;
      {
        boost::shared_ptr<const urdf::Mesh> mesh;
        mesh = boost::dynamic_pointer_cast<const urdf::Mesh >(_geom);
        sizeVals[0] = mesh->scale.x;
        sizeVals[1] = mesh->scale.y;
        sizeVals[2] = mesh->scale.z;
        geometryType = new TiXmlElement(type);
        AddKeyValue(geometryType, "scale", Vector32Str(mesh->scale));
        // do something more to meshes
        {
          /* set mesh file */
          if (mesh->filename.empty())
          {
            sdferr << "urdf2sdf: mesh geometry with no filename given.\n";
          }

          // give some warning if file does not exist.
          // disabled while switching to uri
          // @todo: re-enable check
          // std::ifstream fin;
          // fin.open(mesh->filename.c_str(), std::ios::in);
          // fin.close();
          // if (fin.fail())
          //   sdfwarn << "filename referred by mesh ["
          //          << mesh->filename << "] does not appear to exist.\n";

          // Convert package:// to model://,
          // in ROS, this will work if
          // the model package is in ROS_PACKAGE_PATH and has a manifest.xml
          // as a typical ros package does.
          std::string modelFilename = mesh->filename;
          std::string packagePrefix("package://");
          std::string modelPrefix("model://");
          size_t pos1 = modelFilename.find(packagePrefix, 0);
          if (pos1 != std::string::npos)
          {
            size_t repLen = packagePrefix.size();
            modelFilename.replace(pos1, repLen, modelPrefix);
            // sdfwarn << "ros style uri [package://] is"
            //   << "automatically converted: [" << modelFilename
            //   << "], make sure your ros package is in GAZEBO_MODEL_PATH"
            //   << " and switch your manifest to conform to sdf's"
            //   << " model database format.  See ["
            //   << "http://sdfsim.org/wiki/Model_database#Model_Manifest_XML"
            //   << "] for more info.\n";
          }

          // add mesh filename
          AddKeyValue(geometryType, "uri", modelFilename);
        }
      }
      break;
    default:
      sizeCount = 0;
      sdfwarn << "Unknown body type: [" << _geom->type
        << "] skipped in geometry\n";
      break;
  }

  if (geometryType)
  {
    sdfGeometry->LinkEndChild(geometryType);
    _elem->LinkEndChild(sdfGeometry);
  }
}

////////////////////////////////////////////////////////////////////////////////
std::string GetGeometryBoundingBox(
    boost::shared_ptr<urdf::Geometry> _geom, double *_sizeVals)
{
  std::string type;

  switch (_geom->type)
  {
    case urdf::Geometry::BOX:
      type = "box";
      {
        boost::shared_ptr<const urdf::Box> box;
        box = boost::dynamic_pointer_cast<const urdf::Box >(_geom);
        _sizeVals[0] = box->dim.x;
        _sizeVals[1] = box->dim.y;
        _sizeVals[2] = box->dim.z;
      }
      break;
    case urdf::Geometry::CYLINDER:
      type = "cylinder";
      {
        boost::shared_ptr<const urdf::Cylinder> cylinder;
        cylinder = boost::dynamic_pointer_cast<const urdf::Cylinder >(_geom);
        _sizeVals[0] = cylinder->radius * 2;
        _sizeVals[1] = cylinder->radius * 2;
        _sizeVals[2] = cylinder->length;
      }
      break;
    case urdf::Geometry::SPHERE:
      type = "sphere";
      {
        boost::shared_ptr<const urdf::Sphere> sphere;
        sphere = boost::dynamic_pointer_cast<const urdf::Sphere >(_geom);
        _sizeVals[0] = _sizeVals[1] = _sizeVals[2] = sphere->radius * 2;
      }
      break;
    case urdf::Geometry::MESH:
      type = "trimesh";
      {
        boost::shared_ptr<const urdf::Mesh> mesh;
        mesh = boost::dynamic_pointer_cast<const urdf::Mesh >(_geom);
        _sizeVals[0] = mesh->scale.x;
        _sizeVals[1] = mesh->scale.y;
        _sizeVals[2] = mesh->scale.z;
      }
      break;
    default:
      _sizeVals[0] = _sizeVals[1] = _sizeVals[2] = 0;
      sdfwarn << "Unknown body type: [" << _geom->type
        << "] skipped in geometry\n";
      break;
  }

  return type;
}

////////////////////////////////////////////////////////////////////////////////
void PrintCollisionGroups(UrdfLinkPtr _link)
{
  sdfdbg << "COLLISION LUMPING: link: [" << _link->name << "] contains ["
    << static_cast<int>(_link->collision_groups.size())
    << "] collisions.\n";
  for (std::map<std::string,
      boost::shared_ptr<std::vector<UrdfCollisionPtr > > >::iterator
      colsIt = _link->collision_groups.begin();
      colsIt != _link->collision_groups.end(); ++colsIt)
  {
    sdfdbg << "    collision_groups: [" << colsIt->first << "] has ["
      << static_cast<int>(colsIt->second->size())
      << "] Collision objects\n";
  }
}

////////////////////////////////////////////////////////////////////////////////
urdf::Pose TransformToParentFrame(
    urdf::Pose _transformInLinkFrame, urdf::Pose _parentToLinkTransform)
{
  // transform to sdf::Pose then call TransformToParentFrame
  sdf::Pose p1 = CopyPose(_transformInLinkFrame);
  sdf::Pose p2 = CopyPose(_parentToLinkTransform);
  return CopyPose(TransformToParentFrame(p1, p2));
}

////////////////////////////////////////////////////////////////////////////////
sdf::Pose TransformToParentFrame(sdf::Pose _transformInLinkFrame,
    urdf::Pose _parentToLinkTransform)
{
  // transform to sdf::Pose then call TransformToParentFrame
  sdf::Pose p2 = CopyPose(_parentToLinkTransform);
  return TransformToParentFrame(_transformInLinkFrame, p2);
}

////////////////////////////////////////////////////////////////////////////////
sdf::Pose TransformToParentFrame(sdf::Pose _transformInLinkFrame,
    sdf::Pose _parentToLinkTransform)
{
  sdf::Pose transformInParentLinkFrame;
  // rotate link pose to parentLink frame
  transformInParentLinkFrame.pos =
    _parentToLinkTransform.rot * _transformInLinkFrame.pos;
  transformInParentLinkFrame.rot =
    _parentToLinkTransform.rot * _transformInLinkFrame.rot;
  // translate link to parentLink frame
  transformInParentLinkFrame.pos =
    _parentToLinkTransform.pos + transformInParentLinkFrame.pos;

  return transformInParentLinkFrame;
}

/////////////////////////////////////////////////
/// reduced fixed joints: transform to parent frame
sdf::Pose inverseTransformToParentFrame(
    sdf::Pose _transformInLinkFrame,
    urdf::Pose _parentToLinkTransform)
{
  sdf::Pose transformInParentLinkFrame;
  //   rotate link pose to parentLink frame
  urdf::Rotation ri = _parentToLinkTransform.rotation.GetInverse();
  sdf::Quaternion q1(ri.w, ri.x, ri.y, ri.z);
  transformInParentLinkFrame.pos = q1 * _transformInLinkFrame.pos;
  urdf::Rotation r2 = _parentToLinkTransform.rotation.GetInverse();
  sdf::Quaternion q3(r2.w, r2.x, r2.y, r2.z);
  transformInParentLinkFrame.rot = q3 * _transformInLinkFrame.rot;
  //   translate link to parentLink frame
  transformInParentLinkFrame.pos.x = transformInParentLinkFrame.pos.x
    - _parentToLinkTransform.position.x;
  transformInParentLinkFrame.pos.y = transformInParentLinkFrame.pos.y
    - _parentToLinkTransform.position.y;
  transformInParentLinkFrame.pos.z = transformInParentLinkFrame.pos.z
    - _parentToLinkTransform.position.z;

  return transformInParentLinkFrame;
}

////////////////////////////////////////////////////////////////////////////////
void ReduceSDFExtensionToParent(UrdfLinkPtr _link)
{
  /// @todo: this is a very complicated module that updates the plugins
  /// based on fixed joint reduction really wish this could be a lot cleaner

  std::string linkName = _link->name;

  // update extension map with references to linkName
  // this->ListSDFExtensions();
  std::map<std::string, std::vector<SDFExtension*> >::iterator ext =
    g_extensions.find(linkName);
  if (ext != g_extensions.end())
  {
    // sdfdbg << "  REDUCE EXTENSION: moving reference from ["
    //       << linkName << "] to [" << _link->getParent()->name << "]\n";

    // update reduction transform (for rays, cameras for now).
    //   FIXME: contact frames too?
    for (std::vector<SDFExtension*>::iterator ge = ext->second.begin();
        ge != ext->second.end(); ++ge)
    {
      (*ge)->reductionTransform = TransformToParentFrame(
          (*ge)->reductionTransform,
          _link->parent_joint->parent_to_joint_origin_transform);
      // for sensor and projector blocks only
      ReduceSDFExtensionsTransform((*ge));
    }

    // find pointer to the existing extension with the new _link reference
    std::string newLinkName = _link->getParent()->name;
    std::map<std::string, std::vector<SDFExtension*> >::iterator
      newExt = g_extensions.find(newLinkName);

    // if none exist, create new extension with newLinkName
    if (newExt == g_extensions.end())
    {
      std::vector<SDFExtension*> ge;
      g_extensions.insert(std::make_pair(
            newLinkName, ge));
      newExt = g_extensions.find(newLinkName);
    }

    // move sdf extensions from _link into the parent _link's extensions
    for (std::vector<SDFExtension*>::iterator ge = ext->second.begin();
        ge != ext->second.end(); ++ge)
      newExt->second.push_back(*ge);
    ext->second.clear();
  }

  // for extensions with empty reference, search and replace
  // _link name patterns within the plugin with new _link name
  // and assign the proper reduction transform for the _link name pattern
  for (std::map<std::string, std::vector<SDFExtension*> >::iterator
      sdfIt = g_extensions.begin();
      sdfIt != g_extensions.end(); ++sdfIt)
  {
    // update reduction transform (for contacts, rays, cameras for now).
    for (std::vector<SDFExtension*>::iterator
        ge = sdfIt->second.begin(); ge != sdfIt->second.end(); ++ge)
      ReduceSDFExtensionFrameReplace(*ge, _link);
  }

  // this->ListSDFExtensions();
}

////////////////////////////////////////////////////////////////////////////////
void ReduceSDFExtensionFrameReplace(SDFExtension* _ge,
    UrdfLinkPtr _link)
{
  // std::string linkName = _link->name;
  // std::string newLinkName = _link->getParent()->name;

  // HACK: need to do this more generally, but we also need to replace
  //       all instances of _link name with new link name
  //       e.g. contact sensor refers to
  //         <collision>base_link_collision</collision>
  //         and it needs to be reparented to
  //         <collision>base_footprint_collision</collision>
  // sdfdbg << "  STRING REPLACE: instances of _link name ["
  //       << linkName << "] with [" << newLinkName << "]\n";
  for (std::vector<TiXmlElement*>::iterator blobIt = _ge->blobs.begin();
      blobIt != _ge->blobs.end(); ++blobIt)
  {
    std::ostringstream debugStreamIn;
    debugStreamIn << *(*blobIt);
    // std::string debugBlob = debugStreamIn.str();
    // sdfdbg << "        INITIAL STRING link ["
    //       << linkName << "]-->[" << newLinkName << "]: ["
    //       << debugBlob << "]\n";

    ReduceSDFExtensionContactSensorFrameReplace(blobIt, _link);
    ReduceSDFExtensionPluginFrameReplace(blobIt, _link,
        "plugin", "bodyName", _ge->reductionTransform);
    ReduceSDFExtensionPluginFrameReplace(blobIt, _link,
        "plugin", "frameName", _ge->reductionTransform);
    ReduceSDFExtensionProjectorFrameReplace(blobIt, _link);
    ReduceSDFExtensionGripperFrameReplace(blobIt, _link);
    ReduceSDFExtensionJointFrameReplace(blobIt, _link);

    std::ostringstream debugStreamOut;
    debugStreamOut << *(*blobIt);
  }
}

////////////////////////////////////////////////////////////////////////////////
void ReduceSDFExtensionsTransform(SDFExtension* _ge)
{
  for (std::vector<TiXmlElement*>::iterator blobIt = _ge->blobs.begin();
      blobIt != _ge->blobs.end(); ++blobIt)
  {
    /// @todo make sure we are not missing any additional transform reductions
    ReduceSDFExtensionSensorTransformReduction(blobIt,
        _ge->reductionTransform);
    ReduceSDFExtensionProjectorTransformReduction(blobIt,
        _ge->reductionTransform);
  }
}

////////////////////////////////////////////////////////////////////////////////
void URDF2SDF::ListSDFExtensions()
{
  for (std::map<std::string, std::vector<SDFExtension*> >::iterator
      sdfIt = g_extensions.begin();
      sdfIt != g_extensions.end(); ++sdfIt)
  {
    int extCount = 0;
    for (std::vector<SDFExtension*>::iterator ge = sdfIt->second.begin();
        ge != sdfIt->second.end(); ++ge)
    {
      if ((*ge)->blobs.size() > 0)
      {
        sdfdbg <<  "  PRINTING [" << static_cast<int>((*ge)->blobs.size())
          << "] BLOBS for extension [" << ++extCount
          << "] referencing [" << sdfIt->first << "]\n";
        for (std::vector<TiXmlElement*>::iterator
            blobIt = (*ge)->blobs.begin();
            blobIt != (*ge)->blobs.end(); ++blobIt)
        {
          std::ostringstream streamIn;
          streamIn << *(*blobIt);
          sdfdbg << "    BLOB: [" << streamIn.str() << "]\n";
        }
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
void URDF2SDF::ListSDFExtensions(const std::string &_reference)
{
  for (std::map<std::string, std::vector<SDFExtension*> >::iterator
      sdfIt = g_extensions.begin();
      sdfIt != g_extensions.end(); ++sdfIt)
  {
    if (sdfIt->first == _reference)
    {
      sdfdbg <<  "  PRINTING [" << static_cast<int>(sdfIt->second.size())
        << "] extensions referencing [" << _reference << "]\n";
      for (std::vector<SDFExtension*>::iterator
          ge = sdfIt->second.begin(); ge != sdfIt->second.end(); ++ge)
      {
        for (std::vector<TiXmlElement*>::iterator
            blobIt = (*ge)->blobs.begin();
            blobIt != (*ge)->blobs.end(); ++blobIt)
        {
          std::ostringstream streamIn;
          streamIn << *(*blobIt);
          sdfdbg << "    BLOB: [" << streamIn.str() << "]\n";
        }
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
void CreateSDF(TiXmlElement *_root,
    ConstUrdfLinkPtr _link,
    const sdf::Pose &_transform)
{
  sdf::Pose _currentTransform = _transform;

  // must have an <inertial> block and cannot have zero mass.
  //  allow det(I) == zero, in the case of point mass geoms.
  // @todo:  keyword "world" should be a constant defined somewhere else
  if (_link->name != "world" &&
      ((!_link->inertial) || sdf::equal(_link->inertial->mass, 0.0)))
  {
    if (!_link->child_links.empty())
      sdfwarn << "urdf2sdf: link[" << _link->name
        << "] has no inertia, ["
        << static_cast<int>(_link->child_links.size())
        << "] children links ignored\n.";

    if (!_link->child_joints.empty())
      sdfwarn << "urdf2sdf: link[" << _link->name
        << "] has no inertia, ["
        << static_cast<int>(_link->child_links.size())
        << "] children joints ignored\n.";

    if (_link->parent_joint)
      sdfwarn << "urdf2sdf: link[" << _link->name
        << "] has no inertia, "
        << "parent joint [" << _link->parent_joint->name
        << "] ignored\n.";

    sdfwarn << "urdf2sdf: link[" << _link->name
      << "] has no inertia, not modeled in sdf\n";
    return;
  }

  /* create <body:...> block for non fixed joint attached bodies */
  if ((_link->getParent() && _link->getParent()->name == "world") ||
      !g_reduceFixedJoints ||
      (!_link->parent_joint ||
       _link->parent_joint->type != urdf::Joint::FIXED))
    CreateLink(_root, _link, _currentTransform);

  // recurse into children
  for (unsigned int i = 0 ; i < _link->child_links.size() ; ++i)
    CreateSDF(_root, _link->child_links[i], _currentTransform);
}

////////////////////////////////////////////////////////////////////////////////
sdf::Pose CopyPose(urdf::Pose _pose)
{
  sdf::Pose p;
  p.pos.x = _pose.position.x;
  p.pos.y = _pose.position.y;
  p.pos.z = _pose.position.z;
  p.rot.x = _pose.rotation.x;
  p.rot.y = _pose.rotation.y;
  p.rot.z = _pose.rotation.z;
  p.rot.w = _pose.rotation.w;
  return p;
}

////////////////////////////////////////////////////////////////////////////////
urdf::Pose CopyPose(sdf::Pose _pose)
{
  urdf::Pose p;
  p.position.x = _pose.pos.x;
  p.position.y = _pose.pos.y;
  p.position.z = _pose.pos.z;
  p.rotation.x = _pose.rot.x;
  p.rotation.y = _pose.rot.y;
  p.rotation.z = _pose.rot.z;
  p.rotation.w = _pose.rot.w;
  return p;
}

////////////////////////////////////////////////////////////////////////////////
void CreateLink(TiXmlElement *_root,
    ConstUrdfLinkPtr _link,
    sdf::Pose &_currentTransform)
{
  /* create new body */
  TiXmlElement *elem     = new TiXmlElement("link");

  /* set body name */
  elem->SetAttribute("name", _link->name);

  /* compute global transform */
  sdf::Pose localTransform;
  // this is the transform from parent link to current _link
  // this transform does not exist for the root link
  if (_link->parent_joint)
  {
    localTransform = CopyPose(
        _link->parent_joint->parent_to_joint_origin_transform);
    _currentTransform = localTransform * _currentTransform;
  }
  else
    sdfdbg << "[" << _link->name << "] has no parent joint\n";

  // create origin tag for this element
  AddTransform(elem, _currentTransform);

  /* create new inerial block */
  CreateInertial(elem, _link);

  /* create new collision block */
  CreateCollisions(elem, _link);

  /* create new visual block */
  CreateVisuals(elem, _link);

  /* copy sdf extensions data */
  InsertSDFExtensionLink(elem, _link->name);

  /* add body to document */
  _root->LinkEndChild(elem);

  /* make a <joint:...> block */
  CreateJoint(_root, _link, _currentTransform);
}

////////////////////////////////////////////////////////////////////////////////
void CreateCollisions(TiXmlElement* _elem,
    ConstUrdfLinkPtr _link)
{
  // loop through all collision groups. as well as additional collision from
  //   lumped meshes (fixed joint reduction)
  for (std::map<std::string,
      boost::shared_ptr<std::vector<UrdfCollisionPtr> > >::const_iterator
      collisionsIt = _link->collision_groups.begin();
      collisionsIt != _link->collision_groups.end(); ++collisionsIt)
  {
    unsigned int defaultMeshCount = 0;
    unsigned int groupMeshCount = 0;
    unsigned int lumpMeshCount = 0;
    // loop through collisions in each group
    for (std::vector<UrdfCollisionPtr>::iterator
        collision = collisionsIt->second->begin();
        collision != collisionsIt->second->end();
        ++collision)
    {
      if (collisionsIt->first == "default")
      {
        // sdfdbg << "creating default collision for link [" << _link->name
        //       << "]";

        std::string collisionPrefix = _link->name;

        if (defaultMeshCount > 0)
        {
          // append _[meshCount] to link name for additional collisions
          std::ostringstream collisionNameStream;
          collisionNameStream << collisionPrefix << "_" << defaultMeshCount;
          collisionPrefix = collisionNameStream.str();
        }

        /* make a <collision> block */
        CreateCollision(_elem, _link, *collision, collisionPrefix);

        // only 1 default mesh
        ++defaultMeshCount;
      }
      else if (collisionsIt->first.find(std::string("lump::")) == 0)
      {
        // if collision name starts with "lump::", pass through
        //   original parent link name
        // sdfdbg << "creating lump collision [" << collisionsIt->first
        //       << "] for link [" << _link->name << "].\n";
        /// collisionPrefix is the original name before lumping
        std::string collisionPrefix = collisionsIt->first.substr(6);

        if (lumpMeshCount > 0)
        {
          // append _[meshCount] to link name for additional collisions
          std::ostringstream collisionNameStream;
          collisionNameStream << collisionPrefix << "_" << lumpMeshCount;
          collisionPrefix = collisionNameStream.str();
        }

        CreateCollision(_elem, _link, *collision, collisionPrefix);
        ++lumpMeshCount;
      }
      else
      {
        // sdfdbg << "adding collisions from collision group ["
        //      << collisionsIt->first << "]\n";

        std::string collisionPrefix = _link->name + std::string("_") +
          collisionsIt->first;

        if (groupMeshCount > 0)
        {
          // append _[meshCount] to _link name for additional collisions
          std::ostringstream collisionNameStream;
          collisionNameStream << collisionPrefix << "_" << groupMeshCount;
          collisionPrefix = collisionNameStream.str();
        }

        CreateCollision(_elem, _link, *collision, collisionPrefix);
        ++groupMeshCount;
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
void CreateVisuals(TiXmlElement* _elem,
    ConstUrdfLinkPtr _link)
{
  // loop through all visual groups. as well as additional visuals from
  //   lumped meshes (fixed joint reduction)
  for (std::map<std::string,
      boost::shared_ptr<std::vector<UrdfVisualPtr> > >::const_iterator
      visualsIt = _link->visual_groups.begin();
      visualsIt != _link->visual_groups.end(); ++visualsIt)
  {
    unsigned int defaultMeshCount = 0;
    unsigned int groupMeshCount = 0;
    unsigned int lumpMeshCount = 0;
    // loop through all visuals in this group
    for (std::vector<UrdfVisualPtr>::iterator
        visual = visualsIt->second->begin();
        visual != visualsIt->second->end();
        ++visual)
    {
      if (visualsIt->first == "default")
      {
        // sdfdbg << "creating default visual for link [" << _link->name
        //       << "]";

        std::string visualPrefix = _link->name;

        if (defaultMeshCount > 0)
        {
          // append _[meshCount] to _link name for additional visuals
          std::ostringstream visualNameStream;
          visualNameStream << visualPrefix << "_" << defaultMeshCount;
          visualPrefix = visualNameStream.str();
        }

        // create a <visual> block
        CreateVisual(_elem, _link, *visual, visualPrefix);

        // only 1 default mesh
        ++defaultMeshCount;
      }
      else if (visualsIt->first.find(std::string("lump::")) == 0)
      {
        // if visual name starts with "lump::", pass through
        //   original parent link name
        // sdfdbg << "creating lump visual [" << visualsIt->first
        //       << "] for link [" << _link->name << "].\n";
        /// visualPrefix is the original name before lumping
        std::string visualPrefix = visualsIt->first.substr(6);

        if (lumpMeshCount > 0)
        {
          // append _[meshCount] to _link name for additional visuals
          std::ostringstream visualNameStream;
          visualNameStream << visualPrefix << "_" << lumpMeshCount;
          visualPrefix = visualNameStream.str();
        }

        CreateVisual(_elem, _link, *visual, visualPrefix);
        ++lumpMeshCount;
      }
      else
      {
        // sdfdbg << "adding visuals from visual group ["
        //      << visualsIt->first << "]\n";

        std::string visualPrefix = _link->name + std::string("_") +
          visualsIt->first;

        if (groupMeshCount > 0)
        {
          // append _[meshCount] to _link name for additional visuals
          std::ostringstream visualNameStream;
          visualNameStream << visualPrefix << "_" << groupMeshCount;
          visualPrefix = visualNameStream.str();
        }

        CreateVisual(_elem, _link, *visual, visualPrefix);
        ++groupMeshCount;
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
void CreateInertial(TiXmlElement *_elem,
    ConstUrdfLinkPtr _link)
{
  TiXmlElement *inertial = new TiXmlElement("inertial");

  /* set mass properties */
  // check and print a warning message
  double roll, pitch, yaw;
  _link->inertial->origin.rotation.getRPY(roll, pitch, yaw);
  if (!sdf::equal(roll, 0.0) ||
      !sdf::equal(pitch, 0.0) || !sdf::equal(yaw, 0.0))
    sdferr << "rotation of inertial frame in link ["
      << _link->name << "] is not supported\n";

  /// add pose
  sdf::Pose pose = CopyPose(_link->inertial->origin);
  AddTransform(inertial, pose);

  // add mass
  AddKeyValue(inertial, "mass",
      Values2str(1, &_link->inertial->mass));

  // add inertia (ixx, ixy, ixz, iyy, iyz, izz)
  TiXmlElement *inertia = new TiXmlElement("inertia");
  AddKeyValue(inertia, "ixx",
      Values2str(1, &_link->inertial->ixx));
  AddKeyValue(inertia, "ixy",
      Values2str(1, &_link->inertial->ixy));
  AddKeyValue(inertia, "ixz",
      Values2str(1, &_link->inertial->ixz));
  AddKeyValue(inertia, "iyy",
      Values2str(1, &_link->inertial->iyy));
  AddKeyValue(inertia, "iyz",
      Values2str(1, &_link->inertial->iyz));
  AddKeyValue(inertia, "izz",
      Values2str(1, &_link->inertial->izz));
  inertial->LinkEndChild(inertia);

  _elem->LinkEndChild(inertial);
}

////////////////////////////////////////////////////////////////////////////////
void CreateJoint(TiXmlElement *_root,
    ConstUrdfLinkPtr _link,
    sdf::Pose &_currentTransform)
{
  /* compute the joint tag */
  std::string jtype;
  jtype.clear();
  if (_link->parent_joint != NULL)
  {
    switch (_link->parent_joint->type)
    {
      case urdf::Joint::CONTINUOUS:
      case urdf::Joint::REVOLUTE:
        jtype = "revolute";
        break;
      case urdf::Joint::PRISMATIC:
        jtype = "prismatic";
        break;
      case urdf::Joint::FLOATING:
      case urdf::Joint::PLANAR:
        break;
      case urdf::Joint::FIXED:
        jtype = "fixed";
        break;
      default:
        sdfwarn << "Unknown joint type: [" << _link->parent_joint->type
          << "] in link [" << _link->name << "]\n";
        break;
    }
  }

  // skip if joint type is fixed and we are not faking it with a hinge,
  //   skip/return with the exception of root link being world,
  //   because there's no lumping there
  if (_link->getParent() && _link->getParent()->name != "world"
      && jtype == "fixed" && g_reduceFixedJoints) return;

  if (!jtype.empty())
  {
    TiXmlElement *joint = new TiXmlElement("joint");
    if (jtype == "fixed")
      joint->SetAttribute("type", "revolute");
    else
      joint->SetAttribute("type", jtype);
    joint->SetAttribute("name", _link->parent_joint->name);
    AddKeyValue(joint, "child", _link->name);
    AddKeyValue(joint, "parent", _link->getParent()->name);

    TiXmlElement *jointAxis = new TiXmlElement("axis");
    TiXmlElement *jointAxisLimit = new TiXmlElement("limit");
    TiXmlElement *jointAxisDynamics = new TiXmlElement("dynamics");
    if (jtype == "fixed")
    {
      AddKeyValue(jointAxisLimit, "lower", "0");
      AddKeyValue(jointAxisLimit, "upper", "0");
      AddKeyValue(jointAxisDynamics, "damping", "0");
    }
    else
    {
      sdf::Vector3 rotatedJointAxis =
        _currentTransform.rot.RotateVector(
            sdf::Vector3(_link->parent_joint->axis.x,
              _link->parent_joint->axis.y,
              _link->parent_joint->axis.z));
      double rotatedJointAxisArray[3] =
      { rotatedJointAxis.x, rotatedJointAxis.y, rotatedJointAxis.z };
      AddKeyValue(jointAxis, "xyz",
          Values2str(3, rotatedJointAxisArray));
      if (_link->parent_joint->dynamics)
        AddKeyValue(jointAxisDynamics, "damping",
            Values2str(1, &_link->parent_joint->dynamics->damping));

      if (g_enforceLimits && _link->parent_joint->limits)
      {
        if (jtype == "slider")
        {
          AddKeyValue(jointAxisLimit, "lower",
              Values2str(1, &_link->parent_joint->limits->lower));
          AddKeyValue(jointAxisLimit, "upper",
              Values2str(1, &_link->parent_joint->limits->upper));
          AddKeyValue(jointAxisLimit, "effort",
              Values2str(1, &_link->parent_joint->limits->effort));
          AddKeyValue(jointAxisLimit, "velocity",
              Values2str(1, &_link->parent_joint->limits->velocity));
        }
        else if (_link->parent_joint->type != urdf::Joint::CONTINUOUS)
        {
          double *lowstop  = &_link->parent_joint->limits->lower;
          double *highstop = &_link->parent_joint->limits->upper;
          // enforce ode bounds, this will need to be fixed
          if (*lowstop > *highstop)
          {
            sdfwarn << "urdf2sdf: revolute joint ["
              << _link->parent_joint->name
              << "] with limits: lowStop[" << *lowstop
              << "] > highStop[" << highstop
              << "], switching the two.\n";
            double tmp = *lowstop;
            *lowstop = *highstop;
            *highstop = tmp;
          }
          AddKeyValue(jointAxisLimit, "lower",
              Values2str(1, &_link->parent_joint->limits->lower));
          AddKeyValue(jointAxisLimit, "upper",
              Values2str(1, &_link->parent_joint->limits->upper));
          AddKeyValue(jointAxisLimit, "effort",
              Values2str(1, &_link->parent_joint->limits->effort));
          AddKeyValue(jointAxisLimit, "velocity",
              Values2str(1, &_link->parent_joint->limits->velocity));
        }
      }
    }
    jointAxis->LinkEndChild(jointAxisLimit);
    jointAxis->LinkEndChild(jointAxisDynamics);
    joint->LinkEndChild(jointAxis);

    /* copy sdf extensions data */
    InsertSDFExtensionJoint(joint, _link->parent_joint->name);

    /* add joint to document */
    _root->LinkEndChild(joint);
  }
}

////////////////////////////////////////////////////////////////////////////////
void CreateCollision(TiXmlElement* _elem, ConstUrdfLinkPtr _link,
    UrdfCollisionPtr _collision, const std::string &_oldLinkName)
{
  /* begin create geometry node, skip if no collision specified */
  TiXmlElement *sdfCollision = new TiXmlElement("collision");

  /* set its name, if lumped, add original link name */
  if (_oldLinkName == _link->name)
    sdfCollision->SetAttribute("name",
        _link->name + std::string("_collision"));
  else
    sdfCollision->SetAttribute("name",
        _link->name + std::string("_collision_") + _oldLinkName);

  /* set transform */
  double pose[6];
  pose[0] = _collision->origin.position.x;
  pose[1] = _collision->origin.position.y;
  pose[2] = _collision->origin.position.z;
  _collision->origin.rotation.getRPY(pose[3], pose[4], pose[5]);
  AddKeyValue(sdfCollision, "pose", Values2str(6, pose));


  /* add geometry block */
  if (!_collision || !_collision->geometry)
  {
    // sdfdbg << "urdf2sdf: collision of link [" << _link->name
    //       << "] has no <geometry>.\n";
  }
  else
  {
    CreateGeometry(sdfCollision, _collision->geometry);
  }

  /* set additional data from extensions */
  InsertSDFExtensionCollision(sdfCollision, _oldLinkName);

  /* add geometry to body */
  _elem->LinkEndChild(sdfCollision);
}

////////////////////////////////////////////////////////////////////////////////
void CreateVisual(TiXmlElement *_elem, ConstUrdfLinkPtr _link,
    UrdfVisualPtr _visual, const std::string &_oldLinkName)
{
  /* begin create sdf visual node */
  TiXmlElement *sdfVisual = new TiXmlElement("visual");

  /* set its name */
  // sdfdbg << "original link name [" << _oldLinkName
  //       << "] new link name [" << _link->name << "]\n";
  if (_oldLinkName == _link->name)
    sdfVisual->SetAttribute("name", _link->name + std::string("_vis"));
  else
    sdfVisual->SetAttribute("name", _link->name + std::string("_vis_")
        + _oldLinkName);

  /* add the visualisation transfrom */
  double pose[6];
  pose[0] = _visual->origin.position.x;
  pose[1] = _visual->origin.position.y;
  pose[2] = _visual->origin.position.z;
  _visual->origin.rotation.getRPY(pose[3], pose[4], pose[5]);
  AddKeyValue(sdfVisual, "pose", Values2str(6, pose));

  /* insert geometry */
  if (!_visual || !_visual->geometry)
  {
    // sdfdbg << "urdf2sdf: visual of link [" << _link->name
    //       << "] has no <geometry>\n.";
  }
  else
    CreateGeometry(sdfVisual, _visual->geometry);

  /* set additional data from extensions */
  InsertSDFExtensionVisual(sdfVisual, _oldLinkName);

  /* end create _visual node */
  _elem->LinkEndChild(sdfVisual);
}

////////////////////////////////////////////////////////////////////////////////
TiXmlDocument URDF2SDF::InitModelString(const std::string &_urdfStr,
    bool _enforceLimits)
{
  g_enforceLimits = _enforceLimits;

  /* Create a RobotModel from string */
  boost::shared_ptr<urdf::ModelInterface> robotModel =
    urdf::parseURDF(_urdfStr.c_str());

  // an xml object to hold the xml result
  TiXmlDocument sdfXmlOut;

  if (!robotModel)
  {
    sdferr << "Unable to call parseURDF on robot model\n";
    return sdfXmlOut;
  }

  /* create root element and define needed namespaces */
  TiXmlElement *robot = new TiXmlElement("model");

  // set model name to urdf robot name if not specified
  robot->SetAttribute("name", robotModel->getName());

  /* initialize transform for the model, urdf is recursive,
     while sdf defines all links relative to model frame */
  sdf::Pose transform;

  /* parse sdf extension */
  TiXmlDocument urdfXml;
  urdfXml.Parse(_urdfStr.c_str());
  this->ParseSDFExtension(urdfXml);

  ConstUrdfLinkPtr rootLink = robotModel->getRoot();

  /* Fixed Joint Reduction */
  /* if link connects to parent via fixed joint, lump down and remove link */
  /* set reduceFixedJoints to false will replace fixed joints with
     zero limit revolute joints, otherwise, we reduce it down to its
     parent link recursively */
  if (g_reduceFixedJoints)
    ReduceFixedJoints(robot,
        (boost::const_pointer_cast< urdf::Link >(rootLink)));

  if (rootLink->name == "world")
  {
    /* convert all children link */
    for (std::vector<UrdfLinkPtr>::const_iterator
        child = rootLink->child_links.begin();
        child != rootLink->child_links.end(); ++child)
      CreateSDF(robot, (*child), transform);
  }
  else
  {
    /* convert, starting from root link */
    CreateSDF(robot, rootLink, transform);
  }

  /* insert the extensions without reference into <robot> root level */
  InsertSDFExtensionRobot(robot);

  // add robot to sdfXmlOut
  TiXmlElement *sdfSdf = new TiXmlElement("sdf");
  // Until the URDF parser is updated to SDF 1.4, mark the SDF's as 1.3
  // and rely on the sdf convert functions for compatibility.
  sdfSdf->SetAttribute("version", "1.3");  // SDF_VERSION);
  sdfSdf->LinkEndChild(robot);
  sdfXmlOut.LinkEndChild(sdfSdf);

  // debug
  // sdfXmlOut.Print();

  return sdfXmlOut;
}

////////////////////////////////////////////////////////////////////////////////
TiXmlDocument URDF2SDF::InitModelDoc(TiXmlDocument* _xmlDoc)
{
  std::ostringstream stream;
  stream << *_xmlDoc;
  std::string urdfStr = stream.str();
  return InitModelString(urdfStr);
}

////////////////////////////////////////////////////////////////////////////////
TiXmlDocument URDF2SDF::InitModelFile(const std::string &_filename)
{
  TiXmlDocument xmlDoc;
  if (xmlDoc.LoadFile(_filename))
  {
    return this->InitModelDoc(&xmlDoc);
  }
  else
    sdferr << "Unable to load file[" << _filename << "].\n";

  return xmlDoc;
}

////////////////////////////////////////////////////////////////////////////////
void ReduceSDFExtensionSensorTransformReduction(
    std::vector<TiXmlElement*>::iterator _blobIt,
    sdf::Pose _reductionTransform)
{
  // overwrite <xyz> and <rpy> if they exist
  if ((*_blobIt)->ValueStr() == "sensor")
  {
    // parse it and add/replace the reduction transform
    // find first instance of xyz and rpy, replace with reduction transform

    // debug print
    // for (TiXmlNode* elIt = (*_blobIt)->FirstChild();
    //      elIt; elIt = elIt->NextSibling())
    // {
    //   std::ostringstream streamIn;
    //   streamIn << *elIt;
    //   sdfdbg << "    " << streamIn << "\n";
    // }

    {
      TiXmlNode* oldPoseKey = (*_blobIt)->FirstChild("pose");
      /// @todo: FIXME:  we should read xyz, rpy and aggregate it to
      /// reductionTransform instead of just throwing the info away.
      if (oldPoseKey)
        (*_blobIt)->RemoveChild(oldPoseKey);
    }

    // convert reductionTransform to values
    urdf::Vector3 reductionXyz(_reductionTransform.pos.x,
        _reductionTransform.pos.y,
        _reductionTransform.pos.z);
    urdf::Rotation reductionQ(_reductionTransform.rot.x,
        _reductionTransform.rot.y,
        _reductionTransform.rot.z,
        _reductionTransform.rot.w);

    urdf::Vector3 reductionRpy;
    reductionQ.getRPY(reductionRpy.x, reductionRpy.y, reductionRpy.z);

    // output updated pose to text
    std::ostringstream poseStream;
    poseStream << reductionXyz.x << " " << reductionXyz.y
      << " " << reductionXyz.z << " " << reductionRpy.x
      << " " << reductionRpy.y << " " << reductionRpy.z;
    TiXmlText* poseTxt = new TiXmlText(poseStream.str());

    TiXmlElement* poseKey = new TiXmlElement("pose");
    poseKey->LinkEndChild(poseTxt);

    (*_blobIt)->LinkEndChild(poseKey);
  }
}

////////////////////////////////////////////////////////////////////////////////
void ReduceSDFExtensionProjectorTransformReduction(
    std::vector<TiXmlElement*>::iterator _blobIt,
    sdf::Pose _reductionTransform)
{
  // overwrite <pose> (xyz/rpy) if it exists
  if ((*_blobIt)->ValueStr() == "projector")
  {
    /*
    // parse it and add/replace the reduction transform
    // find first instance of xyz and rpy, replace with reduction transform
    for (TiXmlNode* elIt = (*_blobIt)->FirstChild();
    elIt; elIt = elIt->NextSibling())
    {
    std::ostringstream streamIn;
    streamIn << *elIt;
    sdfdbg << "    " << streamIn << "\n";
    }
    */

    /* should read <pose>...</pose> and agregate reductionTransform */
    TiXmlNode* poseKey = (*_blobIt)->FirstChild("pose");
    // read pose and save it

    // remove the tag for now
    if (poseKey) (*_blobIt)->RemoveChild(poseKey);

    // convert reductionTransform to values
    urdf::Vector3 reductionXyz(_reductionTransform.pos.x,
        _reductionTransform.pos.y,
        _reductionTransform.pos.z);
    urdf::Rotation reductionQ(_reductionTransform.rot.x,
        _reductionTransform.rot.y,
        _reductionTransform.rot.z,
        _reductionTransform.rot.w);

    urdf::Vector3 reductionRpy;
    reductionQ.getRPY(reductionRpy.x, reductionRpy.y, reductionRpy.z);

    // output updated pose to text
    std::ostringstream poseStream;
    poseStream << reductionXyz.x << " " << reductionXyz.y
      << " " << reductionXyz.z << " " << reductionRpy.x
      << " " << reductionRpy.y << " " << reductionRpy.z;
    TiXmlText* poseTxt = new TiXmlText(poseStream.str());

    poseKey = new TiXmlElement("pose");
    poseKey->LinkEndChild(poseTxt);

    (*_blobIt)->LinkEndChild(poseKey);
  }
}

////////////////////////////////////////////////////////////////////////////////
void ReduceSDFExtensionContactSensorFrameReplace(
    std::vector<TiXmlElement*>::iterator _blobIt, UrdfLinkPtr _link)
{
  std::string linkName = _link->name;
  std::string newLinkName = _link->getParent()->name;
  if ((*_blobIt)->ValueStr() == "sensor")
  {
    // parse it and add/replace the reduction transform
    // find first instance of xyz and rpy, replace with reduction transform
    TiXmlNode* contact = (*_blobIt)->FirstChild("contact");
    if (contact)
    {
      TiXmlNode* collision = contact->FirstChild("collision");
      if (collision)
      {
        if (GetKeyValueAsString(collision->ToElement()) ==
            linkName + std::string("_collision"))
        {
          contact->RemoveChild(collision);
          TiXmlElement* collisionNameKey = new TiXmlElement("collision");
          std::ostringstream collisionNameStream;
          collisionNameStream << newLinkName << "_collision_" << linkName;
          TiXmlText* collisionNameTxt = new TiXmlText(
              collisionNameStream.str());
          collisionNameKey->LinkEndChild(collisionNameTxt);
          contact->LinkEndChild(collisionNameKey);
        }
        // @todo: FIXME: chagning contact sensor's contact collision
        //   should trigger a update in sensor offset as well.
        //   But first we need to implement offsets in contact sensors
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
void ReduceSDFExtensionPluginFrameReplace(
    std::vector<TiXmlElement*>::iterator _blobIt, UrdfLinkPtr _link,
    const std::string &_pluginName, const std::string &_elementName,
    sdf::Pose _reductionTransform)
{
  std::string linkName = _link->name;
  std::string newLinkName = _link->getParent()->name;
  if ((*_blobIt)->ValueStr() == _pluginName)
  {
    // replace element containing _link names to parent link names
    // find first instance of xyz and rpy, replace with reduction transform
    TiXmlNode* elementNode = (*_blobIt)->FirstChild(_elementName);
    if (elementNode)
    {
      if (GetKeyValueAsString(elementNode->ToElement()) == linkName)
      {
        (*_blobIt)->RemoveChild(elementNode);
        TiXmlElement* bodyNameKey = new TiXmlElement(_elementName);
        std::ostringstream bodyNameStream;
        bodyNameStream << newLinkName;
        TiXmlText* bodyNameTxt = new TiXmlText(bodyNameStream.str());
        bodyNameKey->LinkEndChild(bodyNameTxt);
        (*_blobIt)->LinkEndChild(bodyNameKey);
        /// @todo update transforms for this sdf plugin too

        // look for offset transforms, add reduction transform
        TiXmlNode* xyzKey = (*_blobIt)->FirstChild("xyzOffset");
        if (xyzKey)
        {
          urdf::Vector3 v1 = ParseVector3(xyzKey);
          _reductionTransform.pos = sdf::Vector3(v1.x, v1.y, v1.z);
          // remove xyzOffset and rpyOffset
          (*_blobIt)->RemoveChild(xyzKey);
        }
        TiXmlNode* rpyKey = (*_blobIt)->FirstChild("rpyOffset");
        if (rpyKey)
        {
          urdf::Vector3 rpy = ParseVector3(rpyKey, M_PI/180.0);
          _reductionTransform.rot =
            sdf::Quaternion::EulerToQuaternion(rpy.x, rpy.y, rpy.z);
          // remove xyzOffset and rpyOffset
          (*_blobIt)->RemoveChild(rpyKey);
        }

        // pass through the parent transform from fixed joint reduction
        _reductionTransform = inverseTransformToParentFrame(_reductionTransform,
            _link->parent_joint->parent_to_joint_origin_transform);

        // create new offset xml blocks
        xyzKey = new TiXmlElement("xyzOffset");
        rpyKey = new TiXmlElement("rpyOffset");

        // create new offset xml blocks
        urdf::Vector3 reductionXyz(_reductionTransform.pos.x,
            _reductionTransform.pos.y,
            _reductionTransform.pos.z);
        urdf::Rotation reductionQ(_reductionTransform.rot.x,
            _reductionTransform.rot.y, _reductionTransform.rot.z,
            _reductionTransform.rot.w);

        std::ostringstream xyzStream, rpyStream;
        xyzStream << reductionXyz.x << " " << reductionXyz.y << " "
          << reductionXyz.z;
        urdf::Vector3 reductionRpy;
        reductionQ.getRPY(reductionRpy.x, reductionRpy.y, reductionRpy.z);
        rpyStream << reductionRpy.x << " " << reductionRpy.y << " "
          << reductionRpy.z;

        TiXmlText* xyzTxt = new TiXmlText(xyzStream.str());
        TiXmlText* rpyTxt = new TiXmlText(rpyStream.str());

        xyzKey->LinkEndChild(xyzTxt);
        rpyKey->LinkEndChild(rpyTxt);

        (*_blobIt)->LinkEndChild(xyzKey);
        (*_blobIt)->LinkEndChild(rpyKey);
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
void ReduceSDFExtensionProjectorFrameReplace(
    std::vector<TiXmlElement*>::iterator _blobIt, UrdfLinkPtr _link)
{
  std::string linkName = _link->name;
  std::string newLinkName = _link->getParent()->name;

  // updates _link reference for <projector> inside of
  // projector plugins
  // update from <projector>MyLinkName/MyProjectorName</projector>
  // to <projector>NewLinkName/MyProjectorName</projector>
  TiXmlNode* projectorElem = (*_blobIt)->FirstChild("projector");
  {
    if (projectorElem)
    {
      std::string projectorName =  GetKeyValueAsString(
          projectorElem->ToElement());
      // extract projector _link name and projector name
      size_t pos = projectorName.find("/");
      if (pos == std::string::npos)
        sdferr << "no slash in projector reference tag [" << projectorName
          << "], expecting linkName/projector_name.\n";
      std::string projectorLinkName = projectorName.substr(0, pos);

      if (projectorLinkName == linkName)
      {
        // do the replacement
        projectorName = newLinkName + "/" +
          projectorName.substr(pos+1, projectorName.size());

        (*_blobIt)->RemoveChild(projectorElem);
        TiXmlElement* bodyNameKey = new TiXmlElement("projector");
        std::ostringstream bodyNameStream;
        bodyNameStream << projectorName;
        TiXmlText* bodyNameTxt = new TiXmlText(bodyNameStream.str());
        bodyNameKey->LinkEndChild(bodyNameTxt);
        (*_blobIt)->LinkEndChild(bodyNameKey);
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
void ReduceSDFExtensionGripperFrameReplace(
    std::vector<TiXmlElement*>::iterator _blobIt, UrdfLinkPtr _link)
{
  std::string linkName = _link->name;
  std::string newLinkName = _link->getParent()->name;

  if ((*_blobIt)->ValueStr() == "gripper")
  {
    TiXmlNode* gripperLink = (*_blobIt)->FirstChild("gripper_link");
    if (gripperLink)
    {
      if (GetKeyValueAsString(gripperLink->ToElement()) == linkName)
      {
        (*_blobIt)->RemoveChild(gripperLink);
        TiXmlElement* bodyNameKey = new TiXmlElement("gripper_link");
        std::ostringstream bodyNameStream;
        bodyNameStream << newLinkName;
        TiXmlText* bodyNameTxt = new TiXmlText(bodyNameStream.str());
        bodyNameKey->LinkEndChild(bodyNameTxt);
        (*_blobIt)->LinkEndChild(bodyNameKey);
      }
    }
    TiXmlNode* palmLink = (*_blobIt)->FirstChild("palm_link");
    if (palmLink)
    {
      if (GetKeyValueAsString(palmLink->ToElement()) == linkName)
      {
        (*_blobIt)->RemoveChild(palmLink);
        TiXmlElement* bodyNameKey = new TiXmlElement("palm_link");
        std::ostringstream bodyNameStream;
        bodyNameStream << newLinkName;
        TiXmlText* bodyNameTxt = new TiXmlText(bodyNameStream.str());
        bodyNameKey->LinkEndChild(bodyNameTxt);
        (*_blobIt)->LinkEndChild(bodyNameKey);
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
void ReduceSDFExtensionJointFrameReplace(
    std::vector<TiXmlElement*>::iterator _blobIt, UrdfLinkPtr _link)
{
  std::string linkName = _link->name;
  std::string newLinkName = _link->getParent()->name;

  if ((*_blobIt)->ValueStr() == "joint")
  {
    // parse it and add/replace the reduction transform
    // find first instance of xyz and rpy, replace with reduction transform
    TiXmlNode* parent = (*_blobIt)->FirstChild("parent");
    if (parent)
    {
      if (GetKeyValueAsString(parent->ToElement()) == linkName)
      {
        (*_blobIt)->RemoveChild(parent);
        TiXmlElement* parentNameKey = new TiXmlElement("parent");
        std::ostringstream parentNameStream;
        parentNameStream << newLinkName;
        TiXmlText* parentNameTxt = new TiXmlText(parentNameStream.str());
        parentNameKey->LinkEndChild(parentNameTxt);
        (*_blobIt)->LinkEndChild(parentNameKey);
      }
    }
    TiXmlNode* child = (*_blobIt)->FirstChild("child");
    if (child)
    {
      if (GetKeyValueAsString(child->ToElement()) == linkName)
      {
        (*_blobIt)->RemoveChild(child);
        TiXmlElement* childNameKey = new TiXmlElement("child");
        std::ostringstream childNameStream;
        childNameStream << newLinkName;
        TiXmlText* childNameTxt = new TiXmlText(childNameStream.str());
        childNameKey->LinkEndChild(childNameTxt);
        (*_blobIt)->LinkEndChild(childNameKey);
      }
    }
    /// @todo add anchor offsets if parent link changes location!
  }
}