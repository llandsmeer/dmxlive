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
    if (i > 100)
        return 'deepskyblue'
    if (i > 50 + 10*sin(2*t))
        return 'aquamarine'
    else:
        return 'sandybrown'
}

