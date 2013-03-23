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
#ifndef _SDF_HH_
#define _SDF_HH_

#include <vector>
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "sdf/Types.hh"
#include "sdf/Param.hh"

/// \ingroup sdf_parser
/// \brief namespace for Simulation Description Format parser
namespace sdf
{
  class SDF;
  class Element;

  /// \def SDFPtr
  /// \bried boost shared pointer to SDF
  typedef boost::shared_ptr<SDF> SDFPtr;

  /// \def ElementPtr
  /// \bried boost shared pointer to an SDF Element
  typedef boost::shared_ptr<Element> ElementPtr;

  /// \def ElementPtr_V
  /// \brief Vector of ElementPtr
  typedef std::vector< ElementPtr > ElementPtr_V;

  /// \addtogroup sdf
  /// \{

  std::string find_file(const std::string &_filename,
                        bool _searchLocalPath = true);

  /// \class Element Element.hh sdf/sdf.hh
  /// \brief SDF Element class
  class Element : public boost::enable_shared_from_this<Element>
  {
    /// \brief Constructor.
    public: Element();

    /// \brief Destructor.
    public: virtual ~Element();

    /// \brief Create a copy of this Element.
    /// \return A copy of this Element.
    public: boost::shared_ptr<Element> Clone() const;

    /// \brief Copy values from an Element.
    /// \param[in] _elem Element to copy value from.
    public: void Copy(const ElementPtr _elem);

    /// \brief Get a pointer to this Element's parent.
    /// \return Pointer to this Element's parent, NULL if there is no
    /// parent.
    public: ElementPtr GetParent() const;

    /// \brief Set the parent of this Element.
    /// \param[in] _parent Paren for this element.
    public: void SetParent(const ElementPtr _parent);

    /// \brief Set the name of the Element.
    /// \param[in] _name The new name for this Element.
    public: void SetName(const std::string &_name);

    /// \brief Get the Element's name.
    /// \return The name of this Element.
    public: const std::string &GetName() const;

    /// \brief Set the requirement type.
    /// \param[in] _req Requirement type for this element:
    /// 0: Not required
    /// 1: Exactly one element is required
    /// +: One or more elements are required
    /// *: Zero or more elements are required.
    public: void SetRequired(const std::string &_req);

    /// \brief Get the requirement string.
    /// \return The requirement string.
    /// \sa Element::SetRequired
    public: const std::string &GetRequired() const;

    /// \brief Set whether this element should copy its child elements
    /// during parsing.
    /// \param[in] _value True to copy Element's children.
    public: void SetCopyChildren(bool _value);

    /// \brief Return true if this Element's child elements should be copied
    /// during parsing.
    /// \return True to copy child elements during parsing.
    public: bool GetCopyChildren() const;

    /// \brief Output Element's description to stdout.
    /// \param[in] _prefix String value to prefix to the output.
    public: void PrintDescription(const std::string &_prefix);

    /// \brief Output Element's values to stdout.
    /// \param[in] _prefix String value to prefix to the output.
    public: void PrintValues(std::string _prefix);

    public: void PrintWiki(std::string _prefix);

    /// \brief Helper function for SDF::PrintDoc
    ///
    /// This generates the SDF html documentation.
    /// \param[out] _html Accumulated HTML for output.
    /// \param[in] _spacing Amount of spacing for this element.
    /// \param[in] _index Unique index for this element.
    public: void PrintDocLeftPane(std::string &_html,
                                  int _spacing, int &_index);

    /// \brief Helper function for SDF::PrintDoc
    ///
    /// This generates the SDF html documentation.
    /// \param[out] _html Accumulated HTML for output
    /// \param[in] _spacing Amount of spacing for this element.
    public: void PrintDocRightPane(std::string &_html,
                                  int _spacing, int &_index);

    /// \brief Convert the element values to a string representation.
    /// \param[in] _prefix String value to prefix to the output.
    /// \return The string representation.
    public: std::string ToString(const std::string &_prefix) const;

    /// \brief Add an attribute value.
    /// \param[in] _key Key value.
    /// \param[in] _type Type of data the attribute will hold.
    /// \param[in] _defaultValue Default value for the attribute.
    /// \param[in] _required Requirement string. \as Element::SetRequired.
    /// \param[in] _description A text description of the attribute.
    public: void AddAttribute(const std::string &_key,
                              const std::string &_type,
                              const std::string &_defaultvalue,
                              bool _required,
                              const std::string &_description="");

