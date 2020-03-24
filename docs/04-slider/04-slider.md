The `sai2-interfaces-slider` Element
====================================
The `sai2-interfaces-slider` element allows you to get/set the values of a 
scalar or vector tied to the Redis key.

## Usage
```
<sai2-interfaces-slider key="..." min="..." max="..." step="...">
</sai2-interfaces-slider>
```

If you specify a vector-valued key with `n` elements, `sai2-interfaces-slider` will create `n` sliders, one for each element.

## Attributes
* `key`: Required. The key for which to create a slider. Can be a scalar or vector.
* `display`: Optional. The friendly display name. If omitted, the default will be the `key`. If you have a vector-valued key, the name will be the value provided `display` plus an index, e.g. `display_name[0]`.
* `min`: Required. The minimum value for the scalar or vector. If you have a vector-valued key, you can specify a minimum value for each element in the vector.
* `max`: Required. The maximum value for the scalar or vector. If you have a vector-valued key, you can specify a maximum value for each element in the vector.
* `step`: Required. The amount to increment the slider value by when you drag the slider.

## Example
