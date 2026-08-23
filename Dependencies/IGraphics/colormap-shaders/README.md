# colormap-shaders

A collection of shaders to draw color map like this:

![rainbow](sample/transform_rainbow.png).

# usage

Each `*.frag` shader sources provides a `colormap` function, which takes an `float` argument `x` (`x` should be: `0.0 <= x <= 1.0`). The `colormap` function returns a `vec4` value which represents an RGBA color.

```
vec4 colormap(float x);

void main() {
    gl_FragColor = colormap(gl_TexCoord[0].x);
}

/* include "shaders/rainbow.frag" here, for example. */
```

## special case

### gnuplot.frag

This shader emulates `gnuplot`'s `rgbformulae`, and the signature of `colormap` is:
```
vec4 colormap(float x, int red_type, int green_type, int blue_type);
```

It takes additional 3 arguments, with same meanings to `rgbformulae`'s 3 arguments respectively.

# usage from c++

```c++
#include <colormap/colormap.h>
#include <iostream>

int main()
{
    using namespace colormap;

    // Print RGB table of MATLAB::Jet colormap.
    MATLAB::Jet jet;
    std::cout << "category: " << jet.getCategory() << std::endl;
    std::cout << "title:    " << jet.getTitle() << std::endl;
    int const size = 256;
    for (int i = 0; i < size; ++i) {
        float const x = i / (float)size;
        Color c = jet.getColor(x);
        std::cout << x << "\t" << c.r << "\t" << c.g << "\t" << c.b << std::endl;
    }

    // Dump category and title of all colormaps.
    for (std::shared_ptr<Colormap const> const& c : ColormapList::getAll()) {
        std::cout << c->getCategory() << " : " << c->getTitle() << std::endl;
    }
    return 0;
}
```

# samples

## MATLAB

* http://www.mathworks.com/products/matlab/

|name          |sample                       |
|--------------|-----------------------------|
|MATLAB\_autumn|![](sample/MATLAB_autumn.png)|
|MATLAB\_bone  |![](sample/MATLAB_bone.png)  |
|MATLAB\_cool  |![](sample/MATLAB_cool.png)  |
|MATLAB\_copper|![](sample/MATLAB_copper.png)|
|MATLAB\_hot   |![](sample/MATLAB_hot.png)   |
|MATLAB\_hsv   |![](sample/MATLAB_hsv.png)   |
|MATLAB\_jet   |![](sample/MATLAB_jet.png)   |
|MATLAB\_parula|![](sample/MATLAB_parula.png)|
|MATLAB\_pink  |![](sample/MATLAB_pink.png)  |
|MATLAB\_spring|![](sample/MATLAB_spring.png)|
|MATLAB\_summer|![](sample/MATLAB_summer.png)|
|MATLAB\_winter|![](sample/MATLAB_winter.png)|

## gnuplot

* http://www.gnuplot.info/

|name                   |sample                          |
|-----------------------|--------------------------------|
|rgbformulae(7, 5, 15)  |![](sample/gnuplot_7_5_15.png)  |
|rgbformulae(3, 11, 6)  |![](sample/gnuplot_3_11_6.png)  |
|rgbformulae(23, 28, 3) |![](sample/gnuplot_23_28_3.png) |
|rgbformulae(21, 22, 23)|![](sample/gnuplot_21_22_23.png)|
|rgbformulae(30, 31, 32)|![](sample/gnuplot_30_31_32.png)|
|rgbformulae(33, 13, 10)|![](sample/gnuplot_33_13_10.png)|
|rgbformulae(34, 35, 36)|![](sample/gnuplot_34_35_36.png)|

## IDL

* http://www.exelisvis.com/ProductsServices/IDL.aspx

