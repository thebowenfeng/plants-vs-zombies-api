
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

The following HTTP APIs are exposed when DLL is injected into the game (port 8080)

For HTTP body payloads, payload must be defined as an JSON object. All fields must either be a JSON object, or a string (even for non-string types). However, the documentation will specify what type the server expect (in string form). Field names must be a string (double quoted) as well.

Not all response will return a body, however, if a body is returned, assume content type is `text/plain` unless otherwise specified. `200` means successful execution of request, `409` typically means the request violates game logic and thus cannot be executed (for example, adding a plant when not in game). `400` means bad request, typically a required field/query param is missing. `500` is a generic fallback for server-side exceptions (typically for malformed input or other uncaught exceptions).

API path is from root path.

|API path|Description|Body (if any)|Query params (if any)|Response (409)|Response (200)
|--|--|--|--|--|--|
|`POST /api/plant/add`|Adds a plant onto the game board|`{row: int, col: int, index: int}`|N/A|- "Not in game" <br> - "Seed (index) does not exist"|N/A|
|`GET /api/seed/bank/size`|Gets the current game's max seed bank size|N/A|N/A|- "Not in game"|Size of current game's seed bank as integer
|`GET /api/seed/bank/selection/type`|Gets the current game's seed bank's seed type by index|N/A|index: int|- "Not in game"|Seed type as integer
|`GET /api/seed/bank/selection/size`|Gets the current game's seed bank's current size (how many seeds are selected). This API only make sense during seed selection stage|N/A|N/A|- "Not in game"|Seed bank size as integer
|`POST /api/seed/choose_seed/random`|Choose random seeds during seed selection and start game|N/A|N/A|- "Not choosing seeds"|N/A
|`POST /api/seed/choose_seed/pick_seed`|Pick a specific seed by seed type|`{type: int}`|N/A|- "Not choosing seeds"|N/A
|`POST /api/game/start`|Starts game|N/A|N/A|- "Not choosing seeds"<br>- "Not enough seeds chosen"|N/A
|`GET /api/game/state`|Gets current application state|N/A|N/A|N/A|Application state enum as integer
|`GET /api/game/result`|Gets current game's result|N/A|N/A|- "Unable to get in game results" (typically not in a game)|Game's result enum as integer
|`POST /api/game/restart`|Restarts current game|N/A|N/A|- "Game not lost"|N/A

Additionally, there are webhook APIs that allow the registration of callbacks to listen to certain game events. These callbacks are registered in the form of HTTP webhooks that queried with a POST request when a certain game event happens, with
the payload of `{type: CALLBACK_KEY, args?: object}`. The following `CALLBACK_KEY` is supported:
- "choose_seed" (Fired when the seed chooser screen is shown, no `args`)
- "game_over" (Fired when a survival level is lost, and the restart level button is shown, no `args`).
All webhooks are ephemeral and tied to the lifecycle of the DLL and strictly in memory. Once the DLL is ejected or the game is restarted, all webhooks must be re-registered.

*Note: Currently the HTTP client does not support HTTPS request. This is feature request*

|API path|Description|Body|Response (200)|Response (400)|Response (304)
|--|--|--|--|--|--|
|`POST /api/webhook/add`|Register a HTTP webhook as a callback|`{callbackKey: CALLBACK_KEY, callbackUrl: string}`|N/A|- "Invalid callback Url"<br>- "Callback key not recognised"|N/A (Callback already registered)
|`POST /api/webhook/remove`|Removes a HTTP webhook callback|`{callbackKey: CALLBACK_KEY, callbackUrl: string}`|N/A|- "Callback key not recognised"|N/A (Callback doesn't exist)

## Contribution

Take a look at the [high priority task backlog](https://github.com/thebowenfeng/plants-vs-zombies-api/issues?q=is%3Aissue%20state%3Aopen%20label%3A%22high%20priority%22). These are existing,

high priority issues that requires immediate attention and would be greatly appreciated.


New features and other changes are welcome, fork the project and raise a pull request.

## Feedback

Use "issues" tab and use labels appropriately. For bugs, try not to use "high priority" unless its a game-breaking issue (e.g game crashing). Use "bug" label instead.

## Credits

https://github.com/yhirose/cpp-httplib for HTTP client/server library

https://github.com/nlohmann/json for JSON parsing library