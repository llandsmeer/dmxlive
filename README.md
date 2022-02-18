# dmxlive

Minimal livecoding environment for WLED over DMX

```
git clone https://github.com/llandsmeer/dmxlive
cd dmxlive
make
```

Open `scene.js` in your text editor of choice
and edit the `ip` and `nleds` values to match
your DMX server, then execute `./dmxlive`.
Whevener you update `scene.js`, dmxlive will
reload the file allowing you to perform live coding.

```javascript
ip = "192.168.1.92"
nleds = 150

function color(i, t) {
    return i % 2 == 0 ? 0 : 255
}
```

The `color` function can return, any javascript value, and the program tries to make it into a color.

# some examples

A one-dimensional seashore

```javascript
function color(i, t) {
    if (i > 100)
        return 'deepskyblue'
    else if (i > 50 + 10*sin(2*t))
        return 'aquamarine'
    else
        return 'sandybrown'
}
```

Draw a repeating rainbow

```javascript
function color(i, t) {
    r = sin(0.5*i + 10*t)
    g = sin(0.5*i + 10*t + 2*PI/3)
    b = sin(0.5*i + 10*t + 4*PI/3)
    return [128+128*r, 128+128*g, 128+128*b]
}
```

# would not have been possible without:

 - libe131
 - duktape
 - named colors from [p5js](https://github.com/processing/p5.js/blob/v1.4.1/src/color/p5.Color.js#L14)
