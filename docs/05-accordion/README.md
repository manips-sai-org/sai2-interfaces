The `sai2-interfaces-accordion` Element
=======================================
The `sai2-interfaces-accordion` element is an accordion that is hooked up to a Redis key. An accordion, in the UI world, is an element that can be toggled to show or hide content. See [here](https://semantic-ui.com/modules/accordion.html#/definition) for more examples of accordions.

The `sai2-interfaces-accordion` element uses a Redis key that is always 0 (false) or 1 (true) to determine if it should display its contents. This is particularly useful in situations such as showing interpolation-related options only when interpolation is enabled.

## Usage
```
<sai2-interfaces-accordion key="..." displayName="...">
  <element></element>
  <element></element>
  <element></element>
  ...
</sai2-interfaces-accordion>
```

## Attributes
* `key`: Required. When this key is equal to 1, the accordion will show its contents.
* `displayName`: Required. This text is what is shown as the accordion header.

## Example
For our example, we're going to use `sai2::interfaces::tutorial::toggle_key` to determine if we should show the accordion or not. We will show sliders when the toggle key is set to true. The corresponding HTML is below:

```
<sai2-interfaces-accordion key="sai2::interfaces::tutorial::toggle_me" 
  displayName="toggle me">
  <sai2-interfaces-slider key="sai2::interfaces::tutorial::scalar_key" 
    display="scalar" min="0" max="5" step="0.1">
  </sai2-interfaces-slider>
  <sai2-interfaces-slider key="sai2::interfaces::tutorial::vector_key" 
    display="vector" min="0" max="5" step="[0.1,0.1]">
  </sai2-interfaces-slider>
</sai2-interfaces-accordion>
```

If you are not familar with the `sai2-interfaces-slider` syntax, please read through [its documentation here](../04-slider/04-slider.md).

First, let's make sure our keys are in redis. There's a helper script in this folder that you can run to do this:
```
wjen@wjen-desktop:~/sai2/core/sai2-interfaces$ python3 docs/05-accordion/writekeys.py 
wjen@wjen-desktop:~/sai2/core/sai2-interfaces$ 
```

Next, we run the server as follows:
```
wjen@wjen-desktop:~/sai2/core/sai2-interfaces$ python3 interface/server.py docs/05-accordion/05-accordion.html 
 * Restarting with stat
 * Debugger is active!
 * Debugger PIN: 142-257-956
(24518) wsgi starting up on http://127.0.0.1:8000
```

Open your browser to `localhost:8000` and you should see something like this:

![accordion initial](./accordion-initial.png)

Click the "toggle me" text, and you should be able to see it turn green and multiple sliders show up:

![accordion toggled](./accordion-toggled.png)

We can verify the `sai2::interfaces::tutorial::toggle_key`has been updated correctly:
```
wjen@wjen-desktop:~/sai2/core/sai2-interfaces$ redis-cli
127.0.0.1:6379> get sai2::interfaces::tutorial::toggle_me
"1"
127.0.0.1:6379> 
``` 

And that's pretty much it - you can place whatever HTML or SAI2-Interfaces component within the `sai2-interfaces-accordion` element!