|name                         |sample                                     |
|-----------------------------|-------------------------------------------|
|IDL\_Black-White\_Linear     |![](sample/IDL_Black-White_Linear.png)     | <!-- #0 -->
|IDL\_Blue-White\_Linear      |![](sample/IDL_Blue-White_Linear.png)      | <!-- #1 -->
|IDL\_Green\-Red\-Blue\-White |![](sample/IDL_Green-Red-Blue-White.png)   | <!-- #2 -->
|IDL\_Red\_Temperature        |![](sample/IDL_Red_Temperature.png)        | <!-- #3 -->
|IDL\_Blue-Green-Red-Yellow   |![](sample/IDL_Blue-Green-Red-Yellow.png)  | <!-- #4 -->
|IDL\_Standard\_Gamma-II      |![](sample/IDL_Standard_Gamma-II.png)      | <!-- #5 -->
|IDL\_Prism                   |![](sample/IDL_Prism.png)                  | <!-- #6 -->
|IDL\_Red-Purple              |![](sample/IDL_Red-Purple.png)             | <!-- #7 -->
|IDL\_Green-White\_Linear     |![](sample/IDL_Green-White_Linear.png)     | <!-- #8 -->
|IDL\_Green-White\_Exponential|![](sample/IDL_Green-White_Exponential.png)| <!-- #9 -->
|IDL\_Green-Pink              |![](sample/IDL_Green-Pink.png)             | <!-- #10 -->
|IDL\_Blue-Red                |![](sample/IDL_Blue-Red.png)               | <!-- #11 -->
|IDL\_16\_Level               |![](sample/IDL_16_Level.png)               | <!-- #12 -->
|IDL\_Rainbow                 |![](sample/IDL_Rainbow.png)                | <!-- #13 -->
|IDL\_Steps                   |![](sample/IDL_Steps.png)                  | <!-- #14 -->
|IDL\_Stern\_Special          |![](sample/IDL_Stern_Special.png)          | <!-- #15 -->
|IDL\_Haze                    |![](sample/IDL_Haze.png)                   | <!-- #16 -->
|IDL\_Blue\-Pastel\-Red       |![](sample/IDL_Blue-Pastel-Red.png)        | <!-- #17 -->
|IDL\_Pastels                 |![](sample/IDL_Pastels.png)                | <!-- #18 -->
|IDL\_Hue\_Sat\_Lightness\_1  |![](sample/IDL_Hue_Sat_Lightness_1.png)    | <!-- #19 -->
|IDL\_Hue\_Sat\_Lightness\_2  |![](sample/IDL_Hue_Sat_Lightness_2.png)    | <!-- #20 -->
|IDL\_Hue\_Sat\_Value\_1      |![](sample/IDL_Hue_Sat_Value_1.png)        | <!-- #21 -->
|IDL\_Hue\_Sat\_Value\_2      |![](sample/IDL_Hue_Sat_Value_2.png)        | <!-- #22 -->
|IDL\_Purple\-Red\+Stripes    |![](sample/IDL_Purple-Red+Stripes.png)     | <!-- #23 -->
|IDL\_Beach                   |![](sample/IDL_Beach.png)                  | <!-- #24 -->
|IDL\_Mac\_Style              |![](sample/IDL_Mac_Style.png)              | <!-- #25 -->
|IDL\_Eos\_A                  |![](sample/IDL_Eos_A.png)                  | <!-- #26 -->
|IDL\_Eos\_B                  |![](sample/IDL_Eos_B.png)                  | <!-- #27 -->
|IDL\_Hardcandy               |![](sample/IDL_Hardcandy.png)              | <!-- #28 -->
|IDL\_Nature                  |![](sample/IDL_Nature.png)                 | <!-- #29 -->
|IDL\_Ocean                   |![](sample/IDL_Ocean.png)                  | <!-- #30 -->
|IDL\_Peppermint              |![](sample/IDL_Peppermint.png)             | <!-- #31 -->
|IDL\_Plasma                  |![](sample/IDL_Plasma.png)                 | <!-- #32 -->
|IDL\_Blue-Red\_2             |![](sample/IDL_Blue-Red_2.png)             | <!-- #33 -->
|IDL\_Rainbow\_2              |![](sample/IDL_Rainbow_2.png)              | <!-- #34 -->
|IDL\_Blue\_Waves             |![](sample/IDL_Blue_Waves.png)             | <!-- #35 -->
|IDL\_Volcano                 |![](sample/IDL_Volcano.png)                | <!-- #36 -->
|IDL\_Waves                   |![](sample/IDL_Waves.png)                  | <!-- #37 -->
|IDL\_Rainbow\_18             |![](sample/IDL_Rainbow_18.png)             | <!-- #38 -->
|IDL\_Rainbow\+White          |![](sample/IDL_Rainbow+White.png)          | <!-- #39 -->
|IDL\_Rainbow\+Black          |![](sample/IDL_Rainbow+Black.png)          | <!-- #40 -->
|IDL\_CB\-Accent              |![](sample/IDL_CB-Accent.png)              | <!-- #41 -->
|IDL\_CB\-Dark2               |![](sample/IDL_CB-Dark2.png)               | <!-- #42 -->
|IDL\_CB\-Paired              |![](sample/IDL_CB-Paired.png)              | <!-- #43 -->
|IDL\_CB\-Pastel1             |![](sample/IDL_CB-Pastel1.png)             | <!-- #44 -->
|IDL\_CB\-Pastel2             |![](sample/IDL_CB-Pastel2.png)             | <!-- #45 -->
|IDL\_CB\-Set1                |![](sample/IDL_CB-Set1.png)                | <!-- #46 -->
|IDL\_CB\-Set2                |![](sample/IDL_CB-Set2.png)                | <!-- #47 -->
|IDL\_CB\-Set3                |![](sample/IDL_CB-Set3.png)                | <!-- #48 -->
|IDL\_CB\-Blues               |![](sample/IDL_CB-Blues.png)               | <!-- #49 -->
|IDL\_CB\-BuGn                |![](sample/IDL_CB-BuGn.png)                | <!-- #50 -->
|IDL\_CB\-BuPu                |![](sample/IDL_CB-BuPu.png)                | <!-- #51 -->
|IDL\_CB\-GnBu                |![](sample/IDL_CB-GnBu.png)                | <!-- #52 -->
|IDL\_CB\-Greens              |![](sample/IDL_CB-Greens.png)              | <!-- #53 -->
|IDL\_CB\-Greys               |![](sample/IDL_CB-Greys.png)               | <!-- #54 -->
|IDL\_CB\-Oranges             |![](sample/IDL_CB-Oranges.png)             | <!-- #55 -->
|IDL\_CB\-OrRd                |![](sample/IDL_CB-OrRd.png)                | <!-- #56 -->
|IDL\_CB\-PuBu                |![](sample/IDL_CB-PuBu.png)                | <!-- #57 -->
|IDL\_CB\-PuBuGn              |![](sample/IDL_CB-PuBuGn.png)              | <!-- #58 -->
|IDL\_CB\-PuRdn               |![](sample/IDL_CB-PuRd.png)                | <!-- #59 -->
|IDL\_CB\-Purples             |![](sample/IDL_CB-Purples.png)             | <!-- #60 -->
|IDL\_CB\-RdPu                |![](sample/IDL_CB-RdPu.png)                | <!-- #61 -->
|IDL\_CB\-Reds                |![](sample/IDL_CB-Reds.png)                | <!-- #62 -->
|IDL\_CB\-YIGn                |![](sample/IDL_CB-YIGn.png)                | <!-- #63 -->
|IDL\_CB\-YIGnBu              |![](sample/IDL_CB-YIGnBu.png)              | <!-- #64 -->
|IDL\_CB\-YIOrBr              |![](sample/IDL_CB-YIOrBr.png)              | <!-- #65 -->
|IDL\_CB\-BrBG                |![](sample/IDL_CB-BrBG.png)                | <!-- #66 -->
|IDL\_CB\-PiYG                |![](sample/IDL_CB-PiYG.png)                | <!-- #67 -->
|IDL\_CB\-PRGn                |![](sample/IDL_CB-PRGn.png)                | <!-- #68 -->
|IDL\_CB\-PuOr                |![](sample/IDL_CB-PuOr.png)                | <!-- #69 -->
|IDL\_CB\-RdBu                |![](sample/IDL_CB-RdBu.png)                | <!-- #70 -->
|IDL\_CB\-RdGy                |![](sample/IDL_CB-RdGy.png)                | <!-- #71 -->
|IDL\_CB\-RdYiBu              |![](sample/IDL_CB-RdYiBu.png)              | <!-- #72 -->
|IDL\_CB\-RdYiGn              |![](sample/IDL_CB-RdYiGn.png)              | <!-- #73 -->
|IDL\_CB\-Spectral            |![](sample/IDL_CB-Spectral.png)            | <!-- #74 -->

