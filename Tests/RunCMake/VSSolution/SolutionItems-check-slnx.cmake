RunCMake_check_slnx("${RunCMake_TEST_BINARY_DIR}/SolutionItems.slnx" [[
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
  <Folder Name="/Outer Group/">
    <File Path="[^"]*/Tests/RunCMake/VSSolution/solution-item-1-1\.txt"/>
  </Folder>
  <Folder Name="/Outer Group/Inner Group/">
    <File Path="[^"]*/Tests/RunCMake/VSSolution/solution-item-2-1\.txt"/>
    <File Path="[^"]*/Tests/RunCMake/VSSolution/solution-item-2-2\.txt"/>
  </Folder>
  <Folder Name="/Solution Items/">
    <File Path="[^"]*/Tests/RunCMake/VSSolution/solution-item-0-1\.txt"/>
  </Folder>
</Solution>$]])

RunCMake_check_slnx("${RunCMake_TEST_BINARY_DIR}/SolutionItems/SolutionItemsSubproject.slnx" [[
<\?xml version="1\.0" encoding="UTF-8"\?>
<Solution>
  <Configurations>
    <BuildType Name="Debug"/>
    <BuildType Name="Release"/>
    <BuildType Name="MinSizeRel"/>
    <BuildType Name="RelWithDebInfo"/>
    <Platform Name="[^"]+"/>
  </Configurations>
  <Project Path="ALL_BUILD\.vcxproj" Id="[0-9a-f-]+">
    <BuildDependency Project="\.\./ZERO_CHECK\.vcxproj"/>
    <Build Solution="Debug\|\*" Project="false"/>
    <Build Solution="Release\|\*" Project="false"/>
    <Build Solution="MinSizeRel\|\*" Project="false"/>
    <Build Solution="RelWithDebInfo\|\*" Project="false"/>
  </Project>
  <Project Path="\.\./ZERO_CHECK\.vcxproj" Id="[0-9a-f-]+"/>
  <Folder Name="/Extraneous/">
    <File Path="[^"]*/Tests/RunCMake/VSSolution/SolutionItems/extraneous\.txt"/>
  </Folder>
</Solution>$]])
