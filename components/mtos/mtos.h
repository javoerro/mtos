#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "typedefs.h"

/**
 * @brief Initialize the MTOS module.
 *
 * This function initializes the MTOS module by setting up the event loop, event handlers,
 * queues, and UART configuration.
 *
 * @param evt_callback Pointer to the event handler callback function.
 * @param usr_data     Pointer to the user data to be passed to the event handler.
 */
void mtos_init(mtos_event_handler_t evt_callback, void* usr_data);

/**
 * @brief Creates a new blob in the MTOS list.
 *
 * This function creates a new blob entry in the MTOS list with the specified parameters.
 *
 * @param name     The name of the blob (up to 16 characters).
 * @param length   The length of the blob.
 * @param slave    The slave identifier.
 * @param trigger  The trigger value (up to 8 characters).
 * @param pattern  The pattern value (up to 8 characters).
 *
 * @return  0 for success.
 *         -1 if node allocation fails.
 *         -2 if memory block allocation fails.
 *         -3 if the name already exists in the list.
 */
int mtos_new_blob(char name[16], size_t length, uint8_t slave, char trigger[8], char pattern[8]);

/**
 * @brief Creates a new array in the MTOS list.
 *
 * This function creates a new array entry in the MTOS list with the specified parameters.
 *
 * @param name     The name of the array (up to 16 characters).
 * @param n        The number of elements in the array.
 * @param size     The size of each element in the array.
 * @param slave    The slave identifier.
 * @param trigger  The trigger value (up to 8 characters).
 * @param pattern  The pattern value (up to 8 characters).
 *
 * @return  0 for success.
 *         -1 if node allocation fails.
 *         -2 if array allocation fails.
 *         -3 if the name already exists in the list.
 */
int mtos_new_array(char name[16], size_t n, size_t size, uint8_t slave, char trigger[8], char pattern[8]);

/**
 * @brief Grabs a memory block from the MTOS list.
 *
 * This function grabs a memory block from the MTOS list with the specified name.
 *
 * @param name     The name of the memory block to grab (up to 16 characters).
 * @param ticks    The maximum amount of time to wait for the memory block to become available.
 * @param ptr      Pointer to store the grabbed memory block.
 * @param length   Pointer to store the length of the grabbed memory block.
 *
 * @return  0 for success.
 *         -1 if the memory block with the specified name does not exist.
 *         -2 if failed to acquire the semaphore within the given time limit.
 */
int mtos_grab_mb(char name[16], TickType_t ticks, void** ptr, size_t* length);

/**
 * @brief Returns a memory block to the MTOS list.
 *
 * This function returns a memory block with the specified name to the MTOS list.
 *
 * @param name     The name of the memory block to return (up to 16 characters).
 *
 * @return  0 for success.
 *         -1 if the memory block with the specified name does not exist.
 *         -2 if failed to release the semaphore.
 */
int mtos_return_mb(char name[16]);

/**
 * @brief Resizes a memory block in the MTOS list.
 *
 * This function resizes a memory block with the specified name in the MTOS list to the new size.
 *
 * @param name     The name of the memory block to resize (up to 16 characters).
 * @param n        The new size of the memory block.
 *
 * @return  0 for success.
 *         -1 if the memory block with the specified name does not exist.
 *         -2 if memory reallocation fails.
 */
int mtos_resize(char name[16], size_t n);

/**
 * @brief Concatenates a string to the memory block identified by the given name.
 *
 * This function concatenates the string pointed to by 'src' to the memory block identified by the specified name.
 *
 * @param name The name of the memory block (up to 16 characters).
 * @param src  Pointer to the source string.
 *
 * @return Pointer to the resulting string in the memory block.
 */
char* mtos_strcat(char name[16], const char* src);

/**
 * @brief Locates the first occurrence of a character in the memory block identified by the given name.
 *
 * This function locates the first occurrence of the character 'chr' in the memory block identified by the specified name.
 *
 * @param name The name of the memory block (up to 16 characters).
 * @param chr  The character to search for.
 *
 * @return Pointer to the first occurrence of the character, or NULL if not found.
 */
