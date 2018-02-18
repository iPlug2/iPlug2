install the following in order to compile (debian)

sudo apt-get install build-essential libgtk-3-dev

then build with the following command (doesn't draw anything yet!)

make GDK3=1 SWELL_SUPPORT_GTK=1 ALLOW_WARNINGS=1 NOLICE=1
