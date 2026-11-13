# Downloads the build dependencies into deps/ (or -Dest), pinned to the versions ReaGlass was built with.
# Windows PowerShell 5.1+.   Bump the pins below when you upgrade a dependency.
#   REAPER SDK         github.com/justinfrankel/reaper-sdk   (zlib-style)
#   webview/webview    github.com/webview/webview            (MIT)
#   WebView2 SDK       NuGet Microsoft.Web.WebView2          (BSD-3-Clause)
param([string]$Dest = (Join-Path $PSScriptRoot '..\deps'))
$ErrorActionPreference = 'Stop'
New-Item -ItemType Directory -Force $Dest | Out-Null
$Dest = (Resolve-Path $Dest).Path

$gitDeps = @(
  @{ Name = 'reaper-sdk'; Url = 'https://github.com/justinfrankel/reaper-sdk.git'; Sha = 'b2ca1ec92c9a7864fb72346037986a384971ed03' },
  @{ Name = 'webview';    Url = 'https://github.com/webview/webview.git';          Sha = 'f244320ab0d82fdab98a094e02996fc26d0ffd0c' }
)
$Wv2Version = '1.0.4314.35'

foreach ($d in $gitDeps) {
  $path = Join-Path $Dest $d.Name
  if (Test-Path $path) { Write-Host "$($d.Name): already present"; continue }
  git init -q $path;                                          if ($LASTEXITCODE) { throw 'git init failed' }
  git -C $path remote add origin $d.Url
  git -C $path fetch -q --depth 1 origin $d.Sha;              if ($LASTEXITCODE) { throw "git fetch failed: $($d.Url)" }
  git -C $path checkout -q FETCH_HEAD;                        if ($LASTEXITCODE) { throw 'git checkout failed' }
  Write-Host "$($d.Name): $($d.Sha.Substring(0,8))"
}

$wv2 = Join-Path $Dest 'wv2'
if (Test-Path $wv2) { Write-Host 'wv2: already present' }
else {
  $zip = Join-Path $env:TEMP 'webview2-sdk.nupkg.zip'
  Invoke-WebRequest "https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2/$Wv2Version" -OutFile $zip
  Expand-Archive $zip -DestinationPath $wv2
  Remove-Item $zip
  Write-Host "wv2: $Wv2Version"
}
Write-Host 'Done. Now run build.bat'
