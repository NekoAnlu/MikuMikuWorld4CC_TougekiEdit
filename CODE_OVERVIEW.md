# Project Root

*   `.clang-format`: Configuration file for ClangFormat, a tool to automatically format C++ code to maintain consistent style.
*   `.editorconfig`: Configuration for EditorConfig, which helps maintain consistent coding styles across different editors and IDEs.
*   `.gitattributes`: Defines attributes per path for Git (e.g., line ending handling).
*   `.github/`: Contains GitHub-specific files.
    *   `workflows/`: Holds YAML files for GitHub Actions (CI/CD).
        *   `build.yml`: Defines a GitHub Action workflow for building the project.
        *   `check.yml`: Defines a GitHub Action workflow for running checks (likely linters or static analysis).
*   `.gitignore`: Specifies intentionally untracked files that Git should ignore (e.g., build outputs, log files).
*   `.gitmodules`: Used if the project includes Git submodules (external repositories embedded within this one). The `Depends` directory might be related to this.
*   `Depends/`: This directory houses third-party libraries and dependencies required by the project.
    *   `DirectXMath-master/`: A library for 3D math operations, likely from Microsoft.
    *   `GLFW/`: A multi-platform library for OpenGL, OpenGL ES and Vulkan development. It provides an API for creating windows, contexts and surfaces, receiving input and events.
        *   `include/GLFW/`: Header files for GLFW.
        *   `lib/glfw3.lib`: Static library for GLFW.
    *   `choc/`: Appears to be the "CHOCOLAT" C++ helper classes, possibly for things like string manipulation or data structures. (I saw `choc::hash::xxHash64` in `Score.cpp`).
    *   `cpp-httplib/`: A C++ single-file header-only cross platform HTTP/HTTPS library. Used for network requests if any.
    *   `glad/`: A multi-language GL/GLES/EGL/GLX/WGL loader-generator based on the official specs. Used to load OpenGL functions at runtime.
        *   `include/`: Header files for glad.
        *   `src/glad.c`: Source file for glad.
    *   `json/`: Contains `json.hpp`, which is the "JSON for Modern C++" single-header library by Niels Lohmann, used for JSON parsing and serialization (as seen in `ApplicationConfiguration.cpp`).
    *   `miniaudio/`: A single file audio playback and capture library. Used for sound playback.
    *   `openssl/`: A robust, commercial-grade, and full-featured toolkit for the Transport Layer Security (TLS) and Secure Sockets Layer (SSL) protocols. Also a general-purpose cryptography library. Might be used if the editor has features like online connectivity or secure data handling, or if `cpp-httplib` requires it for HTTPS.
    *   `stb_image/`: Single-file public domain (or MIT licensed) library for loading images (e.g., PNG, JPG) by Sean Barrett.
    *   `stb_vorbis/`: Single-file public domain (or MIT licensed) library for decoding Ogg Vorbis audio files by Sean Barrett.
*   `LICENSE`: Contains the software license for this project.
*   `Licenses/`: Directory containing license files for the third-party libraries used in `Depends/`.
*   `MikuMikuWorld/`: **This is the main directory containing the source code for the editor itself.** I will detail this further.
*   `MikuMikuWorld.sln`: Visual Studio Solution file, used to manage and build the project in Visual Studio.
*   `README.ja.md`: README file in Japanese.
*   `README.md`: Main README file for the project, likely providing an overview, build instructions, etc.
*   `Rakefile`: A file for Rake, a Ruby-based build utility. This might be an alternative build system or used for automation tasks.
*   `TRANSLATION.md`: Document related to translating the editor into different languages.
*   `compile_flags.txt`: Likely contains compiler flags used for building the project, sometimes used by tools like Clangd for code completion and diagnostics.
*   `installerBase.nsi`: NSIS (Nullsoft Scriptable Install System) script file. This is used to create a Windows installer for the editor.
*   `typos.toml`: Configuration file for `typos-cli`, a source code spell checker.

## MikuMikuWorld/

This is the core directory for the editor's application logic, UI, and chart processing.

*   **Application.h/.cpp**:
    *   Defines the main `Application` class.
    *   Manages the main application loop, window creation (using GLFW), and initialization/shutdown of systems.
    *   Handles overall application state, including settings, loading resources, and managing the main editor interface (`ScoreEditor`).
    *   Contains `WindowState` struct to track window properties.
    *   Handles drag-and-drop of files.

