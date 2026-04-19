SOURCEDIR = .
JPGS := $(shell ls $(SOURCEDIR)/*.jpg)
PNGS := $(shell ls $(SOURCEDIR)/*.png)
RESIZED := $(addprefix 800/,$(JPGS) ) $(addprefix 800/,$(PNGS) ) 

all:	strip $(RESIZED)

strip:	
	exiftool -all= -r -overwrite_original $(SOURCEDIR)

800/%.jpg:	%.jpg
	magick $< -resize 800x $@

800/%.png:	%.png
	magick $< -resize 800x $@
