# Plants vs Zombies API

API for interoperability between Plants Vs Zombies (GOTY) and 3rd party programs. Supports Plants vs Zombies GOTY v1.2.0.1073

## Build

Build using Visual Studio under `x86` profile as a DLL.

## Usage

Inject (load) the DLL file using any DLL injector or simply calling LoadLibrary.

## Documentation

### Local debugger

A local debugger is available as an interactive console when DLL is loaded into PvZ. Type `help` in console for a full list of commands.

### HTTP API

TBD

## Contribution

Take a look at the [high priority task backlog](https://github.com/thebowenfeng/plants-vs-zombies-api/issues?q=is%3Aissue%20state%3Aopen%20label%3A%22high%20priority%22). These are existing,
high priority issues that requires immediate attention and would be greatly appreciated.

New features and other changes are welcome, fork the project and raise a pull request.

## Feedback

Use "issues" tab and use labels appropriately. For bugs, try not to use "high priority" unless its a game-breaking issue (e.g game crashing). Use "bug" label instead.

## Credits

https://github.com/yhirose/cpp-httplib for HTTP client/server library
