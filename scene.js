ip = "192.168.1.200"
nleds = 180

v = 0
function color(i, t) {
    if (i >= floor(nleds * v)) return
    if (i > nleds * 0.8) return 'red'
    if (i > nleds * 0.5) return 'orange'
                         return 'green'
}
