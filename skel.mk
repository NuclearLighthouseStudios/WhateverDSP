# Only define these target when we're not building the library
ifdef WDSP_PATH

.PHONY: skel
skel:
	@echo "----------------------------------------------------------------------"
	@echo "Copying the skeleton WhateverDSP project.."
	@echo "----------------------------------------------------------------------\n"
	-cp -n $(WDSP_PATH)/skel/skel.c ${TARGET}.c
	-cp -n $(WDSP_PATH)/skel/config.ini .
	@echo "\n----------------------------------------------------------------------"
	@echo "Done! Edit and compile ${TARGET}.c to get started!"
	@echo "----------------------------------------------------------------------\n"
	@echo "Tips:\n"
	@echo "- If you're not using VS Code, simply run 'make' to build."
	@echo "- While debugging you may need to do 'make clean DEBUG=true' when you"
	@echo "  want to clean everything.\n"
	@echo "For more documentation, please see our Github:"
	@echo "https://github.com/NuclearLighthouseStudios/WhateverDSP\n"
	@echo "You may also want to check out the examples:"
	@echo "https://github.com/NuclearLighthouseStudios/WhateverDSP-Examples"
	@echo "\nHave fun with WhateverDSP! ¯\_(ツ)_/¯\n"

endif