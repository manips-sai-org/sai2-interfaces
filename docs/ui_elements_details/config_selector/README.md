The `sai2-interfaces-config-selector` Element
=========================================
The `sai2-interfaces-config-selector` element aims to select a file via the web browser, and to set the file path to this file as a key to redis, as well as a reset key. It is used in the main webui file automatically generated by the MainRedisInterface library.

![](./config_selector1.png)

You can chose a file by clicking the browse button or the box area next to it. When pressing the Reset button, two things happen:
* The reset_key value is set to 1
* The config_key value is set the the name of the selected file, prefixed with the path_prefix attribute value

## Usage

```
<sai2-interfaces-axis-setter config_key="..." reset_key="..." path_prefix="...">
</sai2-interfaces-axis-setter>
```

## Attributes

* `config_key`: Optional. The redis key to which the config file name will be published. If not present it will be set to `sai2::interfaces::simviz::config_file`
* `reset_key`: Optional. The boolean redis key to which we publish true when the Reset button is pressed.. If not present it will be set to `sai2::interfaces::simviz::reset`
* `path_prefix`: Optional. The prefix to the file name before publishing to redis

## Example

We provide an [example HTML file](./config_selector.html) containing the following html code for the element:

```
<sai2-interfaces-config-selector config_key="sai2::interfaces::tutorial::config_key"
	reset_key="sai2::interfaces::tutorial::reset_key" 
	path_prefix="path_to_sai2_install/sai2-interfaces/examples/config_folder/"/>
```

Run the server:

```
~/sai2/core/sai2-interfaces$ python3 ui/server.py docs/ui_elements_details/config_selector/config_selector.html 
```

Open a browser and go to `localhost:8000` .
Chose a file in your computer. For example the `panda_config.xml` file in the `sai2-interfaces/examples/config_files/` folder, and press the `Reset` button.

![](./config_selector2.png)

You can examine the keys `sai2::interfaces::tutorial::config_key` and `sai2::interfaces::tutorial::reset_key` that should containt respictively the values `path_to_sai2_install/sai2-interfaces/examples/config_folder/config_panda.xml` and `1` .

```
~$ redis-cli
127.0.0.1:6379> get get sai2::interfaces::tutorial::config_key
127.0.0.1:6379> get get sai2::interfaces::tutorial::reset_key
```