*   **ApplicationConfiguration.h/.cpp**:
    *   Defines structures for application settings (`ApplicationConfiguration` and `InputConfiguration`).
    *   `InputConfiguration` specifically lists all mappable actions (e.g., `togglePlayback`, `save`) and their default `MultiInputBinding`s.
    *   `ApplicationConfiguration` holds all settings like window size, theme, audio volumes, timeline preferences, auto-save options, and the `InputConfiguration`.
    *   Manages reading and writing application settings to a JSON file (likely `app_config.json` as defined in `Constants.h`).
    *   Contains the global `config` object and the `bindings` array for easy access to settings and keybinds.

*   **Audio/**: Sub-directory for audio-related code.
    *   **AudioManager.h/.cpp**:
        *   Manages audio playback for music (BGM) and sound effects (SE).
        *   Uses the `miniaudio` library.
        *   Handles loading sounds, playing, stopping, setting volume, and managing different SE profiles.
        *   Likely responsible for music synchronization with the chart.
    *   **Sound.h/.cpp**:
        *   Represents a single sound effect or music track.
        *   Wraps `ma_sound` from `miniaudio`.
        *   Provides methods to load, play, stop, loop, and set volume for individual sounds.
    *   **Waveform.h/.cpp**:
        *   Generates and stores waveform data (likely from the music file) for display in the editor timeline.
        *   Supports creating a mip chain for efficient display at different zoom levels.

*   **Background.h/.cpp**:
    *   Manages the rendering of the editor's background, which can be a solid color or an image.
    *   Handles loading and drawing the background image, adjusting brightness, and managing related shaders.

*   **BinaryReader.h/.cpp**:
    *   Utility class for reading data from binary files.
    *   Provides methods to read basic types like integers, floats, strings, etc., in a platform-independent way (handles endianness if necessary, though not explicitly stated, it's good practice for binary formats).
    *   Used by `Score.cpp` to deserialize `.mmws`/`.ccmmws` chart files.

*   **BinaryWriter.h/.cpp**:
    *   Utility class for writing data to binary files.
    *   Provides methods to write basic data types.
    *   Used by `Score.cpp` to serialize `Score` objects into `.mmws`/`.ccmmws` chart files.

*   **Colors.h**:
    *   This file was present in the directory listing. Based on its name, it would typically define color constants, palettes, or utility functions for color manipulation, possibly for UI theming or specific rendering tasks. However, the primary `Color` struct used for rendering is defined in `Math.h`. Further inspection would be needed to determine its exact role if it's still actively used.

*   **Constants.h**:
    *   Defines various global constants used throughout the application.
    *   Examples: `TICKS_PER_BEAT`, min/max lane widths, min/max BPM, texture names (`NOTES_TEX`), configuration filenames (`APP_CONFIG_FILENAME`), and file extensions (`SUS_EXTENSION`, `MMWS_EXTENSION`).
    *   Defines `id_t` as the type for note/event identifiers.

*   **File.h/.cpp**:
    *   Provides file system utility functions under the `IO` namespace.
    *   `File` class: A wrapper around C-style file operations (`FILE*`) for easier file reading/writing (e.g., `readAllBytes`, `readLine`, `writeAllLines`).
    *   `FileDialog` class: Wraps native Windows file open/save dialogs. Allows setting title, filters, and default extensions.
    *   Static methods for path manipulation (e.g., `getFilename`, `getFileExtension`, `exists`).

*   **HistoryManager.h/.cpp**:
    *   Implements the undo/redo functionality for the editor.
    *   Stores a history of actions or states (`HistoryItem`).
    *   Allows pushing new states, undoing to a previous state, and redoing an undone action.

*   **IO.h/.cpp**: (Different from `File.h`)
    *   Provides general Input/Output utility functions, also under the `IO` namespace.
    *   String manipulation functions: `trim`, `split`, `startsWith`, `endsWith`, `wideStringToMb`, `mbToWideStr`, `formatString`.
    *   `messageBox`: A function to display native Windows message boxes (Ok, Yes/No, Error, Warning, etc.).

*   **IconsFontAwesome5.h**:
    *   Contains definitions for Font Awesome 5 icons. These are used to display icons in the ImGui UI (e.g., `ICON_FA_SAVE`, `ICON_FA_PLAY`).

*   **ImGui/**: Sub-directory containing the Dear ImGui library source files.
    *   ImGui is a popular immediate mode graphical user interface library for C++.
    *   Files like `imgui.h`, `imgui.cpp`, `imgui_draw.cpp`, `imgui_widgets.cpp` are core ImGui components.
    *   `imgui_impl_glfw.cpp/.h` and `imgui_impl_opengl3.cpp/.h` are backend implementations for using ImGui with GLFW (windowing) and OpenGL3 (rendering).
    *   `imconfig.h`: Custom configuration for ImGui.
    *   `imgui_stdlib.cpp/.h`: Optional helpers for using `std::string` with ImGui input widgets.

*   **ImGuiManager.h/.cpp**:
    *   Manages the ImGui instance for the application.
    *   Initializes ImGui, sets up fonts (including Font Awesome), colors, and style.
    *   Handles new frame logic for ImGui and rendering ImGui draw data.
    *   Manages DPI scaling for the UI.

*   **InputBinding.h/.cpp**:
    *   Defines the `InputBinding` (single key + modifiers) and `MultiInputBinding` (named action with multiple possible bindings) structures.
    *   Provides functions to convert bindings to string representations for display (`ToShortcutString`) and for serialization (`ToSerializedString`), and vice-versa (`FromSerializedString`).
    *   Contains helper functions for checking ImGui key states based on these binding structures (`ImGui::IsDown`, `ImGui::IsPressed`).

*   **Jacket.h/.cpp**:
    *   Manages loading and storing album jacket artwork associated with a score.
    *   Likely handles loading image files into textures that can be displayed in the UI.

*   **JsonIO.h**: (No .cpp file, header-only helpers)
    *   Provides helper functions for working with the `nlohmann::json` library.
    *   Functions like `keyExists`, `arrayHasData`, `tryGetValue` (to safely extract values from JSON objects).
    *   Also declares functions for serializing/deserializing `Note` objects and selections of notes to/from JSON (likely used for clipboard copy/paste functionality rather than file saving for whole scores).

*   **Language.h/.cpp**:
    *   Defines the `Language` class, which stores a set of translated strings for a specific language code (e.g., "en", "ja").
    *   Reads language data from files (likely `.ini` or simple key-value text files).

*   **Localization.h/.cpp**:
    *   Manages multiple `Language` objects.
    *   Allows loading language files from a directory and setting the current language for the application.
    *   Provides the `getString(const std::string& key)` function, which is used throughout the UI code to get translated text.

*   **Math.h/.cpp**:
    *   Defines basic math structures like `Vector2` and `Color`.
    *   Contains various mathematical utility functions:
        *   `lerp` (linear interpolation) and various easing functions (`easeIn`, `easeOut`, `easeInOut`, `easeOutIn`).
        *   `roundUpToPowerOfTwo`.
        *   `gcf` (greatest common factor).
        *   `isWithinRange`.

*   **MikuMikuWorld.rc**: Windows resource file. Contains information like application icon, version info for the executable.
*   **MikuMikuWorld.vcxproj/.vcxproj.filters**: Visual Studio project files.

*   **Note.h/.cpp**:
    *   Defines the `Note` class, representing a single musical note in a chart.
    *   Properties: `ID`, `parentID` (for hold steps), `tick` (timing), `lane`, `width`, `critical`, `friction`, `flick` type, `layer`.
    *   Defines the `HoldStep` struct (ID, type, ease) and the `HoldNote` class (which contains a start `HoldStep`, a vector of middle `HoldStep`s, and an end note ID, plus properties like `startType`, `endType`, `fadeType`, `guideColor`).
    *   Contains utility functions related to notes (e.g., `cycleFlick`, `getNoteSpriteIndex`, `getNoteSE`).
    *   `NoteTextures` struct and `noteTextures` global variable (likely for texture IDs related to note rendering).

*   **NoteTypes.h**:
    *   Defines various enums related to notes:
        *   `NoteType`: Tap, Hold, HoldMid, HoldEnd, Damage.
        *   `FlickType`: None, Default, Left, Right.
        *   `HoldStepType`: Normal, Hidden, Skip.
        *   `EaseType`: Linear, EaseIn, EaseOut, etc.
        *   `HoldNoteType`: Normal, Hidden, Guide.
        *   `GuideColor`: Neutral, Red, Green, etc.
        *   `FadeType`: Out, None, In.
    *   Provides `constexpr const char*` arrays for converting these enum values to strings (used for UI display or serialization).

*   **NotesPreset.h/.cpp**:
    *   Manages note presets or templates that can be quickly inserted into the chart.
    *   Handles saving and loading these presets, likely as small chart snippets.

*   **OpenGlLoader.cpp**: (No .h file)
    *   Likely responsible for initializing GLAD (the OpenGL loading library) to get OpenGL function pointers.

*   **Rendering/**: Sub-directory for graphics rendering code.
    *   **AnchorType.h**: Defines an enum for UI anchoring (e.g., top-left, center).
    *   **Camera.h/.cpp**:
        *   Manages the camera used for rendering the 2D chart view (and potentially 3D elements if any).
        *   Handles view transformations (pan, zoom).
    *   **Framebuffer.h/.cpp**:
        *   Encapsulates an OpenGL Framebuffer Object (FBO).
        *   Used for off-screen rendering, possibly for effects or intermediate rendering steps.
    *   **Quad.h**: (No .cpp file, likely header-only or inline)
        *   Represents a 2D quadrilateral (rectangle). Used for drawing sprites or UI elements.
    *   **Renderer.h/.cpp**:
        *   Manages the main rendering pipeline.
        *   Contains functions to draw various elements of the editor (notes, lanes, timeline, background, UI).
        *   Interacts with shaders, textures, and camera.
    *   **Shader.h/.cpp**:
        *   Represents an OpenGL shader program (vertex and fragment shaders).
        *   Handles loading shader source from files, compiling, linking, and setting uniform variables.
    *   **Sprite.h/.cpp**:
        *   Manages drawing 2D sprites (textured quads).
        *   Handles setting texture, color, transformations for sprites.
    *   **Texture.h/.cpp**:
        *   Represents an OpenGL texture.
        *   Handles loading image data (from `stb_image`) into an OpenGL texture object, setting filtering/wrapping modes.
    *   **VertexBuffer.h/.cpp**:
        *   Encapsulates an OpenGL Vertex Buffer Object (VBO) and Vertex Array Object (VAO).
        *   Manages vertex data for drawing geometric primitives.

*   **ResourceManager.h/.cpp**:
    *   A static class for managing shared resources like textures and shaders.
    *   Provides methods to load resources by filename/name and retrieve them by ID or name.
    *   Helps avoid redundant loading and provides a central point for resource disposal.

*   **SUS.h**: (No .cpp file, just struct definitions)
    *   Defines structures for representing data from ".sus" (Sliding Universal Score / Sekai Viewer) chart files. This is a common external chart format.
    *   Structs like `SUSNote`, `BPM`, `BarLength`, `HiSpeedGroup`, `SUSMetadata`, and the main `SUS` struct to hold all parsed data.
    *   This implies the editor can import `.sus` files.

*   **Score.h/.cpp**:
    *   Defines the main `Score` struct which holds all the data for a music chart (metadata, notes, hold notes, tempo changes, time signatures, hi-speed changes, skills, fever, layers, waypoints).
    *   `ScoreMetadata` struct holds title, artist, author, music/jacket file paths, offset.
    *   Functions `serializeScore` and `deserializeScore` handle saving and loading scores to/from the custom binary format (`.mmws`, `.ccmmws`).

*   **ScoreContext.h/.cpp**:
    *   Acts as a high-level container and manager for the currently active score being edited.
    *   Holds the `Score` object, `EditorScoreData` (for UI-related metadata like jacket image object), `ScoreStats`, `HistoryManager` (for undo/redo specific to this score), `AudioManager` (for this score's music/preview), and `PasteData`.
    *   Manages selections (`selectedNotes`, `selectedHiSpeedChanges`), current tick position, layers.
    *   Provides many methods for manipulating the score based on editor actions (e.g., `deleteSelection`, `copySelection`, `paste`, `setFlick`, `setEase`, `pushHistory`).
    *   Calculates time based on ticks (`getTimeAtCurrentTick`).

*   **ScoreEditor.h/.cpp**:
    *   The main class for the chart editing UI and logic.
    *   Likely contains instances of `ScoreContext`, `Renderer`, and various UI panel/widget classes.
    *   Handles user input from ImGui, translates it into commands for `ScoreContext`, and manages the display of the chart editor interface (timeline, note properties, toolbars, etc.).

*   **ScoreStats.h/.cpp**:
    *   Calculates and stores statistics about a `Score` (e.g., total note count, combo count, density).

*   **SettingsWindow.h/.cpp**:
    *   Manages the UI for the application settings window.
    *   Allows users to change preferences defined in `ApplicationConfiguration` (e.g., key bindings, theme, audio, timeline appearance).

*   **Tempo.h/.cpp**: (Filename is `Tempo.h`, but also contains TimeSignature logic)
    *   Defines `Tempo` struct (tick, bpm) and `TimeSignature` struct (measure, numerator, denominator).
    *   Provides utility functions for time calculations:
        *   Snapping ticks to a grid (`snapTick`).
        *   Converting between ticks and seconds (`ticksToSec`, `secsToTicks`) considering tempo changes.
        *   Converting between measures and ticks (`measureToTicks`, `accumulateMeasures`) considering time signature changes.
        *   Finding the active tempo or time signature at a given tick/measure.

*   **TimelineMode.h**:
    *   Defines enums related to the timeline's editing mode or the type of object being placed (e.g., `TimelineMode_Select`, `TimelineMode_Tap`, `TimelineMode_Bpm`).

*   **UI.h/.cpp**:
    *   A static class providing helper functions and constants for building the ImGui user interface.
    *   `IMGUI_TITLE`, `MODAL_TITLE` macros for consistent window titles.
    *   Standard button sizes (`btnNormal`, `btnSmall`), toolbar button styling.
    *   Custom ImGui widget helpers (`transparentButton`, `propertyLabel`, `addIntProperty`, `addFileProperty`, `tooltip`, `toolbarButton`, etc.) to simplify UI construction and maintain a consistent look and feel.
    *   Manages accent colors and base themes (Dark/Light).

*   **Utilities.h/.cpp**:
    *   General utility functions and classes.
    *   `DECLARE_ENUM_FLAG_OPERATORS` macro to enable bitwise operations on enums.
    *   `Utilities` static class:
        *   `getCurrentDateTime`, `getSystemLocale`, `splitString`, `centerImGuiItem`, `ImGuiCenteredText`.
    *   `Result` class: A simple class to return operation status (Success, Warning, Error) and an optional message.
    *   Miscellaneous helpers like `boolToString`, `arrayLength`, `isArrayIndexInBounds`, `findArrayItem`.
    *   `drawShadedText` for drawing text with a simple shadow/outline.

*   **Windows/**: Sub-directory for UI panels/windows of the editor.
    *   **AboutWindow.h/.cpp**: Manages the "About" dialog, showing application version, credits, etc.
    *   **DebugWindow.h/.cpp**: Shows debugging information, performance metrics, or internal state, likely only enabled in debug builds or via a setting.
    *   **dialogs/**: Contains code for various dialog boxes.
        *   **ExportSusDialog.h/.cpp**: UI for exporting a chart to the `.sus` format.
        *   **FeverTimeDialog.h/.cpp**: UI for setting fever start/end times.
        *   **GUIDialog.h/.cpp**: Base class or common utilities for dialog windows.
        *   **HiSpeedDialog.h/.cpp**: UI for editing Hi-Speed changes.
        *   **LayerDialog.h/.cpp**: UI for managing layers in the chart.
        *   **MetadataDialog.h/.cpp**: UI for editing score metadata (title, artist, etc.).
        *   **NotesPresetDialog.h/.cpp**: UI for managing note presets.
        *   **TimeSignatureDialog.h/.cpp**: UI for editing time signature changes.
        *   **UnsavedChangesDialog.h/.cpp**: UI that prompts the user to save unsaved changes before closing a chart or the application.
        *   **WaypointDialog.h/.cpp**: UI for managing waypoints in the chart.
    *   **HistoryPanel.h/.cpp**: UI panel that displays the undo/redo history.
    *   **LayersWindow.h/.cpp**: UI window for managing chart layers (visibility, selection). (Seems duplicative of `dialogs/LayerDialog.h` - might be a panel vs. modal dialog).
    *   **PropertiesWindow.h/.cpp**: UI panel that displays properties of the selected note(s) or event(s), allowing the user to edit them.
    *   **StatsPanel.h/.cpp**: UI panel that displays chart statistics from `ScoreStats`.
    *   **TimelineControlsWindow.h/.cpp**: UI for controls related to the timeline (e.g., playback buttons, tempo display, current tick, zoom controls).
    *   **ToolbarWindow.h/.cpp**: UI for the main toolbar, containing buttons for common actions (save, open, edit modes, etc.).
    *   **WaypointsPanel.h/.cpp**: UI panel for listing and managing waypoints.
