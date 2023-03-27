# TemplateProject_NG
Template plugin for Commonlib SSE-NG development

## Requirements
* [CMake](https://cmake.org/)
	* Add this to your `PATH`
* [The Elder Scrolls V: Skyrim Special Edition](https://store.steampowered.com/app/489830)
	* Add the environment variable `CompiledPluginsPath` to point to the folder where you want the .dll to be copied after building
* [Vcpkg](https://github.com/microsoft/vcpkg)
	* Add the environment variable `VCPKG_ROOT` with the value as the path to the folder containing vcpkg
* [Visual Studio Community 2022](https://visualstudio.microsoft.com/)
	* Desktop development with C++
* [CommonLibSSE-NG](https://github.com/CharmedBaryon/CommonLibSSE-NG/tree/v3.4.0)
	* Add the environment variable `CommonLibSSEPath_NG` with the value as the path to the folder containing CommonLibSSE-NG
  
## Building

Run the following command:

`git clone https://github.com/D7ry/TemplateProject_NG`

relocate to `TemplateProject_NG` directory.

in `CMakeLists.txt` and `vcpkg.json`, change `templateproject` to the desired project name.

Run the following command:

```
cmake --preset vs2022-windows
```
Alternatively, simply double-click ``build.bat``


relocate to `build` directory, open generated .sln file with Visual Studio 2022 to start editing.
