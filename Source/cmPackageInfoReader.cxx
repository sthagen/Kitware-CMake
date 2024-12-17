/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmPackageInfoReader.h"

#include <initializer_list>
#include <limits>
#include <unordered_map>
#include <utility>

#include <cmext/string_view>

#include <cm3p/json/reader.h>
#include <cm3p/json/value.h>
#include <cm3p/json/version.h>

#include "cmsys/FStream.hxx"
#include "cmsys/RegularExpression.hxx"

#include "cmExecutionStatus.h"
#include "cmListFileCache.h"
#include "cmMakefile.h"
#include "cmMessageType.h"
#include "cmStringAlgorithms.h"
#include "cmSystemTools.h"
#include "cmTarget.h"

namespace {

// Map of CPS language names to CMake language name.  Case insensitivity is
// achieved by converting the CPS value to lower case, so keys in this map must
// be lower case.
std::unordered_map<std::string, std::string> Languages = {
  // clang-format off
  { "c", "C" },
  { "c++", "CXX" },
  { "cpp", "CXX" },
  { "cxx", "CXX" },
  { "objc", "OBJC" },
  { "objc++", "OBJCXX" },
  { "objcpp", "OBJCXX" },
  { "objcxx", "OBJCXX" },
  { "swift", "swift" },
  { "hip", "HIP" },
  { "cuda", "CUDA" },
  { "ispc", "ISPC" },
  { "c#", "CSharp" },
  { "csharp", "CSharp" },
  { "fortran", "Fortran" },
  // clang-format on
};

std::string GetRealPath(std::string const& path)
{
  return cmSystemTools::GetRealPath(path);
}

std::string GetRealDir(std::string const& path)
{
  return cmSystemTools::GetFilenamePath(cmSystemTools::GetRealPath(path));
}

Json::Value ReadJson(std::string const& fileName)
{
  // Open the specified file.
  cmsys::ifstream file(fileName.c_str(), std::ios::in | std::ios::binary);
  if (!file) {
#if JSONCPP_VERSION_HEXA < 0x01070300
    return Json::Value::null;
#else
    return Json::Value::nullSingleton();
#endif
  }

  // Read file content and translate JSON.
  Json::Value data;
  Json::CharReaderBuilder builder;
  builder["collectComments"] = false;
  if (!Json::parseFromStream(builder, file, &data, nullptr)) {
#if JSONCPP_VERSION_HEXA < 0x01070300
    return Json::Value::null;
#else
    return Json::Value::nullSingleton();
#endif
  }

  return data;
}

bool CheckSchemaVersion(Json::Value const& data)
{
  std::string const& version = data["cps_version"].asString();

  // Check that a valid version is specified.
  if (version.empty()) {
    return false;
  }

  // Check that we understand this version.
  return cmSystemTools::VersionCompare(cmSystemTools::OP_GREATER_EQUAL,
                                       version, "0.13") &&
    cmSystemTools::VersionCompare(cmSystemTools::OP_LESS, version, "1");

  // TODO Eventually this probably needs to return the version tuple, and
  // should share code with cmPackageInfoReader::ParseVersion.
}

bool ComparePathSuffix(std::string const& path, std::string const& suffix)
{
  std::string const& tail = path.substr(path.size() - suffix.size());
  return cmSystemTools::ComparePath(tail, suffix);
}

std::string DeterminePrefix(std::string const& filepath,
                            Json::Value const& data)
{
  // First check if an absolute prefix was supplied.
  std::string prefix = data["prefix"].asString();
  if (!prefix.empty()) {
    // Ensure that the specified prefix is valid.
    if (cmsys::SystemTools::FileIsFullPath(prefix) &&
        cmsys::SystemTools::FileIsDirectory(prefix)) {
      cmSystemTools::ConvertToUnixSlashes(prefix);
      return prefix;
    }
    // The specified absolute prefix is not valid.
    return {};
  }

  // Get and validate prefix-relative path.
  std::string relPath = data["cps_path"].asString();
  cmSystemTools::ConvertToUnixSlashes(relPath);
  if (relPath.empty() || !cmHasLiteralPrefix(relPath, "@prefix@/")) {
    // The relative prefix is not valid.
    return {};
  }
  relPath = relPath.substr(8);

  // Get directory portion of the absolute path.
  std::string const& absPath = cmSystemTools::GetFilenamePath(filepath);
  if (ComparePathSuffix(absPath, relPath)) {
    return absPath.substr(0, absPath.size() - relPath.size());
  }

  for (auto* const f : { GetRealPath, GetRealDir }) {
    std::string const& tmpPath = (*f)(absPath);
    if (!cmSystemTools::ComparePath(tmpPath, absPath) &&
        ComparePathSuffix(tmpPath, relPath)) {
      return tmpPath.substr(0, tmpPath.size() - relPath.size());
    }
  }

  return {};
}

// Extract key name from value iterator as string_view.
cm::string_view IterKey(Json::Value::const_iterator const& iter)
{
  char const* end;
  char const* const start = iter.memberName(&end);
  return { start, static_cast<std::string::size_type>(end - start) };
}

// Get list-of-strings value from object.
std::vector<std::string> ReadList(Json::Value const& data, char const* key)
{
  std::vector<std::string> result;

  Json::Value const& arr = data[key];
  if (arr.isArray()) {
    for (Json::Value const& val : arr) {
      if (val.isString()) {
        result.push_back(val.asString());
      }
    }
  }

  return result;
}

std::string NormalizeTargetName(std::string const& name,
                                std::string const& context)
{
  if (cmHasLiteralPrefix(name, ":")) {
    return cmStrCat(context, name);
  }

  std::string::size_type const n = name.find_first_of(':');
  if (n != std::string::npos) {
    cm::string_view v{ name };
    return cmStrCat(v.substr(0, n), ':', v.substr(n));
  }
  return name;
}

void AppendProperty(cmMakefile* makefile, cmTarget* target,
                    cm::string_view property, cm::string_view configuration,
                    std::string const& value)
{
  std::string fullprop;
  if (configuration.empty()) {
    fullprop = cmStrCat("INTERFACE_"_s, property);
  } else {
    fullprop = cmStrCat("INTERFACE_"_s, property, '_',
                        cmSystemTools::UpperCase(configuration));
  }

  target->AppendProperty(fullprop, value, makefile->GetBacktrace());
}

void AddCompileFeature(cmMakefile* makefile, cmTarget* target,
                       cm::string_view configuration, std::string const& value)
{
  auto reLanguageLevel = []() -> cmsys::RegularExpression {
    static cmsys::RegularExpression re{ "^[Cc]([+][+])?([0-9][0-9])$" };
    return re;
  }();

  if (reLanguageLevel.find(value)) {
    std::string::size_type const n = reLanguageLevel.end() - 2;
    cm::string_view const featurePrefix = (n == 3 ? "cxx_std_"_s : "c_std_"_s);
    if (configuration.empty()) {
      AppendProperty(makefile, target, "COMPILE_FEATURES"_s, {},
                     cmStrCat(featurePrefix, value.substr(n)));
    } else {
      std::string const& feature =
        cmStrCat("$<$<CONFIG:"_s, configuration, ">:"_s, featurePrefix,
                 value.substr(n), '>');
      AppendProperty(makefile, target, "COMPILE_FEATURES"_s, {}, feature);
    }
  } else if (cmStrCaseEq(value, "gnu"_s)) {
    // Not implemented in CMake at this time
  } else if (cmStrCaseEq(value, "threads"_s)) {
    AppendProperty(makefile, target, "LINK_LIBRARIES"_s, configuration,
                   "Threads::Threads");
  }
}

void AddLinkFeature(cmMakefile* makefile, cmTarget* target,
                    cm::string_view configuration, std::string const& value)
{
  if (cmStrCaseEq(value, "thread"_s)) {
    AppendProperty(makefile, target, "LINK_LIBRARIES"_s, configuration,
                   "Threads::Threads");
  }
}

} // namespace

