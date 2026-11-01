# RenderItemsTests

CPU-only tests for D3D12 render item construction. The executable links Renderer.D3D12, RenderInterface, Core and graphics dependencies, without Engine or Editor libraries.

Build Core, RenderInterface and Renderer.D3D12 with Debug/x64 first. Then run from the repository root in a Visual Studio developer shell:

```powershell
MSBuild.exe tests/Renderer.D3D12/RenderItemsTests.vcxproj /p:Configuration=Debug /p:Platform=x64
./bin/RenderItemsTests/RenderItemsTests.exe
```

Exit code 0 means success. The final out-of-range error is intentional: the test verifies invalid primitive indices are rejected.

Coverage: current/previous world matrices, material and texture mapping, draw offsets, static/skinned/sky layers, reordered bone palettes, missing history fallback, empty frame clearing, invalid primitive indices, vertex layout sizes.
