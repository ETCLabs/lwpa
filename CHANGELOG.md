# Changelog
All notable changes to the EtcPal library will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- etcpal_queue_receive_from_isr() and etcpal::Queue::ReceiveFromIsr()
- New platform abstraction feature: recursive mutexes (`etcpal/recursive_mutex.h`)
- New platform abstraction feature: event groups (`etcpal/event_group.h`)
- etcpal_netint_is_up()
- Native thread handle functionality:
  * `etcpal_thread_os_handle_t`
  * etcpal_thread_get_os_handle()
  * etcpal_thread_get_current_os_handle()
- New C++ feature: Strongly typed opaque IDs (`etcpal/cpp/opaque_id.h`)
- etcpal/queue implementation expanded to work on Windows and Linux as well as FreeRTOS.

### Changed
- Separated etcpal/lock.h into more specific headers: etcpal/mutex.h, etcpal/signal.h, etcpal/sem.h
  and etcpal/rwlock.h
- Stack size parameters for EtcPal threads are always in bytes, and are translated for the
  underlying thread API if necessary.
- etcpal::Thread value constructor now throws std::system_error on failure instead of etcpal::Error
- EtcPal thread names are now honored on macOS and Linux
- Enum constant names changed in etcpal::LogDispatchPolicy, IpAddrType, UuidVersion due to linting
  rules.

## [0.3.0] - 2020-08-18

Note: This is a pre-release version. This version changelog is not exhaustive.

### Added
- New port: Linux
- New port: macOS
- New port: iOS
- New port: FreeRTOS
- New port: lwIP
- etcpal_init() in etcpal/common.h. etcpal_init() must be called before using
  features defined by feature macros defined in etcpal/common.h.
- Extra documentation page for network interface indexes.
- Addition of a C++ wrapper layer over EtcPal. Added wrapper classes for error,
  inet, lock, log, queue, thread, timer and uuid.
- New module etcpal/queue for RTOS queues. FreeRTOS only for now.
- etcpal/lock: A counting semaphore API.
- etcpal/lock: Functions for posting signals and semaphores from an interrupt
  context.
- etcpal/uuid: Functions to generate version 3 and version 5 UUIDs.
- etcpal/log: Legacy syslog creation added. Interface changed slightly to
  support this.

### Changed
- Naming: Library name changed from lwpa to EtcPal. All API names updated
  accordingly.
- Naming: Separated the platform-specific sources into 'os' and 'net' targets
  to aid in targeting mix-and-match stack environments like FreeRTOS and lwIP.
- Allow EtcPal to be built with no networking support.
- Improvements in mnemonic consistency across the public APIs, including
  addition of meaningful return information, better function naming, etc.
- Modified etcpal_netint and etcpal_inet to make more meaningful use of network
  interface indexes, for portability and compatibility with IPv6.
- etcpal/pdu -> etcpal/acn_pdu, etcpal/root_layer_pdu -> etcpal/acn_rlp. API
  constructs have prefixes changed from `etcpal_` to `acn_` for clarity.

### Removed
- etcpal/bool module. This module is not necessary given our target platform
  and toolchain requirements.
- etcpal/int module. This module is not necessary given our target platform and
  toolchain requirements.

## [0.2.0] - 2019-05-29

Note: This is a pre-release version. This version changelog is not exhaustive.

### Added
- This CHANGELOG file.
- lwpa_mempool: LWPA_MEMPOOL_DEFINE_ARRAY(), which allows a memory pool of
  fixed-size arrays of elements.
- lwpa_uuid: Functions to generate V1 and V4 UUIDs using OS APIs where
  available.

### Changed
- lwpa is now built with CMake. Documentation updated accordingly.
- Harmonize with path-based include structure of other ETC libraries - include
  modules using #include "lwpa/[module].h"
- Windows dependencies removed from unit tests - unit tests are now
  platform-neutral.
- Added lwpa_ prefix to existing values and functions where necessary for better
  namespacing.
- lwpa_socket() signature changed to return a meaningful error code.
- lwpa_poll() removed and replaced with a new API based on context structures.
- lwpa_error: Harmonize naming style with other enums

### Removed
- lwpa_uid (lwpa_uid.h and lwpa_uid.c) and estardm (estardm.h). These are now in
  the [RDM](https://github.com/ETCLabs/RDM) repository as rdm/uid.h, rdm/uid.c,
  and rdm/defs.h, respectively.
- estardmnet (estardmnet.h). This is now in the
  [RDMnet](https://github.com/ETCLabs/RDMnet) repository as rdmnet/defs.h.
- Visual Studio project files, as we now build with CMake.

## [0.1.0] - 2018-10-18

Note: This is a pre-release version. This version changelog is not exhaustive.

### Added
- Initial library modules, tests and documentation.

[Unreleased]: https://github.com/ETCLabs/EtcPal/compare/stable...develop
[0.3.0]: https://github.com/ETCLabs/EtcPal/compare/v0.2.0...v0.3.0
[0.2.0]: https://github.com/ETCLabs/EtcPal/compare/v0.1.0...v0.2.0
[0.1.0]: https://github.com/ETCLabs/EtcPal/releases/tag/v0.1.0