std::unique_ptr<cmPackageInfoReader> cmPackageInfoReader::Read(
  std::string const& path, cmPackageInfoReader const* parent)
{
  // Read file and perform some basic validation:
  //   - the input is valid JSON
  //   - the input is a JSON object
  //   - the input has a "cps_version" that we (in theory) know how to parse
  Json::Value data = ReadJson(path);
  if (!data.isObject() || !CheckSchemaVersion(data)) {
    return nullptr;
  }

  //   - the input has a "name" attribute that is a non-empty string
  Json::Value const& name = data["name"];
  if (!name.isString() || name.empty()) {
    return nullptr;
  }

  //   - the input has a "components" attribute that is a JSON object
  if (!data["components"].isObject()) {
    return nullptr;
  }

  std::string prefix = (parent ? parent->Prefix : DeterminePrefix(path, data));
  if (prefix.empty()) {
    return nullptr;
  }

  // Seems sane enough to hand back to the caller.
  std::unique_ptr<cmPackageInfoReader> reader{ new cmPackageInfoReader };
  reader->Data = std::move(data);
  reader->Prefix = std::move(prefix);
  reader->Path = path;

  // Determine other information we need to know immediately, or (if this is
  // a supplemental reader) copy from the parent.
  if (parent) {
    reader->ComponentTargets = parent->ComponentTargets;
    reader->DefaultConfigurations = parent->DefaultConfigurations;
  } else {
    reader->DefaultConfigurations = ReadList(reader->Data, "configurations");
  }

  return reader;
}

