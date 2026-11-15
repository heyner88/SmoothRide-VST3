$ErrorActionPreference = 'Stop'
if (-not (Test-Path '.\JUCE\CMakeLists.txt')) {
    git clone --depth 1 --branch 8.0.12 https://github.com/juce-framework/JUCE.git JUCE
    if ($LASTEXITCODE -ne 0) { throw 'JUCE download failed' }
}
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed' }
cmake --build build --config Release --target SmoothRide_VST3
if ($LASTEXITCODE -ne 0) { throw 'Build failed' }
Write-Host 'Build complete. Find SmoothRide.vst3 in build\SmoothRide_artefacts\Release\VST3.'
