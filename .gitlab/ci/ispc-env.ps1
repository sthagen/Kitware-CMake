$pwdpath = $pwd.Path
powershell -File ".gitlab/ci/ispc.ps1"
Set-Item -Force -Path "env:PATH" -Value "$pwdpath\.gitlab\ispc\bin;$env:PATH"
ispc --version
