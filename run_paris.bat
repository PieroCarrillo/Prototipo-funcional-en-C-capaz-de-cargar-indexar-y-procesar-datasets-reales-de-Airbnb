@echo off
setlocal

if not exist "data\real\Paris\listings.csv" (
  call import_paris.bat
  if errorlevel 1 exit /b 1
)

call run.bat ^
  --data data\real\Paris ^
  --query Paris ^
  --id 43675393 ^
  --min-price 40 ^
  --max-price 120 ^
  --top 5 ^
  --graph-limit 100

