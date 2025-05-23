/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file LICENSE.rst or https://cmake.org/licensing for details.  */
#pragma once

#include "cmConfigure.h" // IWYU pragma: keep

#include <cstddef>
#include <string>
#include <vector>

#include "cmListFileCache.h"
#include "cmPolicies.h"
#include "cmPropertyMap.h"
#include "cmValue.h"

class cmMakefile;

/** \class cmTest
 * \brief Represent a test
 *
 * cmTest is representation of a test.
 */
class cmTest
{
public:
  /**
   */
  cmTest(cmMakefile* mf);
  ~cmTest();

  //! Set the test name
  void SetName(std::string const& name);
  std::string GetName() const { return this->Name; }

  void SetCommand(std::vector<std::string> const& command);
  std::vector<std::string> const& GetCommand() const { return this->Command; }

  //! Set/Get a property of this source file
  void SetProperty(std::string const& prop, cmValue value);
  void SetProperty(std::string const& prop, std::nullptr_t)
  {
    this->SetProperty(prop, cmValue{ nullptr });
  }
  void SetProperty(std::string const& prop, std::string const& value)
  {
    this->SetProperty(prop, cmValue(value));
  }
  void AppendProperty(std::string const& prop, std::string const& value,
                      bool asString = false);
  cmValue GetProperty(std::string const& prop) const;
  bool GetPropertyAsBool(std::string const& prop) const;
  cmPropertyMap& GetProperties() { return this->Properties; }

  /** Get the cmMakefile instance that owns this test.  */
  cmMakefile* GetMakefile() { return this->Makefile; }

  /** Get the backtrace of the command that created this test.  */
  cmListFileBacktrace const& GetBacktrace() const;

  /** Get/Set whether this is an old-style test.  */
  bool GetOldStyle() const { return this->OldStyle; }
  void SetOldStyle(bool b) { this->OldStyle = b; }

  /** Get if CMP0158 policy is NEW */
  bool GetCMP0158IsNew() const
  {
    return this->PolicyStatusCMP0158 == cmPolicies::NEW;
  }

  /** Get/Set the CMP0178 policy setting */
  cmPolicies::PolicyStatus GetCMP0178() const
  {
    return this->PolicyStatusCMP0178;
  }
  void SetCMP0178(cmPolicies::PolicyStatus p)
  {
    this->PolicyStatusCMP0178 = p;
  }

  /** Set/Get whether lists in command lines should be expanded. */
  bool GetCommandExpandLists() const;
  void SetCommandExpandLists(bool b);

private:
  cmPropertyMap Properties;
  std::string Name;
  std::vector<std::string> Command;
  bool CommandExpandLists = false;

  bool OldStyle;

  cmMakefile* Makefile;
  cmListFileBacktrace Backtrace;
  cmPolicies::PolicyStatus PolicyStatusCMP0158;
  cmPolicies::PolicyStatus PolicyStatusCMP0178;
};
