## dmxlive

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

## microphone input

Optionally enabled with the `ENABLE_AUDIO` preprocessor flag, which is set by default.
Depends on the OpenAL library.

```
sudo apt install libopenal-dev
```

If it is not available, you can use `make build/dmxlive.noaudio`
to build without microphone input support.

For the moment, RMS microphone input is available as the global variable `amp`
in `scene.js`. True RMS is a not a good measure of loudness, but I haven't gotten around
to implementing something better.

## some examples

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

Another way to draw a rainbow (10 repeats)

```javascript
function color(i, t) {
    return hsv(i * 20 * PI / nleds + 5 * t)
}
```

Grayscale Kuramoto model with time-varying coupling coefficient

```javascript
dt = 0.1
phase = []
frequency = []
for (i = 0; i < nleds; i++) {
    phase[i] = 2 * PI * random()
    frequency[i] = random()
}

function color(i, t) {
    K = 0.5 + 0.5 * sin(2*t)
    phase[i] += dt * (
        frequency[i] +
        K * sin(phase[(i+1)%nleds] - phase[i]) +
        K * sin(phase[(i-1+nleds)%nleds] - phase[i])
        )
    return 128 + 128 * sin(phase[i])
}
```

Volume meter

```javascript
function color(i, t) {
    if (i > nleds * amp) return
    if (i > nleds * 0.8) return 'red'
    if (i > nleds * 0.5) return 'orange'
                         return 'green'
}
```

Visualize sounds over time in space

```javascript
amax = 0
hist = []
function color(i, t) {
    if (i == nleds-1)
        hist[i] = hsv(t, 1, amp)
    else
        hist[i] = hist[i+1]
    return hist[nleds - i - 1]
}
```

## would not have been possible without:

 - libe131
 - duktape
 - named colors from [p5js](https://github.com/processing/p5.js/blob/v1.4.1/src/color/p5.Color.js#L14)
