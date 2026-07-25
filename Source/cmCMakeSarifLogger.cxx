/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file LICENSE.rst or https://cmake.org/licensing for details.  */
#include "cmCMakeSarifLogger.h"

#include <cstddef>
#include <limits>
#include <unordered_map>
#include <utility>
#include <vector>

#include <cm/string_view>

#include "cmsys/FStream.hxx"
#include "cmsys/String.h"

#include "cmDiagnostics.h"
#include "cmListFileCache.h"
#include "cmMessageType.h"
#include "cmMessenger.h"
#include "cmSarif.h"
#include "cmState.h"
#include "cmStringAlgorithms.h"
#include "cmSystemTools.h"
#include "cmValue.h"
#include "cmVersionConfig.h"
#include "cmake.h"

// CMake-specific SARIF helpers
namespace {

constexpr char const* CMakeSarifOutputFlag = "CMAKE_EXPORT_SARIF";
constexpr char const* DefaultSarifFile = ".cmake/sarif/cmake.sarif";

/// @brief Express the location of a `cmListFileContext` in SARIF
/// @param[in] uriBaseIds A list of logical base directory names and their path
///
/// Build a SARIF location object detailing the location data available from a
/// context. More specific information like the region (line number) and
/// function call name will be included if available.
///
/// SARIF requests that paths are given relative to a logical base for
/// relocatability. Context locations will be made relative to a logical base
/// iff they fall under one of the directories listed in the `uriBaseIds`
/// map. Bases are tried in order.
cmSarif::Location LocationFromContext(
  cmListFileContext const& lfc,
  std::vector<std::pair<cm::string_view, cm::string_view>> const&
    uriBaseIds = {})
{
  cmSarif::Location location;
  location.Physical.Artifact.Uri = lfc.FilePath;

  // SARIF requests that paths are given relative to a logical base for
  // relocatability. Check if these files are under any of the bases, if
  // provided.
  for (auto const& baseUri : uriBaseIds) {
    std::string relative = cmSystemTools::RelativeIfUnder(
      std::string(baseUri.second), location.Physical.Artifact.Uri);
    if (relative != location.Physical.Artifact.Uri) {
      location.Physical.Artifact.Uri = relative;
      location.Physical.Artifact.UriBaseId = std::string(baseUri.first);
    }
  }

  if (!lfc.Name.empty()) {
    location.Logical.emplace_back(
      cmSarif::LogicalLocation{ lfc.Name, cmSarif::LocationKind::Function });
  }

  // Add info about the region within the file depending on how specific the
  // context is. Watch for deferred call and variable watch placeholders or
  // a zero, which indicates the start of processing a list file.
  if (lfc.Line == cmListFileContext::DeferPlaceholderLine) {
    location.Message = cmSarif::Message{ "DEFERRED" };
  } else if (lfc.Line > 0 && lfc.Line != std::numeric_limits<long>::max()) {
    cmSarif::Region region;
    region.StartLine = lfc.Line;
    location.Physical.ArtifactRegion = region;
  }

  return location;
}

cm::optional<cmSarif::Location> LastLocation(
  cmListFileBacktrace backtrace,
  std::vector<std::pair<cm::string_view, cm::string_view>> const&
    uriBaseIds = {})
{
  if (backtrace.Empty()) {
    return {};
  }
  return LocationFromContext(backtrace.Top(), uriBaseIds);
}

cm::optional<cmSarif::Stack> StackFromBacktrace(
  cmListFileBacktrace bt,
  std::vector<std::pair<cm::string_view, cm::string_view>> const&
    uriBaseIds = {})
{
  if (bt.Empty()) {
    return {};
  }

  cmSarif::Stack stack;
  for (; !bt.Empty(); bt = bt.Pop()) {
    cmSarif::Location topLocation = LocationFromContext(bt.Top(), uriBaseIds);

    // If the location doesn't have a specific region, this entry is a
    // placeholder and should not appear in the call stack.
    if (!topLocation.Message && !topLocation.Physical.ArtifactRegion) {
      continue;
    }

    cmSarif::StackFrame frame;
    frame.Location = std::move(topLocation);
    stack.Frames.emplace_back(std::move(frame));
  }
  return stack;
}

cmSarif::Tool CreateCMakeTool()
{
  cmSarif::ToolComponent cmDriver;
  cmDriver.Name = "CMake";
  cmDriver.Version = CMake_VERSION;

  return cmSarif::Tool{ cmDriver };
}

std::string RuleIdForMessageType(MessageType type,
                                 cmDiagnosticCategory category)
{
  cm::string_view name = cmDiagnostics::GetCategoryString(category);
  if (!name.empty()) {
    // Strip the "CMD_" prefix from the category name and convert to PascalCase
    std::string sarifIdName;
    bool nextWord = true;
    for (char c : name.substr(4)) {
      if (c == '_') {
        nextWord = true;
        continue;
      }
      if (nextWord) {
        sarifIdName += c;
        nextWord = false;
      } else {
        sarifIdName += cmsysString_tolower(c);
      }
    }
    return cmStrCat("CMake.", sarifIdName);
  }

  // Fall back to message type if not a diagnostic
  switch (type) {
    case MessageType::FATAL_ERROR:
      return "CMake.FatalError";
    case MessageType::INTERNAL_ERROR:
      return "CMake.InternalError";
    case MessageType::WARNING:
      return "CMake.Warning";
    default:
      return "";
  }
}

cm::string_view NameForMessageType(MessageType type,
                                   cmDiagnosticCategory category)
{
  if (category != cmDiagnostics::CMD_NONE) {
    return cmDiagnostics::GetCategoryString(category);
  }
  switch (type) {
    case MessageType::FATAL_ERROR:
      return "CMake Error";
    case MessageType::INTERNAL_ERROR:
      return "CMake Internal Error";
    case MessageType::WARNING:
      return "CMake Warning";
    default:
      return "";
  }
}

cmSarif::ReportingDescriptor ReportingDescriptorForMessageType(
  MessageType type, cmDiagnosticCategory category)
{
  cmSarif::ReportingDescriptor rd;
  rd.Id = RuleIdForMessageType(type, category);
  rd.Name = NameForMessageType(type, category);
  return rd;
};

cmSarif::ResultSeverityLevel SarifLevelFromMessageType(MessageType type)
{
  switch (type) {
    case MessageType::FATAL_ERROR:
    case MessageType::INTERNAL_ERROR:
      return cmSarif::ResultSeverityLevel::Error;
    case MessageType::WARNING:
      return cmSarif::ResultSeverityLevel::Warning;
    default:
      return cmSarif::ResultSeverityLevel::Note;
  }
}

} // namespace

