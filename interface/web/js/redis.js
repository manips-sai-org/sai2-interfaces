
export function post_redis_key_val(key, val) {
  let fetchOptions = {
    method: 'POST',
    headers: new Headers({'Content-Type': 'application/json'}),
    mode: 'same-origin',
    body: JSON.stringify({key, val})
  };

  return fetch('/redis', fetchOptions)
    .catch(data => alert('set redis error: ' + toString(data)));
}

export function get_redis_val(key) {
  let fetchOptions = {
    method: 'GET',
    headers: new Headers({'Content-Type': 'application/json'}),
    mode: 'same-origin'
  };

  let params = new URLSearchParams({key: JSON.stringify(key)});

  return fetch('/redis?' + params.toString(), fetchOptions)
    .then(response => response.json())
    .catch(data => alert('get redis error: ' + toString(data)));
}

export function get_redis_all_keys() {
  let fetchOptions = {
    method: 'GET',
    headers: new Headers({'Content-Type': 'application/json'}),
    mode: 'same-origin'
  };

  return fetch('/redis/keys', fetchOptions)
    .then(response => response.json())
    .catch(data => alert('get redis error: ' + toString(data)));
}