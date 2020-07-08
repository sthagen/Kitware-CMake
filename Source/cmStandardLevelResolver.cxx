/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */

#include "cmStandardLevelResolver.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <sstream>
#include <unordered_map>
#include <utility>
#include <vector>

#include <cm/iterator>
#include <cmext/algorithm>

#include "cmGeneratorExpression.h"
#include "cmGeneratorTarget.h"
#include "cmGlobalGenerator.h"
#include "cmMakefile.h"
#include "cmMessageType.h"
#include "cmProperty.h"
#include "cmStringAlgorithms.h"
#include "cmTarget.h"
#include "cmake.h"

namespace {

#define FEATURE_STRING(F) , #F
const char* const C_FEATURES[] = { nullptr FOR_EACH_C_FEATURE(
  FEATURE_STRING) };

const char* const CXX_FEATURES[] = { nullptr FOR_EACH_CXX_FEATURE(
  FEATURE_STRING) };

const char* const CUDA_FEATURES[] = { nullptr FOR_EACH_CUDA_FEATURE(
  FEATURE_STRING) };
#undef FEATURE_STRING

struct StandardNeeded
{
  int index;
  int value;
};

struct StanardLevelComputer
{
  explicit StanardLevelComputer(std::string lang, std::vector<int> levels)
    : Language(std::move(lang))
    , Levels(std::move(levels))
  {
  }

  bool GetNewRequiredStandard(cmMakefile* makefile,
                              std::string const& targetName,
                              const std::string& feature,
                              cmProp currentLangStandardValue,
                              std::string& newRequiredStandard,
                              std::string* error) const
  {
    if (currentLangStandardValue) {
      newRequiredStandard = *currentLangStandardValue;
    } else {
      newRequiredStandard.clear();
    }

    auto needed = this->HighestStandardNeeded(makefile, feature);

    cmProp existingStandard = currentLangStandardValue;
    if (existingStandard == nullptr) {
      cmProp defaultStandard = makefile->GetDef(
        cmStrCat("CMAKE_", this->Language, "_STANDARD_DEFAULT"));
      if (defaultStandard && !defaultStandard->empty()) {
        existingStandard = defaultStandard;
      }
    }

    auto existingLevelIter = cm::cend(this->Levels);
    if (existingStandard) {
      existingLevelIter =
        std::find(cm::cbegin(this->Levels), cm::cend(this->Levels),
                  std::stoi(*existingStandard));
      if (existingLevelIter == cm::cend(this->Levels)) {
        const std::string e =
          cmStrCat("The ", this->Language, "_STANDARD property on target \"",
                   targetName, "\" contained an invalid value: \"",
                   *existingStandard, "\".");
        if (error) {
          *error = e;
        } else {
          makefile->IssueMessage(MessageType::FATAL_ERROR, e);
        }
        return false;
      }
    }

    if (needed.index != -1) {
      // Ensure the C++ language level is high enough to support
      // the needed C++ features.
      if (existingLevelIter == cm::cend(this->Levels) ||
          existingLevelIter < this->Levels.begin() + needed.index) {
        newRequiredStandard = std::to_string(this->Levels[needed.index]);
      }
    }

    return true;
  }

  bool HaveStandardAvailable(cmMakefile* makefile,
                             cmGeneratorTarget const* target,
                             std::string const& config,
                             std::string const& feature) const
  {
    cmProp defaultStandard = makefile->GetDef(
      cmStrCat("CMAKE_", this->Language, "_STANDARD_DEFAULT"));
    if (!defaultStandard) {
      makefile->IssueMessage(
        MessageType::INTERNAL_ERROR,
        cmStrCat("CMAKE_", this->Language,
                 "_STANDARD_DEFAULT is not set.  COMPILE_FEATURES support "
                 "not fully configured for this compiler."));
      // Return true so the caller does not try to lookup the default standard.
      return true;
    }
    // convert defaultStandard to an integer
    if (std::find(cm::cbegin(this->Levels), cm::cend(this->Levels),
                  std::stoi(*defaultStandard)) == cm::cend(this->Levels)) {
      const std::string e = cmStrCat("The CMAKE_", this->Language,
                                     "_STANDARD_DEFAULT variable contains an "
                                     "invalid value: \"",
                                     *defaultStandard, "\".");
      makefile->IssueMessage(MessageType::INTERNAL_ERROR, e);
      return false;
    }

    cmProp existingStandard =
      target->GetLanguageStandard(this->Language, config);
    if (!existingStandard) {
      existingStandard = defaultStandard;
    }

    auto existingLevelIter =
      std::find(cm::cbegin(this->Levels), cm::cend(this->Levels),
                std::stoi(*existingStandard));
    if (existingLevelIter == cm::cend(this->Levels)) {
      const std::string e =
        cmStrCat("The ", this->Language, "_STANDARD property on target \"",
                 target->GetName(), "\" contained an invalid value: \"",
                 *existingStandard, "\".");
      makefile->IssueMessage(MessageType::FATAL_ERROR, e);
      return false;
    }

    auto needed = this->HighestStandardNeeded(makefile, feature);

    return (needed.index == -1) ||
      (this->Levels.begin() + needed.index) <= existingLevelIter;
  }

