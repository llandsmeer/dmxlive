ip = "192.168.1.200"
nleds = 180

amax = 0
hist = []
function color(i, t) {
    if (i == nleds-1)
        hist[i] = hsv(t, 1, amp)
    else
        hist[i] = hist[i+1]
    return hist[nleds - i - 1]
}
