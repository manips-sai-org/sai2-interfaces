The `sai2-interfaces-tabs` Element
==========================================
The `sai2-interfaces-tabs` element is a container that enable to switch between multiple contents using tabs. The tabs are based on bootstrap tab elements.
* The tabs can be placed on the top (by default) or the left.
* The tabs can be connected to a redis key such that the value change depending on which tab is activated
* The tab color can be changed
* Additional content can be added to the tab area

## Usage

```
<sai2-interfaces-tabs name="..." key="..." color="...">
	<sai2-interfaces-tab-content name="..." value="...">
		...
	</sai2-interfaces-tab-content>
	<sai2-interfaces-tab-content name="..." value="...">
		...
	</sai2-interfaces-tab-content>
</sai2-interfaces-tabs>
```

The elements placed inside each `sai2-interfaces-tab-content` tags will show when that tab is selected.

## Attributes

For the `sai2-interfaces-tabs` element:
* `name`: Required.
* `key`: Optional. If the key attribute is provided, the values of the `sai2-interfaces-tab-content` elements will be set to the redis database in this key when the corresponding tab is clicked.
* `color`: Optional. To select the color of the tabs. Must be provided as `'rgb(0,0,0)'` format or `'#f4f4f4'` format. Default value is `'rgb(0, 110, 255)'`
* `tabsOnTheLeft`: Optional. If present, the tabs will be on the left of the contents. Otherwise, they will be on the top.

For the `sai2-interfaces-tab-content` elements:
* `name`: Required. This is the name that will appear on the tab button itself
* `value`: Optional. If a key is provided to the `sai2-interfaces-tabs` element, the value provided here will be set to the key when the tab is clicked

## Example

For this example, we're going show a slider for 
`sai2::interfaces::tutorial::scalar_key` when the **Scalar** tab is selected and a 
slider for `sai2::interfaces::tutorial::vector_key` when the **Vector** tab is selected.
The corresponding [HTML](./tabs.html) is below:

```
<sai2-interfaces-tabs name="tabs">
	<sai2-interfaces-tab-content name="Scalar">
		<sai2-interfaces-slider key="sai2::interfaces::tutorial::scalar_key" display="scalar" min="0"
			max="10" />
	</sai2-interfaces-tab-content>
	<sai2-interfaces-tab-content name="Vector">
		<sai2-interfaces-slider key="sai2::interfaces::tutorial::vector_key" display="vector" min="0"
			max="10" />
	</sai2-interfaces-tab-content>
</sai2-interfaces-tabs>
```

First, let's make sure our keys are in redis. There's a helper script in this 
folder that you can run to do this:

```
~/sai2/core/sai2-interfaces$ python3 docs/ui_elements_details/tabs/writekeys.py 
```

Now let's boot up the server:

```
~/sai2/core/sai2-interfaces$ python3 ui/server.py docs/ui_elements_details/tabs/tabs.html 
```

Open up your browser to `localhost:8000` , and you should see something like this:

![](./tabs1.png)

Click on the `Vector` tab and you should see:

![](./tabs2.png)

If you want to change the color of the tabs, you can add `color='#b30000'` to the attributes of the `sai2-interfaces-tabs` element.

```
<sai2-interfaces-tabs name="tabs" color='#b30000'>
```

Refresh the page to see:
![](./tabs3.png)

To place the tabs on the left, add the `tabsOnTheLeft` attribute:

```
<sai2-interfaces-tabs name="tabs" color='#b30000' tabsOnTheLeft>
```

Refresh the page to see:
![](./tabs4.png)

Finally, you can attach the tabs to a redis key. Add the `key` attribute to the `sai2-interfaces-tabs` element, and a `value` attribute to all the `sai2-interfaces-tab-content` elements. Replace the HTML with:

```		
<sai2-interfaces-tabs name="tabs" key="sai2::interfaces::tutorial::tab_key">
	<sai2-interfaces-tab-content name="Scalar" value="scalar">
		<sai2-interfaces-slider key="sai2::interfaces::tutorial::scalar_key" display="scalar" min="0"
			max="10" />
	</sai2-interfaces-tab-content>
	<sai2-interfaces-tab-content name="Vector" value="vector">
		<sai2-interfaces-slider key="sai2::interfaces::tutorial::vector_key" display="vector" min="0"
			max="10" />
	</sai2-interfaces-tab-content>
</sai2-interfaces-tabs>
```

Refresh the page, switch the tab, and check that the correct value was set to the redis key in the database:
```
~$ redis-cli
127.0.0.1:6379> get sai2::interfaces::tutorial::tab_key
```