/*

I'll create a simple pseudocode comment for each file that you can paste at the top to understand what each file does at a glance. Here they are:

For GameObject.h:

GameObject.h - Abstract base class for game objects
- Defines common properties (position, velocity, color)
- Declares virtual update and draw methods
- Provides getters/setters for position and velocity

For GameObject.cpp:

GameObject.cpp - Implementation of base GameObject class
- Constructor initializes position, velocity and color
- Getter/setter methods for position and velocity

For RocketPart.h:

RocketPart.h - Abstract base class for rocket components
- Defines common properties (relative position, color)
- Declares virtual draw method

For RocketPart.cpp:

RocketPart.cpp - Implementation of RocketPart base class
- Constructor initializes relative position and color

For Engine.h :

Engine.h - Defines rocket engine component
- Inherits from RocketPart
- Properties: shape, thrust power
- Methods for drawing and getting thrust

For Engine.cpp:

Engine.cpp - Implementation of rocket engine
- Creates triangular shape for engine visualization
- Draws engine with proper positioning and rotation
- Returns thrust power for propulsion calculations

For VectorHelper.h:

VectorHelper.h - Utility functions for vector operations
- normalize(): Converts vector to unit vector
- distance(): Calculates distance between two points

For GravitySimulator.h:

GravitySimulator.h - Handles gravity physics
- Manages planets and rockets
- Simulates gravitational attraction between objects
- Controls planet-to-planet and planet-to-rocket gravity

For GravitySimulator.cpp:

GravitySimulator.cpp - Implements gravity physics calculations
- Adds/removes planets and rockets
- Calculates gravitational forces using inverse square law
- Applies forces to update object velocities
- Handles special cases like planet pinning and collision avoidance

For Planet.h:

Planet.h - Defines celestial body objects
- Inherits from GameObject
- Properties: mass, radius, visual shape
- Methods for orbit visualization, velocity vectors
- Handles physics updates and rendering

For Planet.cpp:

Planet.cpp - Implements planet behavior and rendering
- Updates position based on velocity
- Calculates radius based on mass relationship
- Renders planet and visual indicators
- Simulates orbital paths for trajectory visualization

For GameState.h:

GameState.h - Data structures for network synchronization
- Defines serializable states for rockets and planets
- Contains complete game state for network transmission
- Declares operators for packet serialization

For GameState.cpp:

GameState.cpp - Network serialization implementation
- Implements packet operators for sf::Vector2f and sf::Color
- Serializes/deserializes rocket and planet states
- Handles full game state transmission

For PlayerInput.h:

PlayerInput.h - Structure for player control inputs
- Contains fields for player ID and control states
- Tracks thrust, rotation, and vehicle switching
- Implements serialization for network transmission

For NetworkManager.h:

NetworkManager.h - Handles multiplayer networking
- Manages server/client connections
- Sends/receives game states and player inputs
- Provides callbacks for handling network events
- Includes diagnostics for ping and packet loss

For NetworkManager.cpp:

NetworkManager.cpp - Implements networking functionality
- Hosts or joins multiplayer games
- Handles client connections and disconnections
- Processes incoming messages and forwards to game systems
- Manages heartbeats and connection timeouts
- Tracks network performance metrics

For GameServer.h:

GameServer.h - Manages authoritative game state for multiplayer
- Maintains planets and player vehicles
- Processes player inputs
- Generates synchronized game state
- Handles player joining/leaving

For GameClient.h:

GameClient.h - Manages client-side game state
- Processes game states from server
- Handles local player input
- Implements client-side prediction and interpolation
- Manages remote player rendering

for GameClient.cpp:

GameClient.cpp - Implements client-side game logic
- Initializes local game environment
- Processes server game states
- Applies client-side prediction for responsive control
- Interpolates remote player positions
- Handles player joining/leaving

For Button.cpp:

Button.cpp - Implements UI button functionality
- Creates interactive button with shape
- Handles mouse hover and click detection
- Executes callback function when clicked
- Renders button appearance

For Rocket.h:

Rocket.h - Defines player-controlled spacecraft
- Inherits from GameObject
- Manages rocket parts, thrust, rotation
- Handles collision detection with planets
- Provides visualization for velocity and trajectory

For InputManager.h:

InputManager.h - Processes player input
- Handles keyboard events for vehicle control
- Manages different control schemes for multiplayer
- Provides input state tracking

For InputManager.cpp:

InputManager.cpp - Implements player input processing
- Maps keyboard controls to vehicle actions
- Handles thrust level adjustments
- Manages vehicle transformation
- Differentiates between host/client controls in multiplayer

For GameManager.h:

GameManager.h - Coordinates overall game systems
- Manages game view and camera
- Coordinates planets and vehicles
- Handles game loop operations
- Controls zoom and rendering settings

For VehicleManager.h:

VehicleManager.h - Manages player vehicles
- Switches between rocket and car modes
- Coordinates vehicle updates and rendering
- Passes control inputs to active vehicle
- Handles vehicle transformations

For OrbitalMechanics.h:

OrbitalMechanics.h - Calculates orbital parameters
- Computes apoapsis, periapsis, and orbital period
- Calculates eccentricity and orbital energy
- Provides utilities for trajectory prediction

For VehicleManager.cpp:

VehicleManager.cpp - Implements vehicle management
- Initializes rocket and car vehicles
- Handles vehicle switching logic based on proximity to planets
- Updates active vehicle based on type
- Delegates control inputs to appropriate vehicle

For UIManager.h:

UIManager.h - Manages game user interface
- Displays vehicle information, planet data, and controls
- Creates and updates UI panels
- Handles different display modes for multiplayer

For TextPanel.h:

TextPanel.h - UI component for text display
- Creates background panel with text
- Manages positioning and sizing
- Provides methods for updating displayed text

For GameConstants.h:

GameConstants.h - Central location for game parameters
- Defines physical constants (gravity, friction)
- Specifies object sizes and masses
- Sets visualization parameters
- Defines vehicle characteristics

For Car.h:

Car.h - Defines surface vehicle mode
- Inherits from GameObject
- Properties: wheels, body, direction
- Handles surface movement physics
- Manages planetary surface detection

For Car.cpp:

Car.cpp - Implements car vehicle behavior
- Creates car visual representation
- Handles surface movement physics
- Manages vehicle direction and orientation
- Handles transformation from rocket mode

For Button.h:

Button.h - UI button class
- Properties: shape, text, callback function
- Methods for handling mouse interaction
- Functions for rendering and position management

For NetworkWrapper.h:

NetworkWrapper.h - Simplifies network system usage
- Coordinates NetworkManager with GameClient/GameServer
- Provides high-level interface for multiplayer functionality
- Manages initialization and updates for networking components

For GameManager.cpp:

GameManager.cpp - Implements core game management
- Initializes game environment with planets and vehicles
- Updates physics, input, and rendering systems
- Manages camera and zoom behavior
- Handles game events and user interactions

For TextPanel.cpp:

TextPanel.cpp - Implements text panel UI component
- Creates background rectangle with text overlay
- Handles text updates and positioning
- Renders text with appropriate styling

For Rocket.cpp:

Rocket.cpp - Implements rocket spacecraft behavior
- Creates visual representation with triangular body
- Handles thrust application and rotation physics
- Manages collision detection with planets
- Simulates trajectory prediction
- Renders velocity vectors and gravity indicators

For NetworkWrapper.cpp:

NetworkWrapper.cpp - Implements networking coordination
- Initializes multiplayer components based on host/client role
- Connects NetworkManager with GameServer or GameClient
- Sets up callbacks for network events
- Manages network updates and state synchronization

For GameServer.cpp:

GameServer.cpp - Implements server-side game logic
- Initializes game world with planets and physics
- Processes player inputs from clients
- Updates game state for all objects
- Generates synchronized state for transmission
- Manages player joining and leaving

For MenuSystem.h:

MenuSystem.h - Defines game menu interface
- Manages menu states (main menu, join menu)
- Handles UI elements for game setup
- Processes user input for menu navigation
- Provides multiplayer connection settings

For MenuSystem.cpp:

MenuSystem.cpp - Implements game menu system
- Renders menu screens with buttons and input fields
- Processes keyboard and mouse input
- Handles multiplayer connection setup
- Launches separate process for server hosting

For main.cpp:*/
