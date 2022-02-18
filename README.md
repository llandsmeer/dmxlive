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
