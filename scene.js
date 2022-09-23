ip = "192.168.0.100"
ip2 = "192.168.0.104"
ip3 = "192.168.0.102"
nleds = 150
nleds3 = 360

r =0
g =0
b =0

q = 0
w = 0

function base(i) {
    if (i < 40) {
        return k44;
    } else if (i < 80) {
        return k45;
    } else if (i < 120) {
        return k46;
    } else if (i < 160) {
        return k47;
    } else {
        return k48;
    }
}

function keys(i){
    if (i < 20) {
        return k60;
    } else if (i < 40) {
        return k62;
    } else if (i < 60) {
        return k64;
    } else if (i < 80) {
        return k65;
    } else if (i < 100) {
        return k67;
    } else if (i < 120) {
        return k69;
    } else if (i < 140) {
        return k71;
    } else if (i < 160) {
        return k72;
    } else if (i < 180) {
        return k74;
    }  else {
        return k76;
    }
}

h = 0

function starled(i, t) {
    c = (0.5+0.5*sin(t*v8/10)) * cos(t + i)
    return [v5*c, v6*a, v7*a]
    return [127 + 127 * sin(t), 0, 0]
}

function side(i, t) {
    a = base(i)
    b = keys(i)
    c = pow(amp / h, 3)*10
    return [v1+b+c+a, v2+a, v3+a]
}

function color(i, t, device) {
    if (i == 0) {
        h = max(h, amp) * 0.99
    }
    if (device == 3) {
        return starled(i, t)
    } else {
        return side(i, t)
    }
    return [c+a, a, a]
}











 