const char* mtos_strchr(char name[16], char chr);

/**
 * @brief Compares two strings stored in the memory block identified by the given name.
 *
 * This function compares the string in the memory block identified by the specified name with the string pointed to by 'src'.
 *
 * @param name The name of the memory block (up to 16 characters).
 * @param src  Pointer to the string to compare.
 *
 * @return An integer less than, equal to, or greater than zero if the string in the memory block is found to be less than, equal to, or greater than 'src', respectively.
 */
int mtos_strcmp(char name[16], const char* src);

/**
 * @brief Copies a string to the memory block identified by the given name.
 *
 * This function copies the string pointed to by 'src' to the memory block identified by the specified name.
 *
 * @param name The name of the memory block (up to 16 characters).
 * @param src  Pointer to the source string.
 *
 * @return Pointer to the resulting string in the memory block.
 */
char* mtos_strcpy(char name[16], const char* src);

/**
 * @brief Calculates the length of the string in the memory block identified by the given name.
 *
 * This function calculates the length of the string in the memory block identified by the specified name.
 *
 * @param name The name of the memory block (up to 16 characters).
 *
 * @return The length of the string.
 */
size_t mtos_strlen(char name[16]);

/**
 * @brief Concatenates a specified number of characters from a string to the memory block identified by the given name.
 *
 * This function concatenates at most 'n' characters from the string pointed to by 'src' to the memory block identified by the specified name.
 *
 * @param name The name of the memory block (up to 16 characters).
 * @param src  Pointer to the source string.
 * @param n    Maximum number of characters to concatenate.
 *
 * @return Pointer to the resulting string in the memory block.
 */
char* mtos_strncat(char name[16], const char* src, size_t n);

/**
 * @brief Compares a specified number of characters between two strings stored in the memory block identified by the given name.
 *
 * This function compares at most 'n' characters between the string in the memory block identified by the specified name and the string pointed to by 'src'.
 *
 * @param name The name of the memory block (up to 16 characters).
 * @param src  Pointer to the string to compare.
 * @param n    Maximum number of characters to compare.
 *
 * @return An integer less than, equal to, or greater than zero if the string in the memory block is found to be less than, equal to, or greater than 'src', respectively.
 */
int mtos_strncmp(char name[16], const char* src, size_t n);

/**
 * @brief Copies a specified number of characters from a string to the memory block identified by the given name.
 *
 * This function copies at most 'n' characters from the string pointed to by 'src' to the memory block identified by the specified name.
 *
 * @param name The name of the memory block (up to 16 characters).
 * @param src  Pointer to the source string.
 * @param n    Maximum number of characters to copy.
 *
 * @return Pointer to the resulting string in the memory block.
 */
char* mtos_strncpy(char name[16], const char* src, size_t n);

/**
 * @brief Searches a memory block identified by the given name for the first occurrence of any character from the specified string.
 *
 * This function searches the memory block identified by the specified name for the first occurrence of any character from the string pointed to by 'src'.
 *
 * @param name The name of the memory block (up to 16 characters).
 * @param src  Pointer to the string containing the characters to search for.
 *
 * @return Pointer to the first occurrence of a matching character, or NULL if no match is found.
 */
const char* mtos_strpbrk(char name[16], const char* src);

/**
 * @brief Locates the last occurrence of a character in the memory block identified by the given name.
 *
 * This function locates the last occurrence of the character 'chr' in the memory block identified by the specified name.
 *
 * @param name The name of the memory block (up to 16 characters).
 * @param chr  The character to search for.
 *
 * @return Pointer to the last occurrence of the character, or NULL if not found.
 */
const char* mtos_strrchr(char name[16], char chr);

/**
 * @brief Searches for the first occurrence of a substring within the memory block identified by the given name.
 *
 * This function searches the memory block identified by the specified name for the first occurrence of the substring pointed to by 'src'.
 *
 * @param name The name of the memory block (up to 16 characters).
 * @param src  Pointer to the substring to search for.
 *
 * @return Pointer to the first occurrence of the substring, or NULL if not found.
 */
