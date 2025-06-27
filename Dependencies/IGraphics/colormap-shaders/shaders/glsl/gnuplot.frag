float colormap_f1(int formula, float x) {
    float DEG2RAD = 3.1415926535897932384 / 180.0;
    if (formula < 0) {
        x = 1.0 - x;
        formula = -formula;
    }
    float d = 0.0;
    if (formula == 0) {
        return 0.0;
    } else if (formula == 1) {
        return 0.5;
    } else if (formula == 2) {
        return 1.0;
    } else if (formula == 3) {
        d = x;
    } else if (formula == 4) {
        d = x * x;
    } else if (formula == 5) {
        d = x * x * x;
    } else if (formula == 6) {
        d = x * x * x * x;
    } else if (formula == 7) {
        d = sqrt(x);
    } else if (formula == 8) {
        d = sqrt(sqrt(x));
    } else if (formula == 9) {
        d = sin(90.0 * x * DEG2RAD);
    } else if (formula == 10) {
        d = cos(90.0 * x * DEG2RAD);
    } else if (formula == 11) {
        d = abs(x - 0.5);
    } else if (formula == 12) {
        d = (2.0 * x - 1.0) * (2.0 * x - 1.0);
    } else if (formula == 13) {
        d = sin(180.0 * x * DEG2RAD);
    } else if (formula == 14) {
        d = abs(cos(180.0 * x * DEG2RAD));
    } else if (formula == 15) {
        d = sin(360.0 * x * DEG2RAD);
    } else if (formula == 16) {
        d = cos(360.0 * x * DEG2RAD);
    } else if (formula == 17) {
        d = abs(sin(360.0 * x * DEG2RAD));
    } else if (formula == 18) {
        d = abs(cos(360.0e0 * x * DEG2RAD));
    } else if (formula == 19) {
        d = abs(sin(720.0e0 * x * DEG2RAD));
    } else if (formula == 20) {
        d = abs(cos(720.0e0 * x * DEG2RAD));
    } else if (formula == 21) {
        d = 3.0e0 * x;
    } else if (formula == 22) {
        d = 3.0e0 * x - 1.0e0;
    } else if (formula == 23) {
        d = 3.0e0 * x - 2.0e0;
    } else if (formula == 24) {
        d = abs(3.0e0 * x - 1.0e0);
    } else if (formula == 25) {
        d = abs(3.0e0 * x - 2.0e0);
    } else if (formula == 26) {
        d = 1.5e0 * x - 0.5e0;
    } else if (formula == 27) {
        d = 1.5e0 * x - 1.0e0;
    } else if (formula == 28) {
        d = abs(1.5e0 * x - 0.5e0);
    } else if (formula == 29) {
        d = abs(1.5e0 * x - 1.0e0);
    } else if (formula == 30) {
        if (x <= 0.25e0) {
            return 0.0;
        } else if (x >= 0.57) {
            return 1.0;
        } else {
            d = x / 0.32 - 0.78125;
        }
    } else if (formula == 31) {
        if (x <= 0.42) {
            return 0.0;
        } else if (x >= 0.92) {
            return d = 1.0;
        } else {
            d = 2.0 * x - 0.84;
        }
    } else if (formula == 32) {
        if (x <= 0.42) {
            d = x * 4.0;
        } else {
            if (x <= 0.92e0) {
                d = -2.0 * x + 1.84;
            } else {
                d = x / 0.08 - 11.5;
            }
        }
    } else if (formula == 33) {
        d = abs(2.0 * x - 0.5);
    } else if (formula == 34) {
        d = 2.0 * x;
    } else if (formula == 35) {
        d = 2.0 * x - 0.5;
    } else if (formula == 36) {
        d = 2.0 * x - 1.0;
    }
    if (d <= 0.0) {
        return 0.0;
    } else if (d >= 1.0) {
        return 1.0;
    } else {
        return d;
    }
}

vec4 colormap(float x, int red_type, int green_type, int blue_type) {
    return vec4(colormap_f1(red_type, x), colormap_f1(green_type, x), colormap_f1(blue_type, x), 1.0);
}
