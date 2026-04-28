Look at https://cspatterns.dev/json
Could we build a DLL that injecst into CS and exposes an ai that lets a user vibecode their software by exposing a Lua API that gives access to the whole game and lets the ai code anything into it

What AI backend should power the vibe-coding assistant?
User's choice at runtime
How should the DLL be injected into CS2?
Manual Map
How should the in-game UI / editor work?
External window ( the dll loader an script editor and ai are the same zdraw zui window, so the ai can use hooks LuaJIT and many more things from zui that is in the internal dll for lua to use
