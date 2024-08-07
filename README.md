# Sai2-interfaces

sai2-interfaces provides two main tools to enables users to quickly develop controllers and simulations with the sai libraries:
1. Wrappers for controller setup (around [sai2-primitives](https://github.com/manips-sai-org/sai2-primitives) library), simulation (around [sai2-simulation](https://github.com/manips-sai-org/sai2-simulation) library) and graphic visualization (around [sai2-graphics](https://github.com/manips-sai-org/sai2-graphics) library). The code for those is in the `src/` folder. Those wrappers enable:
  + Easy setup and configuration of controllers and simulated worlds from xml/urdf files
  + A lot of interactability with the controllers and simulation/visualization via redis
  + Built-in data logging functionality
  + Automatic generation of a html file for web-based ui interaction with the controller and simulation/visualization
2. A web browser based user interface to interact in real time with the controllers and simulation/visualization. The code for this is in the `ui/` folder. It is split in two parts:
  + The backend server running locally (no internet connection required at runtime) with python
  + The frontend composed of a collection of custom html/css elements such as sliders, toggle buttons, tabs to switch between controllers and so on

## Quickstart

The web based ui has been tested on Chrome/Chromium, Firefox and Safari.
The backend server requires Python 3.5+.

### Dependencies

The UI backend server depends on [redis](https://pypi.org/project/redis/), [Flask](https://pypi.org/project/Flask/), and [click](https://pypi.org/project/click/). You can install them as below:

```
pip3 install -r ui/requirements.txt
```

The C++ libraries depend on:
* [sai2-simulation](https://github.com/manips-sai-org/sai2-simulation)
* [sai2-graphics](https://github.com/manips-sai-org/sai2-graphics)
* [sai2-primitives](https://github.com/manips-sai-org/sai2-primitives)

You will need to build those first.

### Build instructions

Build the project with the following commands

```
mkdir build && cd build
cmake .. && make -j4
```

This will build the wrappers for the controllers and simviz, the MainInterface program, and will export `${SAI2-INTERFACES_UI_DIR}` , which points to the absolute path of the `ui/` folder in this repository. You can then use cmake's `FILE(CREATE_LINK)` macro to make a symlink to this directory from another application for easy access (recommended, for an example see the CMakeLists.txt file of [OpenSai](https://github.com/manips-sai-org/OpenSai)), or the `FILE(COPY)` macro to copy it.

### Running examples

### Documentation

For details on all the UI custom html elements, see the documentation [here](docs/ui_elements_details/ui_docs_menu.md).

## License

Currently pending licensing. PLEASE DO NOT DISTRIBUTE.

## Project contributors

* Mikael Jorda
* Keven Wang
* William Jen

## For questions, contact:

mjorda@stanford.edu or mjorda@jorda-tech.com
