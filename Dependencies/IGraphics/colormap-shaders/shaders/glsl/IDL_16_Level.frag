vec4 colormap(float x) {
    if (x < 0.0) {
        return vec4(0.0, 0.0, 0.0, 1.0);
    } else if (1.0 < x) {
        return vec4(1.0, 1.0, 1.0, 1.0);
    } else if (x < 1.0 / 16.0) {
        return vec4(0.0, 84.0 / 255.0, 0.0, 1.0);
    } else if (x < 2.0 / 16.0) {
        return vec4(0.0, 168.0 / 255.0, 0.0, 1.0);
    } else if (x < 3.0 / 16.0) {
        return vec4(0.0, 1.0, 0.0, 1.0);
    } else if (x < 4.0 / 16.0) {
        return vec4(0.0, 1.0, 84.0 / 255.0, 1.0);
    } else if (x < 5.0 / 16.0) {
        return vec4(0.0, 1.0, 168.0 / 255.0, 1.0);
    } else if (x < 6.0 / 16.0) {
        return vec4(0.0, 1.0, 1.0, 1.0);
    } else if (x < 7.0 / 16.0) {
        return vec4(0.0, 0.0, 1.0, 1.0);
    } else if (x < 8.0 / 16.0) {
        return vec4(128.0 / 255.0, 0.0, 1.0, 1.0);
    } else if (x < 9.0 / 16.0) {
        return vec4(1.0, 0.0, 220.0 / 255.0, 1.0);
    } else if (x < 10.0 / 16.0) {
        return vec4(1.0, 0.0, 180.0 / 255.0, 1.0);
    } else if (x < 11.0 / 16.0) {
        return vec4(1.0, 0.0, 128.0 / 255.0, 1.0);
    } else if (x < 12.0 / 16.0) {
        return vec4(1.0, 0.0, 64.0 / 255.0, 1.0);
    } else if (x < 13.0 / 16.0) {
        return vec4(1.0, 0.0, 0.0, 1.0);
    } else if (x < 14.0 / 16.0) {
        return vec4(220.0 / 255.0, 190.0 / 255.0, 190.0 / 255.0, 1.0);
    } else if (x < 15.0 / 16.0) {
        return vec4(220.0 / 255.0, 220.0 / 255.0, 220.0 / 255.0, 1.0);
    } else {
        return vec4(1.0, 1.0, 1.0, 1.0);
    }
}