## transform

* http://web.archive.org/web/20000520021207/http://www.fortner.com/docs/product_transform.html

|name                      |sample                                 |
|--------------------------|---------------------------------------|
|transform\_rainbow                   |![](sample/transform_rainbow.png)                |
|transform\_apricot                   |![](sample/transform_apricot.png)                |
|transform\_carnation                 |![](sample/transform_carnation.png)              |
|transform\_ether                     |![](sample/transform_ether.png)                  |
|transform\_grayscale\_banded         |![](sample/transform_grayscale_banded.png)       |
|transform\_hot\_metal                |![](sample/transform_hot_metal.png)              |
|transform\_lava\_waves               |![](sample/transform_lava_waves.png)             |
|transform\_malachite                 |![](sample/transform_malachite.png)              |
|transform\_seismic                   |![](sample/transform_seismic.png)                |
|transform\_space                     |![](sample/transform_space.png)                  |
|transform\_morning\_glory            |![](sample/transform_morning_glory.png)          |
|transform\_peanut\_butter\_and\_jerry|![](sample/transform_peanut_butter_and_jerry.png)|
|transform\_purple\_haze              |![](sample/transform_purple_haze.png)            |
|transform\_rose                      |![](sample/transform_rose.png)                   |
|transform\_saturn                    |![](sample/transform_saturn.png)                 |
|transform\_supernova                 |![](sample/transform_supernova.png)              |

## transform
|name             |sample                          |
|-----------------|--------------------------------|
|kbinani\_altitude|![](sample/kbinani_altitude.png)|

# license

The MIT License. See 'LICENSE' file for detail.
