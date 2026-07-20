float colormap_e = exp(1.0);

float colormap_red(float x) {
    if (x < 0.0) {
        return 13.0 / 255.0;
    } else if (x < colormap_e * 0.1) {
        return (706.48 * x + 13.06) / 255.0;
    } else if (x < colormap_e * 0.1 + 149.0 / 510.0) {
        return (166.35 * x + 28.3) / 255.0;
    } else if (x < colormap_e * 0.1 + 298.0 / 510.0) {
        return (313.65 * x - 47.179) / 255.0;
    } else if (x < colormap_e * 0.05 + 202.0 / 255.0) {
        return (557.93 * x - 310.05) / 255.0;
    } else if (x <= 1.0) {
        return (319.64 * x + 439093.0 / 34000.0 * colormap_e - 1030939.0 / 8500.0) / 255.0;
    } else {
        return 249.0 / 255.0;
     }
}

float colormap_green(float x) {
    if (x < colormap_e * 0.1) {
        return 0.0;
    } else if (x < colormap_e * 0.1 + 149.0 / 510.0) {
        return ((3166.59 / 14.9 * colormap_e + 2098.7 / 74.5) * x - (316.659 / 14.9 * colormap_e + 209.87 / 74.5) * colormap_e) / 255.0;
    } else if (x < colormap_e * 0.1 + 298.0 / 510.0) {
        return (725.0 * x - 394.35) / 255.0;
    } else if (x <= 1.0) {
        return (-716.23 * x + 721.38) / 255.0;
    } else {
        return 5.0 / 255.0;
    }
}

float colormap_blue(float x) {
    if (x < 0.0) {
        return 16.0 / 255.0;
    } else if (x < colormap_e * 0.1) {
        return (878.72 * x + 16.389) / 255.0;
    } else if (x < colormap_e * 0.1 + 149.0 / 510.0) {
        return (-166.35 * x + 227.7) / 255.0;
    } else if (x < colormap_e * 0.1 + 298.0 / 510.0) {
        return (-317.2 * x + 305.21) / 255.0;
    } else if (x < 1.0) {
        return ((1530.0 / (212.0 -51.0 * colormap_e)) * x + (153.0 * colormap_e + 894.0) / (51.0 * colormap_e - 212.0)) / 255.0;
    } else {
        return 2.0 / 255.0;
    }
}

vec4 colormap(float x) {
    return vec4(colormap_red(x), colormap_green(x), colormap_blue(x), 1.0);
}