const char* mtos_strstr(char name[16], const char* src);

/**
 * @brief Breaks a string into a series of tokens from the memory block identified by the given name.
 *
 * This function breaks the string in the memory block identified by the specified name into a series of tokens, delimited by the characters in the string pointed to by 'src'.
 *
 * @param name The name of the memory block (up to 16 characters).
 * @param src  Pointer to the string containing the delimiter characters.
 *
 * @return Pointer to the first token found, or NULL if no tokens are found.
 */
char* mtos_strtok(char name[16], const char* src);

/**
 * @brief Fills a memory block identified by the given name with a specified value.
 *
 * This function fills the memory block identified by the specified name with the specified value, repeating 'n' times.
 *
 * @param name The name of the memory block (up to 16 characters).
 * @param chr  The value to be set.
 * @param n    The number of times the value is repeated.
 *
 * @return Pointer to the filled memory block.
 */
void* mtos_memset(char name[16], char chr, size_t n);

/**
 * @brief Copies a specified number of bytes from a source memory block to the memory block identified by the given name.
 *
 * This function copies at most 'n' bytes from the source memory block pointed to by 'src' to the memory block identified by the specified name.
 *
 * @param name The name of the memory block (up to 16 characters).
 * @param src  Pointer to the source memory block.
 * @param n    The number of bytes to copy.
 *
 * @return Pointer to the destination memory block.
 */
void* mtos_memcpy(char name[16], const void* src, size_t n);

/**
 * @brief Moves a specified number of bytes from a source memory block to the memory block identified by the given name.
 *
 * This function moves 'n' bytes from the source memory block pointed to by 'src' to the memory block identified by the specified name.
 *
 * @param name The name of the memory block (up to 16 characters).
 * @param src  Pointer to the source memory block.
 * @param n    The number of bytes to move.
 *
 * @return Pointer to the destination memory block.
 */
void* mtos_memmove(char name[16], const void* src, size_t n);

/**
 * @brief Retrieves the length of the memory block identified by the given name.
 *
 * This function retrieves the length of the memory block identified by the specified name.
 *
 * @param name The name of the memory block (up to 16 characters).
 *
 * @return The length of the memory block, or -1 if the memory block is not found.
 */
int mtos_get_length(char name[16]);

/**
 * @brief Retrieves an element from the memory block identified by the given name at the specified index.
 *
 * This function retrieves an element from the memory block identified by the specified name at the specified index and copies it into the provided 'element' buffer.
 *
 * @param name    The name of the memory block (up to 16 characters).
 * @param element Pointer to the buffer where the retrieved element will be stored.
 * @param index   The index of the element to retrieve.
 *
 * @return 0 if the element is successfully retrieved, -1 if the memory block is not found, -2 if the memory block is a blob, or -3 if the index is out of bounds.
 */
int mtos_borrow_element(char name[16], void* element, size_t index);

/**
 * @brief Returns an element to the memory block identified by the given name at the specified index.
 *
 * This function updates an element in the memory block identified by the specified name at the specified index with the data in the provided 'element' buffer.
 *
 * @param name    The name of the memory block (up to 16 characters).
 * @param element Pointer to the buffer containing the element to be returned.
 * @param index   The index of the element to update.
 *
 * @return 0 if the element is successfully returned, -1 if the memory block is not found, -2 if the memory block is a blob, or -3 if the index is out of bounds.
 */
int mtos_return_element(char name[16], void* element, size_t index);

/**
 * @brief Initiates a call to the memory block identified by the given name.
 *
 * This function initiates a call to the memory block identified by the specified name. The function puts the memory block in the call queue, sets the UART timeout limit, and determines the maximum chunk size for data transmission.
 *
 * @param name            The name of the memory block (up to 16 characters).
 * @param timeout_ms      The timeout value in milliseconds for the UART communication.
 * @param max_chunk_size  The maximum size of each data chunk for transmission.
 *
 * @return 0 if the call is successfully initiated, -1 if the memory block is not found, or -2 if the memory block is a slave.
 */
int mtos_call(char* name, unsigned int timeout_ms, unsigned int max_chunk_size);