ip = "192.168.1.92"
nleds = 150

function color(i, t) {
    return hsv(i * 20 * PI / nleds + 5 * t)
}

