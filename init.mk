.PHONY: init
init:
	@echo "----------------------------------------------------------------------"
	@echo "Initialising a new WhateverDSP project.."
	@echo "----------------------------------------------------------------------\n"
	-cp -n $(WDSP_PATH)/template/template.c ${TARGET}.c
	-cp -n $(WDSP_PATH)/template/config.ini .
	@if test -d .vscode; then echo "\n.vscode exists, not generating a workspace.."; else echo "\nGenerating a VS Code workspace..\n" && make vscode; fi
	@echo "\n----------------------------------------------------------------------"
	@echo "Done! Edit and compile ${TARGET}.c to get started!"
	@echo "----------------------------------------------------------------------\n"
	@echo "Tips:\n"
	@echo "- If you're on Mac OS, your path to the compiler can be different."
	@echo "  You might need to change your compilerPath inside this file:"
	@echo "  .vscode/c_cpp_properties.json\n"
	@echo "- If you're not using VS Code, simply run 'make' to build.\n"
	@echo "- While debugging you may need to do 'make clean DEBUG=true' when"
	@echo "  you want to clean everything.\n"
	@echo "For more documentation, please see our Github:"
	@echo "https://github.com/NuclearLighthouseStudios/WhateverDSP\n"
	@echo "You may also want to check out the examples:"
	@echo "https://github.com/NuclearLighthouseStudios/WhateverDSP-Examples"
	@echo "\nHave fun with WhateverDSP! ¯\_(ツ)_/¯\n"
