**README.md**

## Objective:
This code allows receiving a data block from a remote device through UART connection. The received data block, also known as payload, is used to update the `ptr` member of a node in the linked list `mtos_list_t`. The number of bytes that make up the payload is used to update the `length` member of the `mtos_list_t` linked list node. Although the `length` member is of type `size_t`, it will be restricted to a 24-bit unsigned integer. Therefore, the maximum payload size will be 16,777,215 bytes.

## Communication Protocol:
A communication protocol is implemented based on token detection to indicate the beginning of the frames received via UART. The scheme follows a master-slave configuration, where the local device acts as the master, and the remote device acts as the slave. This means the slave only transmits responses to requests from the local device. The payload is subdivided into several chunks to be transmitted, and the maximum chunk size is limited by the master. This information is included in the request messages sent successively after receiving a response from the slave.

## Frame Format:
The general format of a frame is: |TOKEN|HEADER|DATA|.
The last field is only included in response frames. Both the header and data fields have CRC8 and CRC32, respectively, for integrity control at the end of their respective sections.

## Communication Development:
1. The local device (master) initiates communication by sending |TRIGGER|CHUNK_REQ|.
   - `TRIGGER` is the string found in the `trigger` member of `mtos_list_t`, which is pre-shared between the master and slave.
   - `CHUNK_REQ` is the `chunk_request` structure of the `mtos_header_t` union.

2. The remote device (slave) responds with |TRIGGER|TRIGGER_RES|.
   - `TRIGGER_RES` is the `trigger_response` structure of the `mtos_header_t` union.

3. The remote device must also respond to the chunk request. Immediately after sending the frame from point 2, it sends |PATTERN|CHUNK_RES|CHUNK|.
   - `PATTERN` is the string found in the `pattern` member of `mtos_list_t`, which is pre-shared between the master and slave.
   - `CHUNK_RES` is the `chunk_response` structure of the `mtos_header_t` union.
   - `CHUNK` is a portion of the payload, the number of bytes of which is extracted from the previous field, `chunk_response`. To handle this data more conveniently in the code, the `mtos_chunk_vessel_t` structure is provided.

4. The master device extracts `TRIGGER_RES`, `CHUNK_RES`, and `CHUNK` from the receive buffer. From `TRIGGER_RES`, it extracts the `trigger_response` structure and, by verifying the CRC8, reserves a memory block with the size indicated by that structure.

5. The master device extracts `CHUNK_RES` and `CHUNK` from the receive buffer. From `CHUNK_RES`, it extracts the number of bytes covered by the chunk present in the buffer. With this information, it can extract the chunk and the CRC32 to fill an `mtos_chunk_vessel_t` structure.

6. The integrity of the chunk is verified using CRC32.

7. If the integrity check is successful, the bytes of the chunk are added to the accumulator, the pending payload bytes are recalculated, and the next chunk is requested by preparing the `chunk_request` structure.
   - If the integrity check fails, the chunk data is discarded, and the `chunk_request` structure is prepared to request retransmission of the last chunk.

8. If there are no more pending bytes to receive, the communication is terminated.
   - Otherwise, the chunk request frame |PATTERN|CHUNK_REQ| is sent.