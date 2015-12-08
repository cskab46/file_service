File Service
============
Third assignment for Distrubuted Systems class. A simple distributed file service, created with the Spread Framework.
It provides client, server and slave applications. The file service supports creation, removal, read and write of files. 
The files might optionally have redundant copies which are defined during the creation step. 

Building
-----
* Qt5 is needed to build the gui_client project. 
* CMake version 2.8 is also needed.
* Spread toolkit must be compiled and accessible by the linker. Tested with version 4.0.0.

To compile the project simply run
```
cmake /path_to_source
make .
```
The executables client, gui_client, server and slave will be generated.

Usage
-----
0. Run the Spread daemon.

0. Run a proxy server giving it a priority argument. An election will immediately take place as soon as a new server joins the group.
  ```
  ./server _number_
  ```
0. Run a slave, by simply executing the slave application. It will create a folder with the slave id, given by Spread, and all of the file operations on that slave will persist on this folder.
  
  ```
  ./slave
  ```
0. To perform file operations you can either run the command line client:
  * To create a file named _file_name_ with redundancy _redundancy_:
  ```
  ./client create file_name redundancy
  ```
  * To remove a file named _file_name_:
  ```
  ./client remove file_name
  ```
  * To read a file named _file_name_:
  ```
  ./client read file_name
  ```
  * To write _data_ to a file named _file_name_:
  ```
  ./client write file_name data
  ```
or the GUI client, whose usage is similar to a file manager folder:
  ```
  ./gui_client
  ```
