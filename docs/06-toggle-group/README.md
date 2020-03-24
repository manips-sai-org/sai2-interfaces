The `sai2-interfaces-toggle-group` Element
==========================================
The `sai2-interfaces-toggle-group` element is a container that can switch between two different sets of elements based on a checkbox, which may or may not be controlled by a Redis key.

More concretely, suppose you have a checkbox element. If it's checked, you display a group of elements. If the box is not checked, you display a different group of elements. This checkbox can be connected to a Redis key, but it does not need to be.

## Usage
```
<sai2-interfaces-toggle-group name="...">
  <sai2-interfaces-toggle-group-enabled>
    <element></element>
    <element></element>
    <element></element>
    ...
  </sai2-interfaces-toggle-group-enabled>
  <sai2-interfaces-toggle-group-disabled>
    <element></element>
    <element></element>
    <element></element>
    ...
  </sai2-interfaces-toggle-group-disabled>
</sai2-interfaces-toggle-group>
```

You must place all elements in the `sai2-interfaces-toggle-group-enabled` element if you want them to show when the checkbox is checked. Similarly, place all elements in the `sai2-interfaces-toggle-group-disabled` element if you want them to show when the checkbox is *not* checked.

## Attributes
* `name`: Required. This is the text that is displayed next to the checkbox and is thus used to identify the toggle group on the interface.
* `key`: Optional. The checkbox can be hooked up to a Redis key that holds a boolean value. Otherwise, the checkbox value is held in memory and is initialized to unchecked.

## Example
For this example, we're going show a slider for `sai2::interfaces::tutorial::scalar_key` when the checkbox is checked and a slider for `sai2::interfaces::tutorial::vector_key` when the checkbox is not checked. The corresponding HTML is below:
```
<sai2-interfaces-toggle-group name="Check me!">
  <sai2-interfaces-toggle-group-enabled>
    <sai2-interfaces-slider key="sai2::interfaces::tutorial::scalar_key" 
      display="scalar" min="0" max="5" step="0.1">
    </sai2-interfaces-slider>
  </sai2-interfaces-toggle-group-enabled>
  <sai2-interfaces-toggle-group-disabled>
    <sai2-interfaces-slider key="sai2::interfaces::tutorial::vector_key" 
      display="vector" min="0" max="5" step="[0.1,0.1]">
    </sai2-interfaces-slider>
  </sai2-interfaces-toggle-group-disabled>
</sai2-interfaces-toggle-group>
```

First, let's make sure our keys are in redis. There's a helper script in this folder that you can run to do this:
```
wjen@wjen-desktop:~/sai2/core/sai2-interfaces$ python3 docs/06-toggle-group/writekeys.py 
wjen@wjen-desktop:~/sai2/core/sai2-interfaces$ 
```

Now let's boot up the server:
```
wjen@wjen-desktop:~/sai2/core/sai2-interfaces$ python3 interface/server.py docs/06-toggle-group/06-toggle-group.html 
 * Restarting with stat
 * Debugger is active!
 * Debugger PIN: 142-257-956
(25568) wsgi starting up on http://127.0.0.1:8000
```

Open up your browser to `localhost:8000`, and you should see something like this:

![toggle group initial](./toggle-group-initial.png)

As expected, when the checkbox is not checked, we show the sliders for the `sai2::interfaces::tutorial::vector_key` Redis key. 

Now let's click the checkbox. You should see the slider switch to the `sai2::interfaces::tutorial::scalar_key`:

![toggle group checked](./toggle-group-toggled.png)

Finally, you can add the `key="sai2::interfaces::tutorial::toggle_me"` attribute to the `sai2-interfaces-toggle-group` element in [06-toggle-group.html](./06-toggle-group.html). You should see the checkbox value persist across webpage refreshes.
