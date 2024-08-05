The `sai2-interfaces-enum` Element
==================================
The `sai2-interfaces-enum` element lets you set specific values into a Redis 
key, i.e. an `enum` type in most languages.

## Usage
```
<sai2-interfaces-enum key="...">
  <option value="a">A</option>
  <option value="b">B</option>
  <option value="c">C</option>
  ...
<sai2-interfaces-enum>
```

Specify each valid option as child HTML `<option></option>` tags. The `value`
attribute is used as the value put into the Redis database, and the text 
between the tags is the friendly display text.

## Attributes
* `key`: Required. Specifies which Redis key to get/set to the user-specified 
options. If the Redis key does not exist, then an error will be thrown.
* `display`: Optional. Specifies what text to put next to the dropdown menu.

## Example
Open up `redis-cli`, and let's set the key 
`sai2::interfaces::tutorial::enum_key` to "Apples". Let's say that the valid 
options for this key are `Apples`, `Bananas`, or `Oranges`.
```
wjen@wjen-desktop:~$ redis-cli
127.0.0.1:6379> set sai2::interfaces::tutorial::enum_key Apples
OK
127.0.0.1:6379> get sai2::interfaces::tutorial::enum_key
"Apples"
127.0.0.1:6379> 
```

Let's write the HTML for this and paste it into the body of 
[02-enum.html](./02-enum.html)
```
<sai2-interfaces-enum key="sai2::interfaces::tutorial::enum_key">
  <option>Apples</option>
  <option>Bananas</option>
  <option>Oranges</option>
</sai2-interfaces-enum>
```

We launch our server from the SAI2-Interfaces repository root.
```
wjen@wjen-desktop:~/sai2/core/sai2-interfaces$ python3 interface/server.py docs/02-enum/02-enum.html
 * Restarting with stat
 * Debugger is active!
 * Debugger PIN: 142-257-956
(12039) wsgi starting up on http://127.0.0.1:8000
```

Open your browser to `localhost:8000`, and you should see something like this:

![enum_key with dropdown that has Apples selected](./enum-initial.png)

Select the `Oranges` option in the dropdown, and we should be able to see the 
Redis key update:
```
127.0.0.1:6379> get sai2::interfaces::tutorial::enum_key
"Oranges"
127.0.0.1:6379> 
```

We can also change the associated text next to the dropdown by using the 
`display` attribute:
```
<sai2-interfaces-enum display="My Favorite Fruit"  key="sai2::interfaces::tutorial::enum_key">
  <option value="Apples">Apples</option>
  <option value="Bananas">Bananas</option>
  <option value="Oranges">Oranges</option>
</sai2-interfaces-enum>
```

After refreshing the webpage, you should see something like this:

![My Favorite Fruit with dropdown that has Oranges selected](./enum-initial.png)
