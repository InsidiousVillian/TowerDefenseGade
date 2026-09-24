$build = "C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat"
$project = "c:\Users\jetta\OneDrive\Documentos\GitHub\Towerdefense_GADE\Towerdefense_GADE.uproject"
& $build Towerdefense_GADEEditor Win64 Development "-Project=$project" -WaitMutex
exit $LASTEXITCODE
