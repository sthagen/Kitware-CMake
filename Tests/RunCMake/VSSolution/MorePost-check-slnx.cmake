RunCMake_check_slnx("${RunCMake_TEST_BINARY_DIR}/MorePost.slnx" [[
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
    <Build Solution="Debug\|\*" Project="false"/>
    <Build Solution="Release\|\*" Project="false"/>
    <Build Solution="MinSizeRel\|\*" Project="false"/>
    <Build Solution="RelWithDebInfo\|\*" Project="false"/>
  </Project>
  <Project Path="ZERO_CHECK\.vcxproj" Id="[0-9a-f-]+"/>
  <Properties Name="TestSec2" Scope="PostLoad">
    <Properties Name="Key1" Value="Value1"/>
    <Properties Name="Key2" Value="Value with spaces"/>
  </Properties>
  <Properties Name="TestSec4" Scope="PostLoad">
    <Properties Name="Key6" Value="Value1"/>
    <Properties Name="Key7" Value="Value with spaces"/>
    <Properties Name="Key8" Value="ValueWithoutSpaces"/>
  </Properties>
</Solution>$]])
