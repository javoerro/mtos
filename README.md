# MToS - Memory Transfer over Serial

MToS is a library for the ESP-IDF framework that enables the transfer of memory blocks between two ESP32 devices using the UART (Universal Asynchronous Receiver-Transmitter) serial communication protocol. It provides a convenient and efficient way to exchange data between devices over a serial connection.


## Features

- Efficient transfer of memory blocks over UART.
- Supports both blob (arbitrary data) and array (fixed-size element) transfers.
- Various string and memory manipulation functions available for data processing.
- Seamless integration with the ESP-IDF framework.
- Simplified API for easy usage and configuration.
- Operates on a master/slave scheme, allowing independent functionality for each shared memory block.

## Requirements

- ESP-IDF framework (version 4.4.3 or higher)
- Two ESP32 devices with UART interfaces.

```
     ESP32 #1                   ESP32 #2
+---------------+          +---------------+
|               |          |               |
|    GPIO TX    |--------->|    GPIO RX    |
|    GPIO RX    |<---------|    GPIO TX    |
|               |          |               |
+---------------+          +---------------+
```

## Installation

1. Clone the MToS repository:

```bash
git clone https://github.com/javoerro/mtos.git
```

2. Test the exaple or copy components/mtos into your project

3. Run 'menuconfig' to configure library parameters, such as the UART GPIOs to be used, among others.

4. Build and flash your ESP32 project as usual using the ESP-IDF build system.

## Usage

1. Include the MToS library header file in your source code:

```c
#include "mtos.h"
```

2. Initialize the MToS library and UART communication:

```c
mtos_init(evt_callback);
```

3. Create memory blocks and arrays for data transfer:

```c
mtos_new_blob(name, length, slave, trigger, pattern);
mtos_new_array(name, n, size, slave, trigger, pattern);
```

The lookup of memory blocks is performed using pre-shared trigger and pattern keys

4. Request data block from remote device:

```c
//only from master
mtos_call(name, timeout_ms, max_chunk_size);
```

5. Access and manipulate memory blocks:

```c
mtos_grab_mb(name, ticks, &ptr, &length);
mtos_return_mb(name);
```

6. Perform various string and memory operations:

```c
mtos_strcat(name, src);
mtos_strchr(name, chr);
mtos_strcmp(name, src);
mtos_strcpy(name, src);
mtos_strlen(name);
mtos_strncat(name, src, n);
mtos_strncmp(name, src, n);
mtos_strncpy(name, src, n);
mtos_strpbrk(name, src);
mtos_strrchr(name, chr);
mtos_strstr(name, src);
mtos_strtok(name, src);
mtos_memset(name, chr, n);
mtos_memcpy(name, src, n);
mtos_memmove(name, src, n);
```

## Contributing

Contributions are welcome! If you have any ideas, suggestions, or bug reports, please create an issue in the GitHub repository. Pull requests are also encouraged.