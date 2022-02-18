ip = "192.168.1.92"
nleds = 150

function s(i, t, f) {
    x = sin(i/2 + f*t)
    y = sin(i/2 - f*t)
    l = x + y
    if (l < 0) return 0
    return l / 2
}

function color(i, t) {
    return 255 * s(i, t, 5) * s(i+t, t, 10)
}