    /// \brief Add a value to this Element.
    /// \param[in] _type Type of data the attribute will hold.
    /// \param[in] _defaultValue Default value for the attribute.
    /// \param[in] _required Requirement string. \as Element::SetRequired.
    /// \param[in] _description A text description of the attribute.
    public: void AddValue(const std::string &_type,
                          const std::string &_defaultValue, bool _required,
                          const std::string &_description="");

    /// \brief Get the param of an attribute.
    /// \param[in] _key the name of the attribute
    /// \return The parameter attribute value. NULL if the key is invalid.
    public: ParamPtr GetAttribute(const std::string &_key);

    /// \brief Get the number of attributes
    public: unsigned int GetAttributeCount() const;

    /// \brief Get an attribute using an index
    public: ParamPtr GetAttribute(unsigned int _index) const;

    /// \brief Get the number of element descriptions
    public: unsigned int GetElementDescriptionCount() const;

    /// \brief Get an element description using an index
    public: ElementPtr GetElementDescription(unsigned int _index) const;

    /// \brief Get an element descriptio using a key
    public: ElementPtr GetElementDescription(const std::string &_key) const;

    /// \brief Return true if an element description exists
    public: bool HasElementDescription(const std::string &_name);

    public: bool HasAttribute(const std::string &_key);

    /// \brief Return true if the attribute was set (i.e. not default value)
    public: bool GetAttributeSet(const std::string &_key);

    /// \brief Get the param of the elements value
    public: ParamPtr GetValue();

    public: template<typename T>
            T Get(const std::string &_key = "")
            {
              T result = T();

              if (_key.empty() && this->value)
                this->value->Get<T>(result);
              else if (!_key.empty())
              {
                ParamPtr param = this->GetAttribute(_key);
                if (param)
                  param->Get(result);
                else if (this->HasElement(_key))
                  result = this->GetElementImpl(_key)->Get<T>();
                else if (this->HasElementDescription(_key))
                  result = this->GetElementDescription(_key)->Get<T>();
                else
                  sdferr << "Unable to find value for key[" << _key << "]\n";
              }
              return result;
            }

    public: template<typename T>
            bool Set(const T &_value)
            {
              if (this->value)
              {
                this->value->Set(_value);
                return true;
              }
              return false;
            }

    public: bool HasElement(const std::string &_name) const;

    public: ElementPtr GetElement(const std::string &_name) const;
    public: ElementPtr GetFirstElement() const;

    public: ElementPtr GetNextElement(const std::string &_name = "") const;

    public: ElementPtr GetElement(const std::string &_name);
    public: ElementPtr AddElement(const std::string &_name);
    public: void InsertElement(ElementPtr _elem);

    /// \brief Remove this element from its parent.
    public: void RemoveFromParent();

    /// \brief Remove a child element.
    /// \param[in] _child Pointer to the child to remove.
    public: void RemoveChild(ElementPtr _child);

    /// \brief Remove all child elements.
    public: void ClearElements();

    public: void Update();
    public: void Reset();

    public: void SetInclude(const std::string &_filename);
    public: std::string GetInclude() const;

    /// \brief Get a text description of the element
    public: std::string GetDescription() const;

    /// \brief Set a text description for the element
    public: void SetDescription(const std::string &_desc);

    /// \brief Add a new element description
    public: void AddElementDescription(ElementPtr _elem);

    private: void ToString(const std::string &_prefix,
                           std::ostringstream &_out) const;


    private: boost::shared_ptr<Param> CreateParam(const std::string &_key,
                 const std::string &_type, const std::string &_defaultValue,
                 bool _required, const std::string &_description="");

    public: ElementPtr GetElementImpl(const std::string &_name) const;

    private: std::string name;
    private: std::string required;
    private: std::string description;
    private: bool copyChildren;

    private: ElementPtr parent;

    // Attributes of this element
    private: Param_V attributes;

    // Value of this element
    private: ParamPtr value;

    // The existing child elements
    private: ElementPtr_V elements;

    // The possible child elements
    private: ElementPtr_V elementDescriptions;

    /// name of the include file that was used to create this element
    private: std::string includeFilename;
  };


  /// \brief Base SDF class
  class SDF
  {
    public: SDF();
    public: void PrintDescription();
    public: void PrintValues();
    public: void PrintWiki();
    public: void PrintDoc();
    public: void Write(const std::string &_filename);
    public: std::string ToString() const;

    /// \brief Set SDF values from a string
    public: void SetFromString(const std::string &_sdfData);

    public: ElementPtr root;

    public: static std::string version;
  };
  /// \}
}
#endif