  StandardNeeded HighestStandardNeeded(cmMakefile* makefile,
                                       std::string const& feature) const
  {
    std::string prefix = cmStrCat("CMAKE_", this->Language);
    StandardNeeded maxLevel = { -1, -1 };
    for (size_t i = 0; i < this->Levels.size(); ++i) {
      if (const char* prop = makefile->GetDefinition(cmStrCat(
            prefix, std::to_string(this->Levels[i]), "_COMPILE_FEATURES"))) {
        std::vector<std::string> props = cmExpandedList(prop);
        if (cm::contains(props, feature)) {
          maxLevel = { static_cast<int>(i), this->Levels[i] };
        }
      }
    }
    return maxLevel;
  }

  bool IsLaterStandard(int lhs, int rhs) const
  {
    auto rhsIt =
      std::find(cm::cbegin(this->Levels), cm::cend(this->Levels), rhs);

    return std::find(rhsIt, cm::cend(this->Levels), lhs) !=
      cm::cend(this->Levels);
  }

  std::string Language;
  std::vector<int> Levels;
};

std::unordered_map<std::string, StanardLevelComputer> StandardComputerMapping =
  {
    { "C", StanardLevelComputer{ "C", std::vector<int>{ 90, 99, 11 } } },
    { "CXX",
      StanardLevelComputer{ "CXX", std::vector<int>{ 98, 11, 14, 17, 20 } } },
    { "CUDA",
      StanardLevelComputer{ "CUDA", std::vector<int>{ 03, 11, 14, 17, 20 } } },
    { "OBJC", StanardLevelComputer{ "OBJC", std::vector<int>{ 90, 99, 11 } } },
    { "OBJCXX",
      StanardLevelComputer{ "OBJCXX",
                            std::vector<int>{ 98, 11, 14, 17, 20 } } },
  };
}

bool cmStandardLevelResolver::AddRequiredTargetFeature(
  cmTarget* target, const std::string& feature, std::string* error) const
{
  if (cmGeneratorExpression::Find(feature) != std::string::npos) {
    target->AppendProperty("COMPILE_FEATURES", feature);
    return true;
  }

  std::string lang;
  if (!this->CheckCompileFeaturesAvailable(target->GetName(), feature, lang,
                                           error)) {
    return false;
  }

  target->AppendProperty("COMPILE_FEATURES", feature);

  // FIXME: Add a policy to avoid updating the <LANG>_STANDARD target
  // property due to COMPILE_FEATURES.  The language standard selection
  // should be done purely at generate time based on whatever the project
  // code put in these properties explicitly.  That is mostly true now,
  // but for compatibility we need to continue updating the property here.
  std::string newRequiredStandard;
  bool newRequired = this->GetNewRequiredStandard(
    target->GetName(), feature,
    target->GetProperty(cmStrCat(lang, "_STANDARD")), newRequiredStandard,
    error);
  if (!newRequiredStandard.empty()) {
    target->SetProperty(cmStrCat(lang, "_STANDARD"), newRequiredStandard);
  }
  return newRequired;
}

bool cmStandardLevelResolver::CheckCompileFeaturesAvailable(
  const std::string& targetName, const std::string& feature, std::string& lang,
  std::string* error) const
{
  if (!this->CompileFeatureKnown(targetName, feature, lang, error)) {
    return false;
  }

  const char* features = this->CompileFeaturesAvailable(lang, error);
  if (!features) {
    return false;
  }

  std::vector<std::string> availableFeatures = cmExpandedList(features);
  if (!cm::contains(availableFeatures, feature)) {
    std::ostringstream e;
    e << "The compiler feature \"" << feature << "\" is not known to " << lang
      << " compiler\n\""
      << this->Makefile->GetDefinition("CMAKE_" + lang + "_COMPILER_ID")
      << "\"\nversion "
      << this->Makefile->GetDefinition("CMAKE_" + lang + "_COMPILER_VERSION")
      << ".";
    if (error) {
      *error = e.str();
    } else {
      this->Makefile->IssueMessage(MessageType::FATAL_ERROR, e.str());
    }
    return false;
  }

  return true;
}

