ip = "192.168.1.92"
nleds = 150

amax = 0
hist = []
l = 0
function color(i, t) {
    if (amp > amax) l += 0.1
    amax = min(max(amp, 0.9999*amax), 0.15)
    if (i == nleds-1) {
        if (amax < 0.0005)
            hist[i] = 0
        else
            hist[i] = hsv(l, 1, amp / amax)
    }
    else
        hist[i] = hist[i+1]
    return hist[nleds - i - 1]
}

