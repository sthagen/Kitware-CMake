RunCMake_check_slnx("${RunCMake_TEST_BINARY_DIR}/CustomTypePlatform.slnx" [[
^<\?xml version="1\.0" encoding="UTF-8"\?>
<Solution>
  <Configurations>
    <BuildType Name="Debug"/>
    <BuildType Name="Release"/>
    <BuildType Name="MinSizeRel"/>
    <BuildType Name="RelWithDebInfo"/>
    <Platform Name="[^"]+"/>
  </Configurations>
  <Project Path="ALL_BUILD\.vcxproj" Id="[0-9a-f-]+">
    <BuildDependency Project="ZERO_CHECK\.vcxproj"/>
    <BuildDependency Project="external.project"/>
    <Build Solution="Debug\|\*" Project="false"/>
    <Build Solution="Release\|\*" Project="false"/>
    <Build Solution="MinSizeRel\|\*" Project="false"/>
    <Build Solution="RelWithDebInfo\|\*" Project="false"/>
  </Project>
  <Project Path="ZERO_CHECK\.vcxproj" Id="[0-9a-f-]+"/>
  <Project Path="external.project" Id="[0-9a-f-]+">
    <BuildDependency Project="ZERO_CHECK.vcxproj"/>
    <Platform Project="Custom Platform"/>
  </Project>
</Solution>$]])