std::string cmPackageInfoReader::GetName() const
{
  return this->Data["name"].asString();
}

cm::optional<std::string> cmPackageInfoReader::GetVersion() const
{
  Json::Value const& version = this->Data["version"];
  if (version.isString()) {
    return version.asString();
  }
  return cm::nullopt;
}

std::vector<unsigned> cmPackageInfoReader::ParseVersion() const
{
  // Check that we have a version.
  cm::optional<std::string> const& version = this->GetVersion();
  if (!version) {
    return {};
  }

  std::vector<unsigned> result;
  cm::string_view remnant{ *version };

  // Check if we know how to parse the version.
  Json::Value const& schema = this->Data["version_schema"];
  if (schema.isNull() || cmStrCaseEq(schema.asString(), "simple"_s)) {
    // Keep going until we run out of parts.
    while (!remnant.empty()) {
      std::string::size_type n = remnant.find('.');
      cm::string_view part = remnant.substr(0, n);
      if (n == std::string::npos) {
        remnant = {};
      } else {
        remnant = remnant.substr(n + 1);
      }

      unsigned long const value = std::stoul(std::string{ part }, &n);
      if (n == 0 || value > std::numeric_limits<unsigned>::max()) {
        // The part was not a valid number or is too big.
        return {};
      }
      result.push_back(static_cast<unsigned>(value));
    }
  }

  return result;
}

std::string cmPackageInfoReader::ResolvePath(std::string path) const
{
  cmSystemTools::ConvertToUnixSlashes(path);
  if (cmHasPrefix(path, "@prefix@"_s)) {
    return cmStrCat(this->Prefix, path.substr(8));
  }
  if (!cmSystemTools::FileIsFullPath(path)) {
    return cmStrCat(cmSystemTools::GetFilenamePath(this->Path), '/', path);
  }
  return path;
}

void cmPackageInfoReader::SetOptionalProperty(cmTarget* target,
                                              cm::string_view property,
                                              cm::string_view configuration,
                                              Json::Value const& value) const
{
  if (!value.isNull()) {
    std::string fullprop;
    if (configuration.empty()) {
      fullprop = cmStrCat("IMPORTED_"_s, property);
    } else {
      fullprop = cmStrCat("IMPORTED_"_s, property, '_',
                          cmSystemTools::UpperCase(configuration));
    }

    target->SetProperty(fullprop, this->ResolvePath(value.asString()));
  }
}

void cmPackageInfoReader::SetTargetProperties(
  cmMakefile* makefile, cmTarget* target, Json::Value const& data,
  std::string const& package, cm::string_view configuration) const
{
  // Add compile and link features.
  for (std::string const& def : ReadList(data, "compile_features")) {
    AddCompileFeature(makefile, target, configuration, def);
  }

  for (std::string const& def : ReadList(data, "link_features")) {
    AddLinkFeature(makefile, target, configuration, def);
  }

  // Add compile definitions.
  for (std::string const& def : ReadList(data, "definitions")) {
    AppendProperty(makefile, target, "COMPILE_DEFINITIONS"_s, configuration,
                   def);
  }

  // Add include directories.
  for (std::string inc : ReadList(data, "includes")) {
    AppendProperty(makefile, target, "INCLUDE_DIRECTORIES"_s, configuration,
                   this->ResolvePath(std::move(inc)));
  }

  // Add link name/location(s).
  this->SetOptionalProperty(target, "LOCATION"_s, configuration,
                            data["location"]);

  this->SetOptionalProperty(target, "IMPLIB"_s, configuration,
                            data["link_location"]);

  this->SetOptionalProperty(target, "SONAME"_s, configuration,
                            data["link_name"]);

  // Add link languages.
  for (std::string const& lang : ReadList(data, "link_languages")) {
    auto const li = Languages.find(cmSystemTools::LowerCase(lang));
    if (li != Languages.end()) {
      AppendProperty(makefile, target, "LINK_LANGUAGES"_s, configuration,
                     li->second);
    }
  }

  // Add transitive dependencies.
  for (std::string const& dep : ReadList(data, "requires")) {
    AppendProperty(makefile, target, "LINK_LIBRARIES"_s, configuration,
                   NormalizeTargetName(dep, package));
  }

  for (std::string const& dep : ReadList(data, "link_requires")) {
    std::string const& lib =
      cmStrCat("$<LINK_ONLY:"_s, NormalizeTargetName(dep, package), '>');
    AppendProperty(makefile, target, "LINK_LIBRARIES"_s, configuration, lib);
  }
}