bool cmStandardLevelResolver::CompileFeatureKnown(
  const std::string& targetName, const std::string& feature, std::string& lang,
  std::string* error) const
{
  assert(cmGeneratorExpression::Find(feature) == std::string::npos);

  bool isCFeature =
    std::find_if(cm::cbegin(C_FEATURES) + 1, cm::cend(C_FEATURES),
                 cmStrCmp(feature)) != cm::cend(C_FEATURES);
  if (isCFeature) {
    lang = "C";
    return true;
  }
  bool isCxxFeature =
    std::find_if(cm::cbegin(CXX_FEATURES) + 1, cm::cend(CXX_FEATURES),
                 cmStrCmp(feature)) != cm::cend(CXX_FEATURES);
  if (isCxxFeature) {
    lang = "CXX";
    return true;
  }
  bool isCudaFeature =
    std::find_if(cm::cbegin(CUDA_FEATURES) + 1, cm::cend(CUDA_FEATURES),
                 cmStrCmp(feature)) != cm::cend(CUDA_FEATURES);
  if (isCudaFeature) {
    lang = "CUDA";
    return true;
  }
  std::ostringstream e;
  if (error) {
    e << "specified";
  } else {
    e << "Specified";
  }
  e << " unknown feature \"" << feature
    << "\" for "
       "target \""
    << targetName << "\".";
  if (error) {
    *error = e.str();
  } else {
    this->Makefile->IssueMessage(MessageType::FATAL_ERROR, e.str());
  }
  return false;
}

const char* cmStandardLevelResolver::CompileFeaturesAvailable(
  const std::string& lang, std::string* error) const
{
  if (!this->Makefile->GetGlobalGenerator()->GetLanguageEnabled(lang)) {
    std::ostringstream e;
    if (error) {
      e << "cannot";
    } else {
      e << "Cannot";
    }
    e << " use features from non-enabled language " << lang;
    if (error) {
      *error = e.str();
    } else {
      this->Makefile->IssueMessage(MessageType::FATAL_ERROR, e.str());
    }
    return nullptr;
  }

  const char* featuresKnown =
    this->Makefile->GetDefinition("CMAKE_" + lang + "_COMPILE_FEATURES");

  if (!featuresKnown || !*featuresKnown) {
    std::ostringstream e;
    if (error) {
      e << "no";
    } else {
      e << "No";
    }
    e << " known features for " << lang << " compiler\n\""
      << this->Makefile->GetSafeDefinition("CMAKE_" + lang + "_COMPILER_ID")
      << "\"\nversion "
      << this->Makefile->GetSafeDefinition("CMAKE_" + lang +
                                           "_COMPILER_VERSION")
      << ".";
    if (error) {
      *error = e.str();
    } else {
      this->Makefile->IssueMessage(MessageType::FATAL_ERROR, e.str());
    }
    return nullptr;
  }
  return featuresKnown;
}

bool cmStandardLevelResolver::GetNewRequiredStandard(
  const std::string& targetName, const std::string& feature,
  cmProp currentLangStandardValue, std::string& newRequiredStandard,
  std::string* error) const
{
  std::string lang;
  if (!this->CheckCompileFeaturesAvailable(targetName, feature, lang, error)) {
    return false;
  }

  auto mapping = StandardComputerMapping.find(lang);
  if (mapping != cm::cend(StandardComputerMapping)) {
    return mapping->second.GetNewRequiredStandard(
      this->Makefile, targetName, feature, currentLangStandardValue,
      newRequiredStandard, error);
  }
  return false;
}

bool cmStandardLevelResolver::HaveStandardAvailable(
  cmGeneratorTarget const* target, std::string const& lang,
  std::string const& config, const std::string& feature) const
{
  auto mapping = StandardComputerMapping.find(lang);
  if (mapping != cm::cend(StandardComputerMapping)) {
    return mapping->second.HaveStandardAvailable(this->Makefile, target,
                                                 config, feature);
  }
  return false;
}

bool cmStandardLevelResolver::IsLaterStandard(std::string const& lang,
                                              std::string const& lhs,
                                              std::string const& rhs) const
{
  auto mapping = StandardComputerMapping.find(lang);
  if (mapping != cm::cend(StandardComputerMapping)) {
    return mapping->second.IsLaterStandard(std::stoi(lhs), std::stoi(rhs));
  }
  return false;
}
