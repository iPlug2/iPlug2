float colormap_blue(float x) {
    if (x < 248.25 / 1066.8) {
        return 0.0;
    } else if (x < 384.25 / 1066.8) {
        return (1066.8 * x - 248.25) / 255.0;
    } else if (x < 0.5) {
        return 136.0 / 255.0;
    } else if (x < 595.14 / 1037.9) {
        return (-1037.9 * x + 595.14) / 255.0;
    } else if (x < 666.68 / 913.22) {
        return 0.0;
    } else if (x <= 1.0) {
        return (913.22 * x - 666.68) / 255.0;
    } else {
        return 246.0 / 255.0;
    }
}

float colormap_green(float x) {
    if (x < 0.0) {
        return 253.0 / 255.0;
    } else if (x < 248.25 / 1066.8) {
        return (-545.75 * x + 253.36) / 255.0;
    } else if (x < 384.25 / 1066.8) {
        return  (426.18 * x + 19335217.0 / 711200.0) / 255.0;
    } else if (x < 0.5) {
        return (-385524981.0 / 298300.0 * x + 385524981.0 / 596600.0) / 255.0;
    } else if (x < 666.68 / 913.22) {
        return (3065810.0 / 3001.0 * x - 1532906.0 / 3001.0) / 255.0;
    } else {
        return 0.0;
    }
}

float colormap_red(float x) {
    if (x < 384.25 / 1066.8) {
        return 0.0;
    } else if (x < 0.5) {
        return (1092.0 * x - 99905.0 / 254.0) / 255.0;
    } else if (x < 259.3 / 454.5) {
        return (1091.9 * x - 478.18) / 255.0;
    } else if (x < 34188.3 / 51989.0) {
        return (819.2 * x - 322.6) / 255.0;
    } else if (x < 666.68 / 913.22) {
        return (299.31 * x + 19.283) / 255.0;
    } else {
        return 0.0;
    }
}

vec4 colormap(float x) {
    return vec4(colormap_red(x), colormap_green(x), colormap_blue(x), 1.0);
}