cmCMakeSarifLogger::cmCMakeSarifLogger(cmake& cm)
  : CM(cm)
{
  if (this->CM.GetState()->GetRole() == cmState::Role::Project) {
    cm.MarkCliAsUsed(CMakeSarifOutputFlag);
  }
}

cmCMakeSarifLogger::~cmCMakeSarifLogger()
{
  this->GenerateForRun();
}

cm::optional<std::string> cmCMakeSarifLogger::FileOutputPath() const
{
  // If a SARIF path was specified via CLI, use it. Otherwise, check whether
  // logging is enabled via the project cache variable and use the default
  // path if so.
  if (cm::optional<std::string> specifiedPath = this->CM.GetSarifFilePath()) {
    return specifiedPath;
  }
  if (this->CM.GetState()->GetRole() == cmState::Role::Project &&
      this->CM.GetCacheDefinition(CMakeSarifOutputFlag).IsOn()) {
    return cmStrCat(this->CM.GetHomeOutputDirectory(), '/', DefaultSarifFile);
  }
  return cm::nullopt;
}

bool cmCMakeSarifLogger::WriteFile(std::string const& path,
                                   bool createParentDirectories) const
{
  if (createParentDirectories) {
    if (!cmSystemTools::MakeDirectory(cmSystemTools::GetFilenamePath(path))
           .IsSuccess()) {
      return false;
    }
  }

  cmsys::ofstream outputFile(path);
  if (!outputFile.good()) {
    return false;
  }

  // Run object to build
  cmSarif::Run run;
  run.Tool = CreateCMakeTool();

  // Helper to add rules to the run as encountered in results and get their
  // index for reporting
  std::unordered_map<std::string, std::size_t> ruleIndices;
  auto use_rule = [&](MessageType type, cmDiagnosticCategory category) {
    std::string category_name = RuleIdForMessageType(type, category);
    auto result = ruleIndices.emplace(category_name, 0);
    if (result.second) {
      result.first->second = run.Tool.Driver.Rules.size();
      run.Tool.Driver.Rules.emplace_back(
        ReportingDescriptorForMessageType(type, category));
    }
    return *result.first;
  };

  // Make a prioritized list of base directories applicable in this context.
  // This is used for normalizing the paths of related locations.
  std::vector<std::pair<cm::string_view, cm::string_view>> uriBaseIds;

  std::string const& binDir = this->CM.GetHomeOutputDirectory();
  if (!binDir.empty()) {
    uriBaseIds.emplace_back("CMAKE_BINARY_DIR", binDir);
  }

  std::string const& homeDir = this->CM.GetHomeDirectory();
  if (!homeDir.empty()) {
    uriBaseIds.emplace_back("CMAKE_SOURCE_DIR", homeDir);
  }

  // Log the base directories for this run.
  for (auto const& base : uriBaseIds) {
    run.OriginalUriBaseIds.emplace(
      std::string(base.first),
      cmSarif::ArtifactLocation{ cmStrCat("file://", base.second, "/"), "" });
  }

  cmMessenger const& messenger = *this->CM.GetMessenger();
  for (auto const& message : messenger.GetDisplayedMessages()) {
    // SARIF should only emit diagnostic messages, not general messages/logs
    switch (message.Type) {
      case MessageType::MESSAGE:
      case MessageType::LOG:
      case MessageType::UNDEFINED:
        continue;
      default:
        break;
    }

    std::pair<std::string, std::size_t> ruleInfo =
      use_rule(message.Type, message.Category);

    cmSarif::Result result;
    result.RuleId = ruleInfo.first;
    result.RuleIndex = ruleInfo.second;
    result.Message = cmSarif::Message{ message.Text };
    result.Location = LastLocation(message.Backtrace, uriBaseIds);
    if (cm::optional<cmSarif::Stack> stack =
          StackFromBacktrace(message.Backtrace, uriBaseIds)) {
      result.Stacks.emplace_back(std::move(*stack));
    }
    result.Level = SarifLevelFromMessageType(message.Type);

    run.Results.emplace_back(std::move(result));
  }

  return cmSarif::WriteLog(path, run);
}

void cmCMakeSarifLogger::GenerateForRun() const
{
  cm::optional<std::string> path = this->FileOutputPath();
  if (!path) {
    return;
  }

  // If using the default path within the build dir, ensure parents are created
  bool const createParents = !this->CM.GetSarifFilePath().has_value();
  if (!this->WriteFile(*path, createParents)) {
    cmSystemTools::Error(cmStrCat("Failed to write SARIF log to ", *path));
  }
}
