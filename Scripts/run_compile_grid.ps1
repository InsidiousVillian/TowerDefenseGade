$editor = "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$project = "c:\Users\jetta\OneDrive\Documentos\GitHub\Towerdefense_GADE\Towerdefense_GADE.uproject"
$script = "c:\Users\jetta\OneDrive\Documentos\GitHub\Towerdefense_GADE\Scripts\compile_grid_generator.py"
& $editor $project -stdout -unattended -nop4 -nosplash -log="CompileGridGenerator.log" "-ExecutePythonScript=$script"
exit $LASTEXITCODE
