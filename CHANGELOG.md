# CHANGE LOG

### [2.2.1] - 2024-06-08

### Fixed

- [setrate_lws.c, setrate_util.c, setrate.h] Fixed a bug causing error with large CamillaDSP configuration files.

   

### [2.2.0] - 2024-03-27

### Changed

- [all files] Finite state machine, logging functionality and documentation have been improved.

  

### [2.1.1] - 2024-03-24

### Fixed

- [setrate_lws.c] Fixed a bug causing occasional early disconnection from the websocket server.

   

### [2.1.0] - 2024-03-14

### Added

- The *--upsampling* command option has been added to allow upsampling of the captured audio by a fixed integer factor. For example, this flag can be used to instruct CamillaDSP to perform 2X or 4X oversampling regardless of the sample rate of the incoming audio.

### Changed

- The *--err*, --warn, --user and --notice command flags have been removed.  

  

### [2.0.0] - 2023-10-20

### Added

- The *--capture* command flag has been added to have the *capture_samplerate* parameter updated instead of the *samplerate* parameter. This flag should be used  when *CamillaDSP* is configured for resampling of the captured audio to a fixed playback rate.
- The *--address* and *--port* command options have been added to set IP address and port of the websocket server.
- The module *setrate_fsm.c* has been added for management of the finite-state machine that drives the sample rate update process. 

### Changed

- The *--server* command option has been removed.  
- Source code has been completely re-engineered. The process is now driven by a [finite-state machine]( https://www.adservio.fr/post/introduction-to-state-machine  ) to achieve more flexibility/maintainability and much better control. The finite-state machine diagram is provided in the *doc* folder. 
- The connection to the websocket server is now always on. *Libwebsockets* ping/pong protocol handles *keepalive* traffic. Reconnections are handled by the *libwesockets* scheduler. 
- As a result of the above changes, performance has improved noticeably (less than 3ms to complete a config update cycle on a Raspberry Pi 4).

### Fixed

- [setrate_signal.c] Fixed a bug in disabling signals.

  

### [1.0.0] - 2023-09-20

### Added

- A few command options have been added.

### Changed

- Logging has been improved.
- README.md file has been improved.

### Fixed

- [Makefile] Fixed BIN target
- [setrate_main.c] added delay to avoid occasional ENODEV error reported by CamillaDSP (capture device not found) after restoring a valid configuration. 
