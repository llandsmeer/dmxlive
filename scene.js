ip = "192.168.1.92"
nleds = 150

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