cmTarget* cmPackageInfoReader::AddLibraryComponent(
  cmMakefile* makefile, cmStateEnums::TargetType type, std::string const& name,
  Json::Value const& data, std::string const& package) const
{
  // Create the imported target.
  cmTarget* const target = makefile->AddImportedTarget(name, type, false);

  // Set target properties.
  this->SetTargetProperties(makefile, target, data, package, {});
  auto const& cfgData = data["configurations"];
  for (auto ci = cfgData.begin(), ce = cfgData.end(); ci != ce; ++ci) {
    this->SetTargetProperties(makefile, target, *ci, package, IterKey(ci));
  }

  // Set default configurations.
  if (!this->DefaultConfigurations.empty()) {
    target->SetProperty("IMPORTED_CONFIGURATIONS",
                        cmJoin(this->DefaultConfigurations, ";"_s));
  }

  return target;
}

bool cmPackageInfoReader::ImportTargets(cmMakefile* makefile,
                                        cmExecutionStatus& status)
{
  std::string const& package = this->GetName();

  // Read components.
  Json::Value const& components = this->Data["components"];

  for (auto ci = components.begin(), ce = components.end(); ci != ce; ++ci) {
    cm::string_view const& name = IterKey(ci);
    std::string const& type =
      cmSystemTools::LowerCase((*ci)["type"].asString());

    // Get and validate full target name.
    std::string const& fullName = cmStrCat(package, "::"_s, name);
    {
      std::string msg;
      if (!makefile->EnforceUniqueName(fullName, msg)) {
        status.SetError(msg);
        return false;
      }
    }

    cmTarget* target = nullptr;
    if (type == "symbolic"_s) {
      // TODO
    } else if (type == "dylib"_s) {
      target = this->AddLibraryComponent(
        makefile, cmStateEnums::SHARED_LIBRARY, fullName, *ci, package);
    } else if (type == "module"_s) {
      target = this->AddLibraryComponent(
        makefile, cmStateEnums::MODULE_LIBRARY, fullName, *ci, package);
    } else if (type == "archive"_s) {
      target = this->AddLibraryComponent(
        makefile, cmStateEnums::STATIC_LIBRARY, fullName, *ci, package);
    } else if (type == "interface"_s) {
      target = this->AddLibraryComponent(
        makefile, cmStateEnums::INTERFACE_LIBRARY, fullName, *ci, package);
    } else {
      makefile->IssueMessage(MessageType::WARNING,
                             cmStrCat("component "_s, fullName,
                                      " has unknown type "_s, type,
                                      " and was not imported"_s));
    }

    if (target) {
      this->ComponentTargets.emplace(std::string{ name }, target);
    }
  }

  // Read default components.
  std::vector<std::string> const& defaultComponents =
    ReadList(this->Data, "default_components");
  if (!defaultComponents.empty()) {
    std::string msg;
    if (!makefile->EnforceUniqueName(package, msg)) {
      status.SetError(msg);
      return false;
    }

    cmTarget* const target = makefile->AddImportedTarget(
      package, cmStateEnums::INTERFACE_LIBRARY, false);
    for (std::string const& name : defaultComponents) {
      std::string const& fullName = cmStrCat(package, "::"_s, name);
      AppendProperty(makefile, target, "LINK_LIBRARIES"_s, {}, fullName);
    }
  }

  return true;
}

bool cmPackageInfoReader::ImportTargetConfigurations(
  cmMakefile* makefile, cmExecutionStatus& status) const
{
  std::string const& configuration = this->Data["configuration"].asString();

  if (configuration.empty()) {
    makefile->IssueMessage(MessageType::WARNING,
                           cmStrCat("supplemental file "_s, this->Path,
                                    " does not specify a configuration"_s));
    return true;
  }

  std::string const& package = this->GetName();
  Json::Value const& components = this->Data["components"];

  for (auto ci = components.begin(), ce = components.end(); ci != ce; ++ci) {
    // Get component name and look up target.
    cm::string_view const& name = IterKey(ci);
    auto const& ti = this->ComponentTargets.find(std::string{ name });
    if (ti == this->ComponentTargets.end()) {
      status.SetError(cmStrCat("component "_s, name, " was not found"_s));
      return false;
    }

    // Read supplemental data for component.
    this->SetTargetProperties(makefile, ti->second, *ci, package,
                              configuration);
  }

  return true;
}
