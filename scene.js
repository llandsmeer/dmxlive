ip = "192.168.1.92"
nleds = 150

amax = 0
hist = []
function color(i, t) {
    amax = min(max(amp, 0.99999*amax), 0.05)
    if (i == nleds-1)
        hist[i] = hsv(t, 1, amp / amax)
    else
        hist[i] = hist[i+1]
    return hist[nleds - i - 1]
}

