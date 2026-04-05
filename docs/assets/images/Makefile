SOURCEDIR = .
IMAGES := $(shell find $(SOURCEDIR) -maxdepth 1 -name '*.jpg')
RESIZED := $(addprefix 800/,$(IMAGES) )

all:	strip $(RESIZED)

strip:	
	exiftool -all= -r -overwrite_original $(SOURCEDIR)

800/%.jpg:	%.jpg
	magick $< -resize 800x $@
