@echo off
:loop
echo Starting batch of 100 requests...

for /L %%i in (0,1,5) do (
    start /b curl -s -X PUT http://localhost:8080/api/plant/add -H "Content-Type: application/json" -d "{\"row\": \"0\", \"col\": \"0\", \"index\": \"%%i\"}"
)

echo Waiting 5 seconds...
timeout /t 5 > nul
goto loop