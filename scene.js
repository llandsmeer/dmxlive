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
    r = sin(0.5*i + 10*t)
    g = sin(0.5*i + 10*t + 2*PI/3)
    b = sin(0.5*i + 10*t + 4*PI/3)
    return [128+128*r, 128+128*g, 128+128*b]
}

